#include "encoder-common.h"

#include <plugin-support.h>
#include <util/bmem.h>

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

#include <stdio.h>
#include <string.h>

#define S_QSCALE "qscale"
#define S_VENDOR "vendor_apl0"

struct ff_ctx {
	AVCodecContext *cc;
	AVFrame *frame;
	AVPacket *packet;
	int64_t next_pts;
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

static void *ff_create(const struct pr_params *p, char *err, size_t err_len)
{
	const AVCodec *codec = avcodec_find_encoder_by_name(p->codec_name);
	if (!codec) {
		snprintf(err, err_len, "the FFmpeg loaded by OBS has no %s encoder", p->codec_name);
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
	cc->width = (int)p->width;
	cc->height = (int)p->height;
	cc->pix_fmt = AV_PIX_FMT_YUV422P10LE;
	cc->time_base = (AVRational){(int)p->fps_den, (int)p->fps_num};
	cc->framerate = (AVRational){(int)p->fps_num, (int)p->fps_den};
	cc->profile = (int)p->profile; /* AV_PROFILE_PRORES_PROXY..HQ are 0..3 */
	cc->color_primaries = (enum AVColorPrimaries)p->color_primaries;
	cc->color_trc = (enum AVColorTransferCharacteristic)p->color_transfer;
	cc->colorspace = (enum AVColorSpace)p->color_matrix;
	cc->color_range = p->full_range ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;

	/* Parallelism comes from encoding several frames at once, one context per frame in flight. FFmpeg's own
	 * frame threading would delay packets inside the codec, and OBS never flushes video encoders. With a
	 * single frame in flight, slice threads are all there is. */
	cc->thread_count = p->in_flight > 1 ? 1 : p->threads;
	cc->thread_type = FF_THREAD_SLICE;

	if (p->qscale > 0) {
		cc->flags |= AV_CODEC_FLAG_QSCALE;
		cc->global_quality = p->qscale * FF_QP2LAMBDA;
	}
	if (p->vendor_apl0)
		av_opt_set(cc->priv_data, "vendor", "apl0", 0);

	int ret = avcodec_open2(cc, codec, NULL);
	if (ret < 0) {
		char msg[AV_ERROR_MAX_STRING_SIZE];
		snprintf(err, err_len, "avcodec_open2 failed: %s", av_make_error_string(msg, sizeof(msg), ret));
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

static bool ff_get_planes(void *ctx, struct pr_planes *planes, char *err, size_t err_len)
{
	struct ff_ctx *c = ctx;
	AVFrame *f = c->frame;

	int ret = av_frame_make_writable(f);
	if (ret < 0) {
		char msg[AV_ERROR_MAX_STRING_SIZE];
		snprintf(err, err_len, "av_frame_make_writable: %s", av_make_error_string(msg, sizeof(msg), ret));
		return false;
	}
	/* the converter writes Cb and Cr with one stride; AVFrame allocates them alike */
	if (f->linesize[1] != f->linesize[2]) {
		snprintf(err, err_len, "unexpected chroma strides %d/%d", f->linesize[1], f->linesize[2]);
		return false;
	}

	planes->y = (uint16_t *)f->data[0];
	planes->cb = (uint16_t *)f->data[1];
	planes->cr = (uint16_t *)f->data[2];
	planes->y_stride = (size_t)f->linesize[0] / 2;
	planes->c_stride = (size_t)f->linesize[1] / 2;
	return true;
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

static void ks_add_properties(obs_properties_t *props)
{
	obs_property_t *q = obs_properties_add_int(props, S_QSCALE, obs_module_text("QScale"), 0, 31, 1);
	obs_property_set_long_description(q, obs_module_text("QScale.Description"));
	obs_properties_add_bool(props, S_VENDOR, obs_module_text("Vendor.Apl0"));
}

static void ks_set_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_QSCALE, 0);
	obs_data_set_default_bool(settings, S_VENDOR, true);
}

/* prores_aw has no quality-scale control. */
static void aw_add_properties(obs_properties_t *props)
{
	obs_properties_add_bool(props, S_VENDOR, obs_module_text("Vendor.Apl0"));
}

static void aw_set_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, S_VENDOR, true);
}

static const struct pr_backend ffmpeg_ks_backend = {
	.name_key = "Encoder.FFmpeg",
	.log_name = "ffmpeg-ks",
	.codec_name = "prores_ks",
	.default_in_flight = 0, /* automatic */
	.create = ff_create,
	.get_planes = ff_get_planes,
	.encode = ff_encode,
	.destroy = ff_destroy,
	.add_properties = ks_add_properties,
	.set_defaults = ks_set_defaults,
};

static const struct pr_backend ffmpeg_aw_backend = {
	.name_key = "Encoder.FFmpegAw",
	.log_name = "ffmpeg-aw",
	.codec_name = "prores_aw",
	.default_in_flight = 0, /* automatic */
	.create = ff_create,
	.get_planes = ff_get_planes,
	.encode = ff_encode,
	.destroy = ff_destroy,
	.add_properties = aw_add_properties,
	.set_defaults = aw_set_defaults,
};

static void *ffmpeg_ks_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return pr_encoder_create(&ffmpeg_ks_backend, settings, encoder);
}

static void *ffmpeg_aw_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return pr_encoder_create(&ffmpeg_aw_backend, settings, encoder);
}

struct obs_encoder_info pr_ffmpeg_encoder_info = {
	.id = "obs_prores_ffmpeg",
	.type = OBS_ENCODER_VIDEO,
	.codec = "prores",
	.get_name = pr_encoder_get_name,
	.create = ffmpeg_ks_create,
	.destroy = pr_encoder_destroy,
	.encode = pr_encoder_encode,
	.update = pr_encoder_update,
	.get_extra_data = pr_encoder_get_extra_data,
	.get_defaults2 = pr_encoder_get_defaults,
	.get_properties2 = pr_encoder_get_properties,
	.type_data = (void *)&ffmpeg_ks_backend,
};

struct obs_encoder_info pr_ffmpeg_aw_encoder_info = {
	.id = "obs_prores_ffmpeg_aw",
	.type = OBS_ENCODER_VIDEO,
	.codec = "prores",
	.get_name = pr_encoder_get_name,
	.create = ffmpeg_aw_create,
	.destroy = pr_encoder_destroy,
	.encode = pr_encoder_encode,
	.update = pr_encoder_update,
	.get_extra_data = pr_encoder_get_extra_data,
	.get_defaults2 = pr_encoder_get_defaults,
	.get_properties2 = pr_encoder_get_properties,
	.type_data = (void *)&ffmpeg_aw_backend,
};
