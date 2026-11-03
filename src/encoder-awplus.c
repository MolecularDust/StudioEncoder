#include "encoder-common.h"
#include "prores-aw.h"

#include <plugin-support.h>

#include <util/bmem.h>

#include <stdio.h>

/* AW+: FFmpeg's prores_aw encoder ported into the plugin (prores-aw.c) with coefficient rounding instead of
 * truncation, so ProRes Proxy keeps fine detail. One frame is encoded on one thread; speed comes from the
 * frame-parallel pipeline in encoder-common.c. */

#define S_VENDOR "vendor_apl0"

struct awp_ctx {
	struct pa_ctx *pa;
	uint32_t width;
	uint16_t *y;
	uint16_t *cb;
	uint16_t *cr;
};

static void awp_destroy(void *ctx)
{
	struct awp_ctx *c = ctx;
	if (!c)
		return;
	pa_destroy(c->pa);
	bfree(c->y);
	bfree(c->cb);
	bfree(c->cr);
	bfree(c);
}

static void *awp_create(const struct pr_params *p, char *err, size_t err_len)
{
	struct awp_ctx *c = bzalloc(sizeof(*c));
	c->width = p->width;
	c->pa = pa_create((int)p->width, (int)p->height, (int)p->profile, pa_default_round8((int)p->profile),
			  p->vendor_apl0 ? "apl0" : "fmpg", p->color_primaries, p->color_transfer, p->color_matrix);
	if (!c->pa) {
		snprintf(err, err_len, "unsupported frame size %ux%u or profile", p->width, p->height);
		awp_destroy(c);
		return NULL;
	}
	static bool logged = false;
	if (!logged) {
		obs_log(LOG_INFO, "aw+: DCT uses %s", pa_simd_active() ? "AVX2" : "plain C (no AVX2 on this CPU)");
		logged = true;
	}
	const size_t luma = (size_t)p->width * p->height;
	const size_t chroma = (size_t)(p->width / 2) * p->height;
	c->y = bmalloc(luma * sizeof(uint16_t));
	c->cb = bmalloc(chroma * sizeof(uint16_t));
	c->cr = bmalloc(chroma * sizeof(uint16_t));
	return c;
}

static bool awp_get_planes(void *ctx, struct pr_planes *planes, char *err, size_t err_len)
{
	struct awp_ctx *c = ctx;
	UNUSED_PARAMETER(err);
	UNUSED_PARAMETER(err_len);
	planes->semi_planar = false;
	planes->y = c->y;
	planes->cb = c->cb;
	planes->cr = c->cr;
	planes->y_stride = c->width;
	planes->c_stride = c->width / 2;
	return true;
}

static bool awp_encode(void *ctx, uint8_t *out, size_t out_cap, size_t *out_size, char *err, size_t err_len)
{
	struct awp_ctx *c = ctx;
	const int64_t n = pa_encode(c->pa, c->y, c->width, c->cb, c->cr, c->width / 2, out, out_cap);
	if (n < 0) {
		snprintf(err, err_len, "encoded frame larger than the %zu-byte buffer", out_cap);
		return false;
	}
	*out_size = (size_t)n;
	return true;
}

static void awp_add_properties(obs_properties_t *props)
{
	obs_properties_add_bool(props, S_VENDOR, obs_module_text("Vendor.Apl0"));
}

static void awp_set_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, S_VENDOR, true);
}

static const struct pr_backend awplus_backend = {
	.name_key = "Encoder.AwPlus",
	.log_name = "aw+",
	.codec_name = NULL,
	.default_in_flight = 0, /* automatic */
	.prores = true,
	.create = awp_create,
	.get_planes = awp_get_planes,
	.encode = awp_encode,
	.destroy = awp_destroy,
	.add_properties = awp_add_properties,
	.set_defaults = awp_set_defaults,
};

static void *awplus_encoder_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return pr_encoder_create(&awplus_backend, settings, encoder);
}

/* Takes over the ID the FFmpeg prores_aw encoder had, so profiles that used it get the fixed encoder. */
struct obs_encoder_info pr_awplus_encoder_info = {
	.id = "obs_prores_ffmpeg_aw",
	.type = OBS_ENCODER_VIDEO,
	.codec = "prores",
	.get_name = pr_encoder_get_name,
	.create = awplus_encoder_create,
	.destroy = pr_encoder_destroy,
	.encode = pr_encoder_encode,
	.update = pr_encoder_update,
	.get_extra_data = pr_encoder_get_extra_data,
	.get_defaults2 = pr_encoder_get_defaults,
	.get_properties2 = pr_encoder_get_properties,
	.type_data = (void *)&awplus_backend,
};
