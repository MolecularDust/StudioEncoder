#include "encoder-common.h"

#include <plugin-support.h>
#include <util/bmem.h>

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

#include <stdio.h>
#include <string.h>

struct ff_ctx {
	AVCodecContext *cc;
	AVFrame *frame;
	AVPacket *packet;
	int64_t next_pts;
	/* AMD only: the hardware returns each packet one frame late, so OBS's timestamps wait here */
	struct pr_planes *planes;
	int64_t pts_queue[4];
	int queued;
};

static void ff_destroy(void *ctx)
{
	struct ff_ctx *c = ctx;
	if (!c)
		return;
	av_packet_free(&c->packet);
	av_frame_free(&c->frame);
	avcodec_free_context(&c->cc);
	bfree(c);
}

static bool ff_encode(void *ctx, uint8_t *out, size_t out_cap, size_t *out_size, char *err, size_t err_len)
{
	struct ff_ctx *c = ctx;
	char msg[AV_ERROR_MAX_STRING_SIZE];

	c->frame->pts = c->next_pts++;
	int ret = avcodec_send_frame(c->cc, c->frame);
	if (ret < 0) {
		snprintf(err, err_len, "avcodec_send_frame: %s", av_make_error_string(msg, sizeof(msg), ret));
		return false;
	}

	ret = avcodec_receive_packet(c->cc, c->packet);
	if (ret < 0) {
		snprintf(err, err_len, "avcodec_receive_packet: %s", av_make_error_string(msg, sizeof(msg), ret));
		return false;
	}

	if ((size_t)c->packet->size > out_cap) {
		snprintf(err, err_len, "packet of %d bytes exceeds buffer of %zu", c->packet->size, out_cap);
		av_packet_unref(c->packet);
		return false;
	}
	memcpy(out, c->packet->data, (size_t)c->packet->size);
	*out_size = (size_t)c->packet->size;
	av_packet_unref(c->packet);
	return true;
}

/* ---- H.264 10-bit 4:2:2 all-intra on NVIDIA NVENC (RTX 50 series and newer) ---- */

#define S_NV_BITRATE "bitrate_mbps"
#define S_NV_PRESET "nvenc_preset"

static void *nv_create(const struct pr_params *p, char *err, size_t err_len)
{
	const AVCodec *codec = avcodec_find_encoder_by_name("h264_nvenc");
	if (!codec) {
		snprintf(err, err_len, "the FFmpeg loaded by OBS has no h264_nvenc encoder");
		return NULL;
	}

	struct ff_ctx *c = bzalloc(sizeof(*c));
	c->cc = avcodec_alloc_context3(codec);
	c->frame = av_frame_alloc();
	c->packet = av_packet_alloc();
	if (!c->cc || !c->frame || !c->packet) {
		snprintf(err, err_len, "out of memory");
		goto fail;
	}

	AVCodecContext *cc = c->cc;
	/* P216 is exactly what OBS outputs, so frames go to the GPU without conversion */
	cc->pix_fmt = AV_PIX_FMT_P216LE;
	cc->width = (int)p->width;
	cc->height = (int)p->height;
	cc->time_base = (AVRational){(int)p->fps_den, (int)p->fps_num};
	cc->framerate = (AVRational){(int)p->fps_num, (int)p->fps_den};
	cc->color_primaries = (enum AVColorPrimaries)p->color_primaries;
	cc->color_trc = (enum AVColorTransferCharacteristic)p->color_transfer;
	cc->colorspace = (enum AVColorSpace)p->color_matrix;
	cc->color_range = p->full_range ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;

	/* all-intra: every frame an IDR frame, no B-frames. gop_size 0 is NVENC's explicit intra-only mode;
	 * with gop_size 1 FFmpeg 8.0.1 keeps the preset's B-frame interval and NVENC rejects the setup. */
	cc->gop_size = 0;
	cc->max_b_frames = 0;

	const int64_t bitrate = (int64_t)(p->bitrate_mbps > 0 ? p->bitrate_mbps : 200) * 1000000;
	cc->bit_rate = bitrate;
	cc->rc_max_rate = bitrate;
	cc->rc_buffer_size = (int)bitrate;

	/* SPS/PPS go to extradata for the MOV header instead of being repeated in every packet */
	cc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	/* FFmpeg builds older than about 7.1 (common in Linux distributions) don't know this profile */
	if (av_opt_set(cc->priv_data, "profile", "high422", 0) < 0) {
		snprintf(err, err_len, "the FFmpeg loaded by OBS is too old for H.264 4:2:2 on NVENC (no high422 profile)");
		goto fail;
	}
	av_opt_set(cc->priv_data, "preset", p->nvenc_fast ? "p1" : "p5", 0);
	/* "hq" tuning refuses a GOP of 1 at p4-p7 ("Gop Length should be greater than number of B frames + 1") */
	av_opt_set(cc->priv_data, "tune", "ll", 0);
	av_opt_set(cc->priv_data, "rc", "cbr", 0);
	/* return each packet as soon as its frame is encoded: OBS never flushes video encoders */
	av_opt_set_int(cc->priv_data, "delay", 0, 0);

	int ret = avcodec_open2(cc, codec, NULL);
	if (ret < 0) {
		char msg[AV_ERROR_MAX_STRING_SIZE];
		snprintf(err, err_len,
			 "NVENC could not start H.264 10-bit 4:2:2 (%s). This encoder needs an NVIDIA RTX 50-series GPU "
			 "and a current driver; on other GPUs use ProRes (FFmpeg prores_aw, fast).",
			 av_make_error_string(msg, sizeof(msg), ret));
		goto fail;
	}

	c->frame->format = cc->pix_fmt;
	c->frame->width = cc->width;
	c->frame->height = cc->height;
	if (av_frame_get_buffer(c->frame, 0) < 0) {
		snprintf(err, err_len, "could not allocate frame buffers");
		goto fail;
	}
	return c;

fail:
	ff_destroy(c);
	return NULL;
}

static bool nv_get_planes(void *ctx, struct pr_planes *planes, char *err, size_t err_len)
{
	struct ff_ctx *c = ctx;
	AVFrame *f = c->frame;

	int ret = av_frame_make_writable(f);
	if (ret < 0) {
		char msg[AV_ERROR_MAX_STRING_SIZE];
		snprintf(err, err_len, "av_frame_make_writable: %s", av_make_error_string(msg, sizeof(msg), ret));
		return false;
	}

	planes->semi_planar = true;
	planes->y = (uint16_t *)f->data[0];
	planes->uv = (uint16_t *)f->data[1];
	planes->y_stride = (size_t)f->linesize[0] / 2;
	planes->uv_stride = (size_t)f->linesize[1] / 2;
	planes->cb = planes->cr = NULL;
	planes->c_stride = 0;
	return true;
}

static bool nv_get_extra_data(void *ctx, uint8_t **extra_data, size_t *size)
{
	struct ff_ctx *c = ctx;
	if (!c || !c->cc || !c->cc->extradata || c->cc->extradata_size <= 0)
		return false;
	*extra_data = c->cc->extradata;
	*size = (size_t)c->cc->extradata_size;
	return true;
}

static void nv_add_properties(obs_properties_t *props)
{
	obs_property_t *b = obs_properties_add_int(props, S_NV_BITRATE, obs_module_text("NvBitrate"), 50, 400, 10);
	obs_property_int_set_suffix(b, " Mbit/s");
	obs_property_set_long_description(b, obs_module_text("NvBitrate.Description"));

	obs_property_t *p = obs_properties_add_list(props, S_NV_PRESET, obs_module_text("NvPreset"), OBS_COMBO_TYPE_LIST,
						    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("NvPreset.Quality"), 0);
	obs_property_list_add_int(p, obs_module_text("NvPreset.Fast"), 1);
}

static void nv_set_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_NV_BITRATE, 200);
	obs_data_set_default_int(settings, S_NV_PRESET, 0);
}

static const struct pr_backend nvenc_h264_backend = {
	.name_key = "Encoder.NvencH264",
	.log_name = "nvenc-h264",
	.codec_name = "h264_nvenc",
	.default_in_flight = 0, /* automatic */
	.max_in_flight = 2,     /* each frame in flight opens an NVENC session */
	.prores = false,
	.create = nv_create,
	.get_planes = nv_get_planes,
	.encode = ff_encode,
	.destroy = ff_destroy,
	.get_extra_data = nv_get_extra_data,
	.add_properties = nv_add_properties,
	.set_defaults = nv_set_defaults,
};

static void *nvenc_h264_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return pr_encoder_create(&nvenc_h264_backend, settings, encoder);
}

struct obs_encoder_info pr_nvenc_h264_encoder_info = {
	.id = "obs_prores_nvenc_h264_422_intra",
	.type = OBS_ENCODER_VIDEO,
	.codec = "h264",
	.get_name = pr_encoder_get_name,
	.create = nvenc_h264_create,
	.destroy = pr_encoder_destroy,
	.encode = pr_encoder_encode,
	.update = pr_encoder_update,
	.get_extra_data = pr_encoder_get_extra_data,
	.get_defaults2 = pr_encoder_get_defaults,
	.get_properties2 = pr_encoder_get_properties,
	.type_data = (void *)&nvenc_h264_backend,
};

/* Asks the GPU driver whether it can start the H.264 10-bit 4:2:2 NVENC mode, by opening an encoder on a small
 * frame size and closing it again. Used once at start-up to decide whether the encoder is offered. */
bool pr_nvenc_h264_422_probe(char *reason, size_t reason_len)
{
	const AVCodec *codec = avcodec_find_encoder_by_name("h264_nvenc");
	if (!codec) {
		snprintf(reason, reason_len, "the FFmpeg loaded by OBS has no h264_nvenc encoder");
		return false;
	}

	AVCodecContext *cc = avcodec_alloc_context3(codec);
	if (!cc) {
		snprintf(reason, reason_len, "out of memory");
		return false;
	}

	cc->pix_fmt = AV_PIX_FMT_P216LE;
	cc->width = 256;
	cc->height = 256;
	cc->time_base = (AVRational){1, 30};
	cc->framerate = (AVRational){30, 1};
	cc->gop_size = 0; /* intra-only, see above */
	cc->max_b_frames = 0;
	cc->bit_rate = 20000000;
	cc->rc_max_rate = 20000000;
	cc->rc_buffer_size = 20000000;
	cc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	if (av_opt_set(cc->priv_data, "profile", "high422", 0) < 0) {
		snprintf(reason, reason_len, "this FFmpeg build has no high422 profile for h264_nvenc (it must be built with nv-codec-headers 13.0 or newer)");
		avcodec_free_context(&cc);
		return false;
	}
	av_opt_set(cc->priv_data, "preset", "p1", 0);
	av_opt_set(cc->priv_data, "tune", "ll", 0);
	av_opt_set(cc->priv_data, "rc", "cbr", 0);
	av_opt_set_int(cc->priv_data, "delay", 0, 0);

	const int ret = avcodec_open2(cc, codec, NULL);
	if (ret < 0) {
		char msg[AV_ERROR_MAX_STRING_SIZE];
		snprintf(reason, reason_len, "%s", av_make_error_string(msg, sizeof(msg), ret));
		avcodec_free_context(&cc);
		return false;
	}
	avcodec_free_context(&cc);
	return true;
}

/* ---- HEVC 10-bit 4:2:0 all-intra on AMD (AMF, any Radeon with a VCN encoder) ---- */

#define S_AMD_QUALITY "amd_quality"

/* Constant-quality presets. Every frame is a keyframe, so the quantiser sets the file size. Typical 4K 29.97 camera
 * footage (measured on an RX 9070 XT): Small about 28 Mbit/s, Standard about 68, High about 116, Maximum about 170.
 * Noisy footage makes bigger files at the same quantiser. */
static const int amd_qp[] = {22, 14, 10, 6};
static const char *const amd_quality_names[] = {"Small", "Standard", "High", "Maximum"};

static int amd_qp_for(int quality)
{
	if (quality < 0 || quality > 3)
		quality = 2;
	return amd_qp[quality];
}

/* Opens hevc_amf for 10-bit 4:2:0 intra-only on the given frame size. Shared by the encoder and the start-up check. */
static AVCodecContext *amd_open(int width, int height, AVRational fps, int quality, const struct pr_params *p,
				char *err, size_t err_len)
{
	const AVCodec *codec = avcodec_find_encoder_by_name("hevc_amf");
	if (!codec) {
		snprintf(err, err_len, "the FFmpeg loaded by OBS has no hevc_amf encoder");
		return NULL;
	}
	AVCodecContext *cc = avcodec_alloc_context3(codec);
	if (!cc) {
		snprintf(err, err_len, "out of memory");
		return NULL;
	}

	/* P010 is what OBS outputs, so frames go to the encoder without conversion */
	cc->pix_fmt = AV_PIX_FMT_P010LE;
	cc->width = width;
	cc->height = height;
	cc->time_base = (AVRational){fps.den, fps.num};
	cc->framerate = fps;
	if (p) {
		cc->color_primaries = (enum AVColorPrimaries)p->color_primaries;
		cc->color_trc = (enum AVColorTransferCharacteristic)p->color_transfer;
		cc->colorspace = (enum AVColorSpace)p->color_matrix;
		cc->color_range = p->full_range ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
	}

	/* all-intra: a GOP of one frame, so every frame is an IDR frame; no B-frames */
	cc->gop_size = 1;
	cc->max_b_frames = 0;
	/* VPS/SPS/PPS go to extradata for the MOV header */
	cc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	const int qp = amd_qp_for(quality);
	/* without this, AMF writes profile Main into a 10-bit stream when started from here */
	av_opt_set(cc->priv_data, "profile", "main10", 0);
	av_opt_set(cc->priv_data, "rc", "cqp", 0);
	av_opt_set_int(cc->priv_data, "qp_i", qp, 0);
	av_opt_set_int(cc->priv_data, "qp_p", qp, 0);
	av_opt_set(cc->priv_data, "quality", "quality", 0);

	const int ret = avcodec_open2(cc, codec, NULL);
	if (ret < 0) {
		char msg[AV_ERROR_MAX_STRING_SIZE];
		snprintf(err, err_len, "%s", av_make_error_string(msg, sizeof(msg), ret));
		avcodec_free_context(&cc);
		return NULL;
	}
	return cc;
}

static void *amd_create(const struct pr_params *p, char *err, size_t err_len)
{
	char why[200] = "";
	struct ff_ctx *c = bzalloc(sizeof(*c));
	c->frame = av_frame_alloc();
	c->packet = av_packet_alloc();
	if (!c->frame || !c->packet) {
		snprintf(err, err_len, "out of memory");
		goto fail;
	}

	c->cc = amd_open((int)p->width, (int)p->height, (AVRational){(int)p->fps_num, (int)p->fps_den}, p->amd_quality, p,
			 why, sizeof(why));
	if (!c->cc) {
		snprintf(err, err_len,
			 "the AMD encoder could not start HEVC 10-bit (%s). This encoder needs a Radeon graphics card "
			 "with a current driver.",
			 why);
		goto fail;
	}

	c->frame->format = c->cc->pix_fmt;
	c->frame->width = c->cc->width;
	c->frame->height = c->cc->height;
	if (av_frame_get_buffer(c->frame, 0) < 0) {
		snprintf(err, err_len, "could not allocate frame buffers");
		goto fail;
	}
	return c;

fail:
	ff_destroy(c);
	return NULL;
}

static bool amd_get_planes(void *ctx, struct pr_planes *planes, char *err, size_t err_len)
{
	struct ff_ctx *c = ctx;
	c->planes = planes; /* the planes live in the slot, which outlives this context's use */
	return nv_get_planes(ctx, planes, err, err_len);
}

/* AMF returns the packet for a frame only after the next frame went in, so the first call has no packet. The
 * timestamps of the frames waiting inside the encoder are kept in order and handed back with each packet. */
static bool amd_encode(void *ctx, uint8_t *out, size_t out_cap, size_t *out_size, char *err, size_t err_len)
{
	struct ff_ctx *c = ctx;
	char msg[AV_ERROR_MAX_STRING_SIZE];

	if (c->queued >= (int)(sizeof(c->pts_queue) / sizeof(c->pts_queue[0]))) {
		snprintf(err, err_len, "the AMD encoder is holding back more frames than expected");
		return false;
	}
	c->pts_queue[c->queued++] = c->planes->pts;

	c->frame->pts = c->next_pts++;
	int ret = avcodec_send_frame(c->cc, c->frame);
	if (ret < 0) {
		snprintf(err, err_len, "avcodec_send_frame: %s", av_make_error_string(msg, sizeof(msg), ret));
		return false;
	}

	ret = avcodec_receive_packet(c->cc, c->packet);
	if (ret == AVERROR(EAGAIN)) {
		*out_size = 0;
		return true;
	}
	if (ret < 0) {
		snprintf(err, err_len, "avcodec_receive_packet: %s", av_make_error_string(msg, sizeof(msg), ret));
		return false;
	}

	if ((size_t)c->packet->size > out_cap) {
		snprintf(err, err_len, "packet of %d bytes exceeds buffer of %zu", c->packet->size, out_cap);
		av_packet_unref(c->packet);
		return false;
	}
	memcpy(out, c->packet->data, (size_t)c->packet->size);
	*out_size = (size_t)c->packet->size;
	av_packet_unref(c->packet);

	c->planes->out_pts = c->pts_queue[0];
	c->planes->out_pts_valid = true;
	for (int i = 1; i < c->queued; i++)
		c->pts_queue[i - 1] = c->pts_queue[i];
	c->queued--;
	return true;
}

static void amd_add_properties(obs_properties_t *props)
{
	obs_property_t *q = obs_properties_add_list(props, S_AMD_QUALITY, obs_module_text("AmdQuality"),
						    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(q, obs_module_text("AmdQuality.Small"), 0);
	obs_property_list_add_int(q, obs_module_text("AmdQuality.Standard"), 1);
	obs_property_list_add_int(q, obs_module_text("AmdQuality.High"), 2);
	obs_property_list_add_int(q, obs_module_text("AmdQuality.Maximum"), 3);
	obs_property_set_long_description(q, obs_module_text("AmdQuality.Description"));
}

static void amd_set_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_AMD_QUALITY, 2);
}

static void amd_describe(const struct pr_params *p, char *buf, size_t len)
{
	const int q = p->amd_quality < 0 || p->amd_quality > 3 ? 2 : p->amd_quality;
	snprintf(buf, len, "%s (QP %d)", amd_quality_names[q], amd_qp[q]);
}

static const struct pr_backend amd_hevc_backend = {
	.name_key = "Encoder.AmdHevc",
	.log_name = "amd-hevc",
	.codec_name = "hevc_amf",
	.default_in_flight = 0, /* automatic */
	.max_in_flight = 1,     /* one encoder, so the frame held back inside it stays in order; fast enough at 4K */
	.prores = false,
	.input_p010 = true,
	.describe = amd_describe,
	.create = amd_create,
	.get_planes = amd_get_planes, /* semi-planar Y + interleaved CbCr, like NVENC's P216 */
	.encode = amd_encode,
	.destroy = ff_destroy,
	.get_extra_data = nv_get_extra_data,
	.add_properties = amd_add_properties,
	.set_defaults = amd_set_defaults,
};

static void *amd_hevc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return pr_encoder_create(&amd_hevc_backend, settings, encoder);
}

struct obs_encoder_info pr_amd_hevc_encoder_info = {
	.id = "obs_prores_amd_hevc_intra",
	.type = OBS_ENCODER_VIDEO,
	.codec = "hevc",
	.get_name = pr_encoder_get_name,
	.create = amd_hevc_create,
	.destroy = pr_encoder_destroy,
	.encode = pr_encoder_encode,
	.update = pr_encoder_update,
	.get_extra_data = pr_encoder_get_extra_data,
	.get_defaults2 = pr_encoder_get_defaults,
	.get_properties2 = pr_encoder_get_properties,
	.type_data = (void *)&amd_hevc_backend,
};

/* Asks the GPU driver whether it can start the HEVC 10-bit AMF mode, by opening an encoder on a small frame size and
 * closing it again. Used once at start-up to decide whether the encoder is offered. */
bool pr_amd_hevc_probe(char *reason, size_t reason_len)
{
	/* AMF refuses very small frames (256x256 fails), so ask with a normal small size */
	AVCodecContext *cc = amd_open(640, 360, (AVRational){30, 1}, 2, NULL, reason, reason_len);
	if (!cc)
		return false;
	avcodec_free_context(&cc);
	return true;
}

/* ---- HEVC 10-bit 4:2:0 all-intra on NVIDIA NVENC (any GeForce with 10-bit HEVC encoding: GTX 10 series and newer) ----
 *
 * For NVIDIA cards older than the RTX 50 series, which have no H.264 10-bit or 4:2:2 mode. P010 comes straight from
 * OBS (Settings > Advanced > Color Format P010), like the AMD encoder. NVENC returns each packet right away, so it
 * runs through the same simple path as the H.264 NVENC encoder. */

/* Opens hevc_nvenc for 10-bit 4:2:0 intra-only on the given frame size. Shared by the encoder and the start-up check. */
static AVCodecContext *nvh_open(int width, int height, AVRational fps, int bitrate_mbps, bool fast,
				const struct pr_params *p, char *err, size_t err_len)
{
	const AVCodec *codec = avcodec_find_encoder_by_name("hevc_nvenc");
	if (!codec) {
		snprintf(err, err_len, "the FFmpeg loaded by OBS has no hevc_nvenc encoder");
		return NULL;
	}
	AVCodecContext *cc = avcodec_alloc_context3(codec);
	if (!cc) {
		snprintf(err, err_len, "out of memory");
		return NULL;
	}

	cc->pix_fmt = AV_PIX_FMT_P010LE; /* what OBS outputs with Color Format P010: no conversion */
	cc->width = width;
	cc->height = height;
	cc->time_base = (AVRational){fps.den, fps.num};
	cc->framerate = fps;
	if (p) {
		cc->color_primaries = (enum AVColorPrimaries)p->color_primaries;
		cc->color_trc = (enum AVColorTransferCharacteristic)p->color_transfer;
		cc->colorspace = (enum AVColorSpace)p->color_matrix;
		cc->color_range = p->full_range ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
	}

	/* all-intra: gop_size 0 is NVENC's explicit intra-only mode (see nv_create) */
	cc->gop_size = 0;
	cc->max_b_frames = 0;

	const int64_t bitrate = (int64_t)(bitrate_mbps > 0 ? bitrate_mbps : 150) * 1000000;
	cc->bit_rate = bitrate;
	cc->rc_max_rate = bitrate;
	cc->rc_buffer_size = (int)bitrate;

	/* VPS/SPS/PPS go to extradata for the MOV header */
	cc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	av_opt_set(cc->priv_data, "profile", "main10", 0);
	av_opt_set(cc->priv_data, "preset", fast ? "p1" : "p5", 0);
	av_opt_set(cc->priv_data, "tune", "ll", 0);
	av_opt_set(cc->priv_data, "rc", "cbr", 0);
	/* return each packet as soon as its frame is encoded: OBS never flushes video encoders */
	av_opt_set_int(cc->priv_data, "delay", 0, 0);

	const int ret = avcodec_open2(cc, codec, NULL);
	if (ret < 0) {
		char msg[AV_ERROR_MAX_STRING_SIZE];
		snprintf(err, err_len, "%s", av_make_error_string(msg, sizeof(msg), ret));
		avcodec_free_context(&cc);
		return NULL;
	}
	return cc;
}

static void *nvh_create(const struct pr_params *p, char *err, size_t err_len)
{
	char why[200] = "";
	struct ff_ctx *c = bzalloc(sizeof(*c));
	c->frame = av_frame_alloc();
	c->packet = av_packet_alloc();
	if (!c->frame || !c->packet) {
		snprintf(err, err_len, "out of memory");
		goto fail;
	}

	c->cc = nvh_open((int)p->width, (int)p->height, (AVRational){(int)p->fps_num, (int)p->fps_den}, p->bitrate_mbps,
			 p->nvenc_fast, p, why, sizeof(why));
	if (!c->cc) {
		snprintf(err, err_len,
			 "NVENC could not start HEVC 10-bit (%s). This encoder needs an NVIDIA GeForce card with 10-bit "
			 "HEVC encoding (GTX 10 series or newer) and a current driver.",
			 why);
		goto fail;
	}

	c->frame->format = c->cc->pix_fmt;
	c->frame->width = c->cc->width;
	c->frame->height = c->cc->height;
	if (av_frame_get_buffer(c->frame, 0) < 0) {
		snprintf(err, err_len, "could not allocate frame buffers");
		goto fail;
	}
	return c;

fail:
	ff_destroy(c);
	return NULL;
}

static void nvh_add_properties(obs_properties_t *props)
{
	obs_property_t *b = obs_properties_add_int(props, S_NV_BITRATE, obs_module_text("NvBitrate"), 30, 400, 10);
	obs_property_int_set_suffix(b, " Mbit/s");
	obs_property_set_long_description(b, obs_module_text("NvHevcBitrate.Description"));

	obs_property_t *p = obs_properties_add_list(props, S_NV_PRESET, obs_module_text("NvPreset"), OBS_COMBO_TYPE_LIST,
						    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("NvPreset.Quality"), 0);
	obs_property_list_add_int(p, obs_module_text("NvPreset.Fast"), 1);
}

static void nvh_set_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_NV_BITRATE, 150);
	obs_data_set_default_int(settings, S_NV_PRESET, 0);
}

static const struct pr_backend nvenc_hevc_backend = {
	.name_key = "Encoder.NvencHevc",
	.log_name = "nvenc-hevc",
	.codec_name = "hevc_nvenc",
	.default_in_flight = 0, /* automatic */
	.max_in_flight = 2,     /* each frame in flight opens an NVENC session */
	.prores = false,
	.input_p010 = true,
	.container_tag = FOURCC('h', 'v', 'c', '1'),
	.create = nvh_create,
	.get_planes = nv_get_planes, /* semi-planar Y + interleaved CbCr, like the other NVENC encoder */
	.encode = ff_encode,
	.destroy = ff_destroy,
	.get_extra_data = nv_get_extra_data,
	.add_properties = nvh_add_properties,
	.set_defaults = nvh_set_defaults,
};

static void *nvenc_hevc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return pr_encoder_create(&nvenc_hevc_backend, settings, encoder);
}

struct obs_encoder_info pr_nvenc_hevc_encoder_info = {
	.id = "obs_prores_nvenc_hevc_intra",
	.type = OBS_ENCODER_VIDEO,
	.codec = "hevc",
	.get_name = pr_encoder_get_name,
	.create = nvenc_hevc_create,
	.destroy = pr_encoder_destroy,
	.encode = pr_encoder_encode,
	.update = pr_encoder_update,
	.get_extra_data = pr_encoder_get_extra_data,
	.get_defaults2 = pr_encoder_get_defaults,
	.get_properties2 = pr_encoder_get_properties,
	.type_data = (void *)&nvenc_hevc_backend,
};

/* Asks the GPU driver whether it can start HEVC 10-bit on NVENC, by opening an encoder on a small frame and closing
 * it again. Used once at start-up to decide whether the encoder is offered. */
bool pr_nvenc_hevc_probe(char *reason, size_t reason_len)
{
	AVCodecContext *cc = nvh_open(640, 360, (AVRational){30, 1}, 20, true, NULL, reason, reason_len);
	if (!cc)
		return false;
	avcodec_free_context(&cc);
	return true;
}
