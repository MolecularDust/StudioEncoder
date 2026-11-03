#include "encoder-common.h"

#include <obs-nal.h>
#include <plugin-support.h>
#include <util/bmem.h>
#include <util/platform.h>
#include <util/threading.h>

#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#if defined(_M_X64) || defined(__SSE2__)
#include <emmintrin.h>
#define PR_HAVE_SSE2 1
#endif

/* ProRes FourCCs, stored the way obs-ffmpeg-mux expects "codec_type": the four characters read as a
 * big-endian integer (the muxer byte-swaps it on little-endian hosts). */
#define FOURCC(a, b, c, d) (((int64_t)(a) << 24) | ((int64_t)(b) << 16) | ((int64_t)(c) << 8) | (int64_t)(d))

static const int64_t profile_fourcc[] = {
	FOURCC('a', 'p', 'c', 'o'), /* Proxy */
	FOURCC('a', 'p', 'c', 's'), /* LT */
	FOURCC('a', 'p', 'c', 'n'), /* 422 */
	FOURCC('a', 'p', 'c', 'h'), /* HQ */
};

#define S_PROFILE "profile"
#define S_THREADS "threads"
#define S_IN_FLIGHT "in_flight"
#define S_LOG_LEVEL "log_level"
#define S_INFO "info"
#define S_CODEC_TYPE "codec_type"
#define S_OVERLOAD "overload_action"
#define S_SIZING "inflight_sizing"

/* How automatic frames in flight decides how many workers to use. */
enum pr_sizing {
	PR_SIZING_LATE = 0,     /* start at half the logical CPUs, add one worker at a time while frames are late */
	PR_SIZING_MEASURED = 1, /* start small, then size from the measured encode time of a frame */
};

/* Measured sizing: workers needed = frames per second x seconds per frame x this margin, plus one. */
#define PR_SIZING_MARGIN 1.25

enum pr_overload_action {
	PR_OVERLOAD_WARN = 0,
	PR_OVERLOAD_STOP = 1,
};

/* An overloaded second is one where OBS repeated more than this share of frames. Stopping needs several in a
 * row, so a single hiccup (a scene switch, a disk stall) does not end a recording. */
#define PR_OVERLOAD_SHARE 0.02
#define PR_OVERLOAD_STOP_SECONDS 3

#define PR_MAX_IN_FLIGHT 32
/* automatic mode grows whenever the oldest frame is not ready in time */
#define PR_AUTO_START_IN_FLIGHT 4 /* lower bound for the automatic starting depth */

struct pr_encoder;

/* One frame in flight: a backend context, its output buffer and the worker thread that fills it. */
struct pr_slot {
	struct pr_encoder *owner;
	void *ctx;
	struct pr_planes planes;
	uint8_t *out;
	size_t out_size;
	int64_t pts;
	bool ok;
	char err[256];
	uint64_t enc_ns;
	os_sem_t *start;
	os_event_t *done;
	pthread_t thread;
	bool thread_started;
};

struct pr_encoder {
	obs_encoder_t *encoder;
	const struct pr_backend *backend;
	struct pr_params params;

	/* Each slot encodes whole frames independently on its own thread. Frames wait in a FIFO in submission
	 * order and are collected from its head, so packets come out in order however long each frame takes.
	 * With a depth of one the encode is synchronous. */
	struct pr_slot *slots[PR_MAX_IN_FLIGHT];
	int created;   /* slots that exist */
	int depth;     /* frames allowed in flight right now */
	int max_depth; /* ceiling for automatic growth */
	bool auto_depth;
	int fifo[PR_MAX_IN_FLIGHT]; /* slot indices of frames in flight, oldest first */
	int fifo_head;
	int pending;
	int free_list[PR_MAX_IN_FLIGHT];
	int free_count;
	uint64_t frames_at_last_grow;
	int sizing;                  /* enum pr_sizing */
	uint64_t last_sizing_ns;     /* measured sizing: last time the worker count was re-evaluated */
	double avg_worker_ns;         /* moving average of the time one frame spends on a worker */
	volatile bool quit;

	uint8_t *packet_buf;
	size_t packet_cap;

	/* statistics, logged at destroy and when a frame blows its time budget */
	uint64_t frame_interval_ns;
	uint64_t frames;
	uint64_t over_budget;
	uint64_t call_ns;
	uint64_t max_call_ns;
	uint64_t worker_ns;
	uint64_t wait_ns;
	uint64_t last_warn_ns;

	/* Overload: when encoding blocks OBS's video thread for too long, OBS fills the gap by handing the encoder
	 * the same cached frame again (video_output_cur_frame does not advance on a repeat). Those repeats look like
	 * frozen video. They are counted here, per recording, by noticing the same frame buffer twice in a row,
	 * rather than from OBS's shared counters, which OBS resets whenever an output connects. */
	int overload_action;
	const uint8_t *last_frame_data; /* plane 0 of the previous frame */
	uint64_t frames_in;
	uint64_t repeats_in;
	uint64_t window_frames; /* in the current one-second window */
	uint64_t window_repeats;
	uint64_t window_start_ns;
	int overloaded_seconds; /* consecutive */
	bool stopped_for_overload;
};

static void pr_log(const struct pr_encoder *enc, int level, int obs_level, const char *fmt, ...)
{
	if (level > enc->params.log_level)
		return;

	char msg[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);

	obs_log(obs_level, "%s: %s", enc->backend->log_name, msg);
}

static void fill_color(struct pr_params *p, const struct video_output_info *voi)
{
	p->full_range = voi->range == VIDEO_RANGE_FULL;

	switch (voi->colorspace) {
	case VIDEO_CS_601:
		p->color_primaries = 6; /* SMPTE 170M */
		p->color_transfer = 6;
		p->color_matrix = 6;
		break;
	case VIDEO_CS_2100_PQ:
		p->color_primaries = 9; /* BT.2020 */
		p->color_transfer = 16; /* PQ */
		p->color_matrix = 9;
		break;
	case VIDEO_CS_2100_HLG:
		p->color_primaries = 9;
		p->color_transfer = 18; /* HLG */
		p->color_matrix = 9;
		break;
	case VIDEO_CS_SRGB:
		p->color_primaries = 1;
		p->color_transfer = 13; /* IEC 61966-2-1 */
		p->color_matrix = 1;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
	default:
		p->color_primaries = 1;
		p->color_transfer = 1;
		p->color_matrix = 1;
		break;
	}
}

static void read_settings(struct pr_params *p, obs_data_t *settings, const struct pr_backend *backend)
{
	int profile = (int)obs_data_get_int(settings, S_PROFILE);
	if (profile < PR_PROFILE_PROXY || profile > PR_PROFILE_HQ)
		profile = PR_PROFILE_422;
	p->profile = (enum pr_profile)profile;
	p->threads = (int)obs_data_get_int(settings, S_THREADS);

	/* 0 = automatic */
	int in_flight = (int)obs_data_get_int(settings, S_IN_FLIGHT);
	if (in_flight < 0)
		in_flight = 0;
	if (in_flight > PR_MAX_IN_FLIGHT)
		in_flight = PR_MAX_IN_FLIGHT;
	p->in_flight = in_flight;

	p->qscale = (int)obs_data_get_int(settings, "qscale");
	p->vendor_apl0 = obs_data_get_bool(settings, "vendor_apl0");
	p->log_level = (int)obs_data_get_int(settings, S_LOG_LEVEL);
	p->codec_name = backend->codec_name;
	p->bitrate_mbps = (int)obs_data_get_int(settings, "bitrate_mbps");
	p->nvenc_fast = obs_data_get_int(settings, "nvenc_preset") == 1;
}

/* The muxer reads the container FourCC from the encoder's own settings. Only ProRes needs one; 0 lets the muxer
 * choose its default tag (avc1 for H.264). */
static void publish_codec_type(obs_data_t *settings, const struct pr_backend *backend, enum pr_profile profile)
{
	obs_data_set_int(settings, S_CODEC_TYPE, backend->prores ? profile_fourcc[profile] : 0);
}

static void *pr_worker(void *param)
{
	struct pr_slot *s = param;
	struct pr_encoder *enc = s->owner;

	while (os_sem_wait(s->start) == 0) {
		if (enc->quit)
			break;

		const uint64_t t0 = os_gettime_ns();
		s->err[0] = '\0';
		s->out_size = 0;
		s->ok = enc->backend->encode(s->ctx, s->out, enc->packet_cap, &s->out_size, s->err, sizeof(s->err));
		s->enc_ns = os_gettime_ns() - t0;
		os_event_signal(s->done);
	}
	return NULL;
}

static void destroy_slot(struct pr_encoder *enc, struct pr_slot *s)
{
	if (s->thread_started) {
		os_sem_post(s->start);
		pthread_join(s->thread, NULL);
	}
	if (s->ctx)
		enc->backend->destroy(s->ctx);
	if (s->start)
		os_sem_destroy(s->start);
	if (s->done)
		os_event_destroy(s->done);
	bfree(s->out);
	bfree(s);
}

/* Creates one more slot and puts it on the free list. */
static bool add_slot(struct pr_encoder *enc, char *err, size_t err_len)
{
	if (enc->created >= PR_MAX_IN_FLIGHT) {
		snprintf(err, err_len, "at most %d frames can be in flight", PR_MAX_IN_FLIGHT);
		return false;
	}

	struct pr_slot *s = bzalloc(sizeof(*s));
	s->owner = enc;
	s->out = bmalloc(enc->packet_cap);
	if (os_sem_init(&s->start, 0) != 0 || os_event_init(&s->done, OS_EVENT_TYPE_AUTO) != 0) {
		snprintf(err, err_len, "could not create synchronisation objects");
		goto fail;
	}
	s->ctx = enc->backend->create(&enc->params, err, err_len);
	if (!s->ctx)
		goto fail;
	if (pthread_create(&s->thread, NULL, pr_worker, s) != 0) {
		snprintf(err, err_len, "could not start worker thread");
		goto fail;
	}
	s->thread_started = true;

	const int idx = enc->created++;
	enc->slots[idx] = s;
	enc->free_list[enc->free_count++] = idx;
	return true;

fail:
	destroy_slot(enc, s);
	return false;
}

static void free_slots(struct pr_encoder *enc)
{
	enc->quit = true;
	for (int i = 0; i < enc->created; i++)
		destroy_slot(enc, enc->slots[i]);
	enc->created = 0;
}

void *pr_encoder_create(const struct pr_backend *backend, obs_data_t *settings, obs_encoder_t *encoder)
{
	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video ? video_output_get_info(video) : NULL;
	if (!voi) {
		obs_log(LOG_ERROR, "%s: no video output available", backend->log_name);
		return NULL;
	}

	if (voi->format != VIDEO_FORMAT_P216) {
		obs_log(LOG_ERROR,
			"%s: video format is %s, but this encoder needs P216. Set Settings > Advanced > Color Format to "
			"P216 (Rec. 709, Limited).",
			backend->log_name, get_video_format_name(voi->format));
		obs_encoder_set_last_error(encoder, obs_module_text("Error.WrongFormat"));
		return NULL;
	}

	struct pr_encoder *enc = bzalloc(sizeof(*enc));
	enc->encoder = encoder;
	enc->backend = backend;

	read_settings(&enc->params, settings, backend);
	enc->params.width = obs_encoder_get_width(encoder);
	enc->params.height = obs_encoder_get_height(encoder);
	enc->params.fps_num = voi->fps_num;
	enc->params.fps_den = voi->fps_den;
	fill_color(&enc->params, voi);
	publish_codec_type(settings, enc->backend, enc->params.profile);

	if (enc->params.width == 0 || enc->params.height == 0 || (enc->params.width & 1)) {
		obs_log(LOG_ERROR, "%s: unsupported frame size %ux%u (width must be even)", backend->log_name,
			enc->params.width, enc->params.height);
		obs_encoder_set_last_error(encoder, obs_module_text("Error.BadSize"));
		bfree(enc);
		return NULL;
	}

	/* Automatic mode starts at half the logical CPUs, so the first seconds of a recording are not spent ramping
	 * up, and may grow to one frame per logical CPU. */
	enc->auto_depth = enc->params.in_flight == 0;
	if (enc->auto_depth) {
		int cores = os_get_logical_cores();
		enc->max_depth = cores < PR_AUTO_START_IN_FLIGHT ? PR_AUTO_START_IN_FLIGHT
								  : (cores > PR_MAX_IN_FLIGHT ? PR_MAX_IN_FLIGHT : cores);
		enc->sizing = (int)obs_data_get_int(settings, S_SIZING);
		/* measured sizing starts small and sizes itself once the first frames are timed */
		int start_depth = enc->sizing == PR_SIZING_MEASURED ? PR_AUTO_START_IN_FLIGHT : cores / 2;
		if (start_depth < PR_AUTO_START_IN_FLIGHT)
			start_depth = PR_AUTO_START_IN_FLIGHT;
		enc->depth = start_depth < enc->max_depth ? start_depth : enc->max_depth;
	} else {
		enc->max_depth = enc->params.in_flight;
		enc->depth = enc->params.in_flight;
	}
	/* some backends open a hardware session per frame in flight, and those sessions are limited */
	if (backend->max_in_flight > 0) {
		if (enc->max_depth > backend->max_in_flight)
			enc->max_depth = backend->max_in_flight;
		if (enc->depth > enc->max_depth)
			enc->depth = enc->max_depth;
	}
	/* backends use this to choose between frame-level and slice-level parallelism */
	enc->params.in_flight = enc->max_depth;

	/* Real footage at HQ 4K is under 4 MB per frame, but pure noise reaches about 2.5 bytes per pixel (20 MB at 4K
	 * HQ, 8.8 MB even at Proxy). 3 bytes per pixel covers that; Windows only backs the pages a frame actually
	 * writes, so the usual cost is a few MB per frame in flight. */
	enc->packet_cap = (size_t)enc->params.width * enc->params.height * 3 + 1048576;
	enc->packet_buf = bmalloc(enc->packet_cap);
	enc->frame_interval_ns = enc->params.fps_num ? (uint64_t)enc->params.fps_den * 1000000000ULL / enc->params.fps_num
						     : 0;

	enc->overload_action = (int)obs_data_get_int(settings, S_OVERLOAD);
	enc->window_start_ns = os_gettime_ns();

	char err[256] = "";
	for (int i = 0; i < enc->depth; i++) {
		if (!add_slot(enc, err, sizeof(err)))
			goto fail;
	}

	static const char *const profile_names[] = {"Proxy", "LT", "422", "HQ"};
	char quality[64];
	if (backend->prores)
		snprintf(quality, sizeof(quality), "profile %s", profile_names[enc->params.profile]);
	else
		snprintf(quality, sizeof(quality), "%d Mbit/s, preset %s", enc->params.bitrate_mbps,
			 enc->params.nvenc_fast ? "p1 (fast)" : "p5 (quality)");
	if (enc->auto_depth)
		obs_log(LOG_INFO, "%s: %ux%u %u/%u fps, %s, P216 input, frames in flight: automatic (%d, up to %d)",
			backend->log_name, enc->params.width, enc->params.height, enc->params.fps_num, enc->params.fps_den,
			quality, enc->depth, enc->max_depth);
	else
		obs_log(LOG_INFO, "%s: %ux%u %u/%u fps, %s, P216 input, %d frame%s in flight", backend->log_name,
			enc->params.width, enc->params.height, enc->params.fps_num, enc->params.fps_den, quality, enc->depth,
			enc->depth == 1 ? "" : "s");
	return enc;

fail:
	obs_log(LOG_ERROR, "%s: could not create encoder: %s", backend->log_name, err[0] ? err : "unknown error");
	obs_encoder_set_last_error(encoder, err[0] ? err : obs_module_text("Error.CreateFailed"));
	free_slots(enc);
	bfree(enc->packet_buf);
	bfree(enc);
	return NULL;
}

void pr_encoder_destroy(void *data)
{
	struct pr_encoder *enc = data;
	if (!enc)
		return;

	if (enc->frames) {
		const double n = (double)enc->frames;
		obs_log(LOG_INFO,
			"%s: %llu frames, %d in flight at the end; per frame: encode %.1f ms (worker), OBS thread %.1f "
			"ms (max %.1f), waiting %.1f ms; %llu over the %.1f ms frame budget",
			enc->backend->log_name, (unsigned long long)enc->frames, enc->depth,
			(double)enc->worker_ns / n / 1e6, (double)enc->call_ns / n / 1e6, (double)enc->max_call_ns / 1e6,
			(double)enc->wait_ns / n / 1e6, (unsigned long long)enc->over_budget,
			(double)enc->frame_interval_ns / 1e6);

		const uint64_t repeated = enc->repeats_in;
		const uint64_t total = enc->frames_in;
		if (repeated > 0)
			obs_log(LOG_WARNING,
				"%s: OVERLOAD SUMMARY: OBS repeated %llu of %llu frames (%.1f%%) because this encoder could not "
				"keep up. Those parts of the recording look frozen.%s",
				enc->backend->log_name, (unsigned long long)repeated, (unsigned long long)total,
				total ? 100.0 * (double)repeated / (double)total : 0.0,
				enc->stopped_for_overload ? " The recording was stopped because of this." : "");
		else
			obs_log(LOG_INFO, "%s: no frames were repeated; the encoder kept up for the whole recording",
				enc->backend->log_name);
	}
	/* OBS keeps feeding the encoder after Stop until a packet past the stop point arrives, so frames still
	 * encoding here were captured after the stop point; the muxer would discard them anyway. */
	if (enc->pending > 0)
		pr_log(enc, PR_LOG_INFO, LOG_INFO,
		       "%d frame%s captured after the stop point discarded at shutdown (expected; the recording is "
		       "complete)",
		       enc->pending, enc->pending == 1 ? "" : "s");

	free_slots(enc);
	bfree(enc->packet_buf);
	bfree(enc);
}

bool pr_encoder_update(void *data, obs_data_t *settings)
{
	struct pr_encoder *enc = data;

	/* Everything but logging is fixed once the encoder is created. */
	enc->params.log_level = (int)obs_data_get_int(settings, S_LOG_LEVEL);
	publish_codec_type(settings, enc->backend, enc->params.profile);
	return true;
}

/* Automatic mode: when the oldest frame was still encoding as its turn came, allow one more frame in flight.
 * Wait for the previous change to take effect (a full pipeline of frames) before growing again. Growth only
 * happens while frames are late, up to one worker per logical CPU: stopping earlier left prores_ks short of
 * real time on a 32-thread CPU that could have kept up. */
static void size_from_cost(struct pr_encoder *enc);

static void maybe_grow(struct pr_encoder *enc, uint64_t waited_ns)
{
	if (!enc->auto_depth || enc->depth >= enc->max_depth)
		return;
	if (enc->sizing == PR_SIZING_MEASURED) {
		size_from_cost(enc);
		return;
	}
	if (waited_ns < enc->frame_interval_ns / 4)
		return;
	if (enc->frames - enc->frames_at_last_grow < (uint64_t)enc->depth)
		return;

	const double throughput = enc->avg_worker_ns > 0.0 ? (double)enc->depth * 1e9 / enc->avg_worker_ns : 0.0;

	if (enc->created <= enc->depth) {
		char err[256] = "";
		if (!add_slot(enc, err, sizeof(err))) {
			pr_log(enc, PR_LOG_WARNINGS, LOG_WARNING, "could not add a worker (%s); staying at %d in flight",
			       err, enc->depth);
			enc->max_depth = enc->depth;
			return;
		}
	}
	enc->depth++;
	enc->frames_at_last_grow = enc->frames;
	pr_log(enc, PR_LOG_INFO, LOG_INFO, "frames in flight raised to %d (oldest frame waited %.1f ms, %.1f fps)",
	       enc->depth, (double)waited_ns / 1e6, throughput);
}

/* Measured sizing: workers needed = frames per second x seconds one frame takes on a worker x a margin, plus one.
 * Evaluated after the first timed frame, then at most once a second; it only ever adds workers, all at once. A
 * slow encoder may still lag for the first fraction of a second while that first frame is timed. */
static void size_from_cost(struct pr_encoder *enc)
{
	if (enc->avg_worker_ns <= 0.0 || !enc->params.fps_den)
		return;
	const uint64_t now = os_gettime_ns();
	if (enc->last_sizing_ns && now - enc->last_sizing_ns < 1000000000ULL)
		return;
	enc->last_sizing_ns = now;

	const double fps = (double)enc->params.fps_num / (double)enc->params.fps_den;
	int need = (int)ceil(fps * enc->avg_worker_ns / 1e9 * PR_SIZING_MARGIN) + 1;
	if (need > enc->max_depth)
		need = enc->max_depth;
	if (need <= enc->depth)
		return;

	const int old_depth = enc->depth;
	while (enc->created < need) {
		char err[256] = "";
		if (!add_slot(enc, err, sizeof(err))) {
			pr_log(enc, PR_LOG_WARNINGS, LOG_WARNING, "could not add a worker (%s); staying at %d in flight", err,
			       enc->created);
			enc->max_depth = enc->created;
			break;
		}
	}
	enc->depth = need < enc->created ? need : enc->created;
	pr_log(enc, PR_LOG_INFO, LOG_INFO,
	       "frames in flight sized from %d to %d: a frame takes %.1f ms on a worker, %.2f fps needs %d with margin",
	       old_depth, enc->depth, enc->avg_worker_ns / 1e6, fps, need);
}

/* Counts frames OBS repeated because encoding held up its video thread (the same frame buffer handed over twice
 * in a row) and, once a second, warns about or stops on them. Returns false when the recording should stop. */
static bool check_overload(struct pr_encoder *enc, const struct encoder_frame *frame, uint64_t now)
{
	const bool repeat = enc->last_frame_data && frame->data[0] == enc->last_frame_data;
	enc->last_frame_data = frame->data[0];
	enc->frames_in++;
	enc->window_frames++;
	if (repeat) {
		enc->repeats_in++;
		enc->window_repeats++;
	}

	if (now - enc->window_start_ns < 1000000000ULL)
		return true;

	const uint64_t repeated = enc->window_repeats;
	const uint64_t total = enc->window_frames;
	enc->window_repeats = 0;
	enc->window_frames = 0;
	enc->window_start_ns = now;

	if (total == 0 || (double)repeated <= (double)total * PR_OVERLOAD_SHARE) {
		enc->overloaded_seconds = 0;
		return true;
	}

	enc->overloaded_seconds++;
	obs_log(LOG_WARNING,
		"%s: OVERLOAD: OBS repeated %llu of the last %llu frames (%.0f%%) because encoding could not keep up; the "
		"recording looks frozen there. Use a lighter profile or a lower resolution; SE: ProRes (AW+, fast) is the lightest ProRes encoder.",
		enc->backend->log_name, (unsigned long long)repeated, (unsigned long long)total,
		100.0 * (double)repeated / (double)total);

	if (enc->overload_action == PR_OVERLOAD_STOP && enc->overloaded_seconds >= PR_OVERLOAD_STOP_SECONDS) {
		enc->stopped_for_overload = true;
		obs_log(LOG_ERROR, "%s: stopping the recording after %d overloaded seconds in a row (On overload: Stop)",
			enc->backend->log_name, enc->overloaded_seconds);
		obs_encoder_set_last_error(enc->encoder, obs_module_text("Error.Overload"));
		return false;
	}
	return true;
}

bool pr_encoder_encode(void *data, struct encoder_frame *frame, struct encoder_packet *packet, bool *received_packet)
{
	struct pr_encoder *enc = data;
	*received_packet = false;

	if (!frame)
		return true;

	const uint32_t min_stride = enc->params.width * 2;
	if (!frame->data[0] || !frame->data[1] || frame->linesize[0] < min_stride || frame->linesize[1] < min_stride) {
		pr_log(enc, PR_LOG_ERRORS, LOG_ERROR,
		       "frame %lld is not a P216 frame (planes %p/%p, strides %u/%u)", (long long)frame->pts,
		       (void *)frame->data[0], (void *)frame->data[1], frame->linesize[0], frame->linesize[1]);
		return false;
	}

	const uint64_t start = os_gettime_ns();
	char err[256] = "";

	if (!check_overload(enc, frame, start))
		return false;

	/* convert into a free slot, hand it to its worker and queue it behind the frames already in flight */
	if (enc->free_count == 0) {
		pr_log(enc, PR_LOG_ERRORS, LOG_ERROR, "frame %lld: no free worker (internal error)",
		       (long long)frame->pts);
		return false;
	}
	const int in_idx = enc->free_list[--enc->free_count];
	struct pr_slot *in = enc->slots[in_idx];
	if (!enc->backend->get_planes(in->ctx, &in->planes, err, sizeof(err))) {
		enc->free_list[enc->free_count++] = in_idx;
		pr_log(enc, PR_LOG_ERRORS, LOG_ERROR, "frame %lld: %s", (long long)frame->pts,
		       err[0] ? err : "could not get frame buffers");
		return false;
	}
	if (in->planes.semi_planar) {
		/* the encoder takes P216 as OBS delivers it: copy the Y and interleaved CbCr planes row by row */
		const size_t row_bytes = (size_t)enc->params.width * 2;
		for (uint32_t y = 0; y < enc->params.height; y++) {
			memcpy(in->planes.y + (size_t)y * in->planes.y_stride, frame->data[0] + (size_t)y * frame->linesize[0],
			       row_bytes);
			memcpy(in->planes.uv + (size_t)y * in->planes.uv_stride,
			       frame->data[1] + (size_t)y * frame->linesize[1], row_bytes);
		}
	} else {
		pr_p216_to_planar10(frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1],
				    enc->params.width, enc->params.height, in->planes.y, in->planes.y_stride,
				    in->planes.cb, in->planes.cr, in->planes.c_stride);
	}
	in->pts = frame->pts;
	os_sem_post(in->start);
	enc->fifo[(enc->fifo_head + enc->pending) % PR_MAX_IN_FLIGHT] = in_idx;
	enc->pending++;

	/* until the pipeline is full there is nothing to hand back yet */
	if (enc->pending < enc->depth) {
		enc->call_ns += os_gettime_ns() - start;
		return true;
	}

	const int out_idx = enc->fifo[enc->fifo_head];
	struct pr_slot *out = enc->slots[out_idx];
	const uint64_t wait_start = os_gettime_ns();
	os_event_wait(out->done);
	const uint64_t waited = os_gettime_ns() - wait_start;
	enc->fifo_head = (enc->fifo_head + 1) % PR_MAX_IN_FLIGHT;
	enc->pending--;

	if (!out->ok) {
		enc->free_list[enc->free_count++] = out_idx;
		pr_log(enc, PR_LOG_ERRORS, LOG_ERROR, "encoding frame %lld failed: %s", (long long)out->pts,
		       out->err[0] ? out->err : "unknown error");
		return false;
	}

	/* copy out of the slot: the slot may be reused by the next submit, before OBS has read the packet */
	memcpy(enc->packet_buf, out->out, out->out_size);
	const int64_t out_pts = out->pts;
	const size_t out_size = out->out_size;
	const uint64_t out_enc_ns = out->enc_ns;
	enc->free_list[enc->free_count++] = out_idx;

	const uint64_t elapsed = os_gettime_ns() - start;
	enc->frames++;
	enc->call_ns += elapsed;
	enc->wait_ns += waited;
	enc->worker_ns += out_enc_ns;
	enc->avg_worker_ns = enc->avg_worker_ns > 0.0 ? enc->avg_worker_ns * 0.9 + (double)out_enc_ns * 0.1
						      : (double)out_enc_ns;
	if (elapsed > enc->max_call_ns)
		enc->max_call_ns = elapsed;

	maybe_grow(enc, waited);

	if (enc->frame_interval_ns && elapsed > enc->frame_interval_ns) {
		enc->over_budget++;
		/* at most one warning per second */
		if (start - enc->last_warn_ns > 1000000000ULL) {
			enc->last_warn_ns = start;
			/* informational: whether OBS actually repeated frames is reported by check_overload() */
			pr_log(enc, PR_LOG_INFO, LOG_INFO,
			       "frame %lld took %.1f ms on the OBS thread (%.1f ms waiting for a worker), longer than "
			       "the %.1f ms frame interval; OBS may skip frames. %s",
			       (long long)out_pts, (double)elapsed / 1e6, (double)waited / 1e6,
			       (double)enc->frame_interval_ns / 1e6,
			       enc->depth < enc->max_depth ? "Adding workers."
							   : "All workers busy; raise frames in flight or lower the profile.");
		}
	}
	pr_log(enc, PR_LOG_DEBUG, LOG_INFO, "frame %lld: %zu bytes, encode %.2f ms, OBS thread %.2f ms",
	       (long long)out_pts, out_size, (double)out_enc_ns / 1e6, (double)elapsed / 1e6);

	/* ProRes is intra-only: every frame is a keyframe and there is no reordering. */
	packet->type = OBS_ENCODER_VIDEO;
	packet->data = enc->packet_buf;
	packet->size = out_size;
	packet->pts = out_pts;
	packet->dts = out_pts;
	packet->keyframe = true;
	packet->priority = OBS_NAL_PRIORITY_HIGHEST;
	packet->drop_priority = OBS_NAL_PRIORITY_HIGHEST;
	packet->timebase_num = (int32_t)enc->params.fps_den;
	packet->timebase_den = (int32_t)enc->params.fps_num;

	*received_packet = true;
	return true;
}

const char *pr_encoder_get_name(void *type_data)
{
	const struct pr_backend *backend = type_data;
	return obs_module_text(backend->name_key);
}

void pr_encoder_get_defaults(obs_data_t *settings, void *type_data)
{
	const struct pr_backend *backend = type_data;

	obs_data_set_default_int(settings, S_PROFILE, PR_PROFILE_422);
	obs_data_set_default_int(settings, S_THREADS, 0);
	obs_data_set_default_int(settings, S_IN_FLIGHT, backend->default_in_flight);
	obs_data_set_default_int(settings, S_LOG_LEVEL, PR_LOG_WARNINGS);
	obs_data_set_default_int(settings, S_OVERLOAD, PR_OVERLOAD_WARN);
	obs_data_set_default_int(settings, S_SIZING, PR_SIZING_LATE);
	obs_data_set_default_int(settings, S_CODEC_TYPE, backend->prores ? profile_fourcc[PR_PROFILE_422] : 0);
	obs_data_set_default_string(settings, S_INFO, obs_module_text("Info.Text"));
	backend->set_defaults(settings);
}

obs_properties_t *pr_encoder_get_properties(void *data, void *type_data)
{
	UNUSED_PARAMETER(data);
	const struct pr_backend *backend = type_data;

	obs_properties_t *props = obs_properties_create();

	obs_property_t *info = obs_properties_add_text(props, S_INFO, obs_module_text("Info.Requirements"), OBS_TEXT_INFO);
	obs_property_text_set_info_type(info, OBS_TEXT_INFO_NORMAL);

	obs_property_t *p;
	if (backend->prores) {
		p = obs_properties_add_list(props, S_PROFILE, obs_module_text("Profile"), OBS_COMBO_TYPE_LIST,
					    OBS_COMBO_FORMAT_INT);
		obs_property_list_add_int(p, obs_module_text("Profile.Proxy"), PR_PROFILE_PROXY);
		obs_property_list_add_int(p, obs_module_text("Profile.LT"), PR_PROFILE_LT);
		obs_property_list_add_int(p, obs_module_text("Profile.422"), PR_PROFILE_422);
		obs_property_list_add_int(p, obs_module_text("Profile.HQ"), PR_PROFILE_HQ);
	}

	backend->add_properties(props);

	obs_property_t *f =
		obs_properties_add_int(props, S_IN_FLIGHT, obs_module_text("InFlight"), 0, PR_MAX_IN_FLIGHT, 1);
	obs_property_set_long_description(f, obs_module_text("InFlight.Description"));

	p = obs_properties_add_list(props, S_SIZING, obs_module_text("Sizing"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("Sizing.Late"), PR_SIZING_LATE);
	obs_property_list_add_int(p, obs_module_text("Sizing.Measured"), PR_SIZING_MEASURED);
	obs_property_set_long_description(p, obs_module_text("Sizing.Description"));

	p = obs_properties_add_list(props, S_OVERLOAD, obs_module_text("Overload"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("Overload.Warn"), PR_OVERLOAD_WARN);
	obs_property_list_add_int(p, obs_module_text("Overload.Stop"), PR_OVERLOAD_STOP);
	obs_property_set_long_description(p, obs_module_text("Overload.Description"));

	if (backend->prores) {
		obs_property_t *t = obs_properties_add_int(props, S_THREADS, obs_module_text("Threads"), 0, 64, 1);
		obs_property_set_long_description(t, obs_module_text("Threads.Description"));
	}

	p = obs_properties_add_list(props, S_LOG_LEVEL, obs_module_text("LogLevel"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("LogLevel.Errors"), PR_LOG_ERRORS);
	obs_property_list_add_int(p, obs_module_text("LogLevel.Warnings"), PR_LOG_WARNINGS);
	obs_property_list_add_int(p, obs_module_text("LogLevel.Info"), PR_LOG_INFO);
	obs_property_list_add_int(p, obs_module_text("LogLevel.Debug"), PR_LOG_DEBUG);

	return props;
}

/* obs-ffmpeg-mux silently drops all packets unless this returns true. ProRes has no codec headers (the macOS
 * VideoToolbox ProRes encoder also returns true with empty data); H.264 returns its SPS/PPS. Every worker's
 * context is set up identically, so the first one's headers stand for all. */
bool pr_encoder_get_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	static uint8_t empty;
	struct pr_encoder *enc = data;

	if (enc && enc->backend->get_extra_data && enc->created > 0)
		return enc->backend->get_extra_data(enc->slots[0]->ctx, extra_data, size);

	*extra_data = &empty;
	*size = 0;
	return true;
}

/* 10-bit codes sit in the top 10 bits of each 16-bit word, so (v + 32) >> 6 rounds to 10 bits. The saturating
 * add caps at 65535, which shifts to exactly 1023, so no separate clamp is needed. */
static inline uint16_t round10(uint16_t v)
{
	uint32_t r = ((uint32_t)v + 32u) >> 6;
	return (uint16_t)(r > 1023u ? 1023u : r);
}

void pr_p216_to_planar10(const uint8_t *src_y, size_t src_y_stride, const uint8_t *src_uv, size_t src_uv_stride,
			 uint32_t width, uint32_t height, uint16_t *dst_y, size_t dst_y_stride, uint16_t *dst_cb,
			 uint16_t *dst_cr, size_t dst_c_stride)
{
	const uint32_t cw = width / 2;

	for (uint32_t y = 0; y < height; y++) {
		const uint16_t *s = (const uint16_t *)(src_y + (size_t)y * src_y_stride);
		uint16_t *d = dst_y + (size_t)y * dst_y_stride;
		uint32_t x = 0;
#ifdef PR_HAVE_SSE2
		const __m128i bias = _mm_set1_epi16(32);
		for (; x + 8 <= width; x += 8) {
			__m128i v = _mm_loadu_si128((const __m128i *)(s + x));
			v = _mm_srli_epi16(_mm_adds_epu16(v, bias), 6);
			_mm_storeu_si128((__m128i *)(d + x), v);
		}
#endif
		for (; x < width; x++)
			d[x] = round10(s[x]);

		const uint16_t *uv = (const uint16_t *)(src_uv + (size_t)y * src_uv_stride);
		uint16_t *cb = dst_cb + (size_t)y * dst_c_stride;
		uint16_t *cr = dst_cr + (size_t)y * dst_c_stride;
		x = 0;
#ifdef PR_HAVE_SSE2
		const __m128i lo_mask = _mm_set1_epi32(0x0000FFFF);
		for (; x + 8 <= cw; x += 8) {
			/* two loads of four U,V pairs each; after the shift every 16-bit lane is <= 1023 */
			__m128i a = _mm_loadu_si128((const __m128i *)(uv + 2 * x));
			__m128i b = _mm_loadu_si128((const __m128i *)(uv + 2 * x + 8));
			a = _mm_srli_epi16(_mm_adds_epu16(a, bias), 6);
			b = _mm_srli_epi16(_mm_adds_epu16(b, bias), 6);
			__m128i u = _mm_packs_epi32(_mm_and_si128(a, lo_mask), _mm_and_si128(b, lo_mask));
			__m128i v = _mm_packs_epi32(_mm_srli_epi32(a, 16), _mm_srli_epi32(b, 16));
			_mm_storeu_si128((__m128i *)(cb + x), u);
			_mm_storeu_si128((__m128i *)(cr + x), v);
		}
#endif
		for (; x < cw; x++) {
			cb[x] = round10(uv[2 * x]);
			cr[x] = round10(uv[2 * x + 1]);
		}
	}
}
