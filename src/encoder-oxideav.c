#include "encoder-common.h"
#include "prores-rs.h"

#include <util/bmem.h>

#include <stdio.h>

/* ProRes encoder from the OxideAV project (oxideav-prores, pure Rust). One frame is encoded on one thread,
 * so speed comes from the frame-parallel pipeline in encoder-common.c. */

struct ox_ctx {
	void *handle;
	uint32_t width;
	uint16_t *y;
	uint16_t *cb;
	uint16_t *cr;
};

static void ox_ctx_destroy(void *ctx)
{
	struct ox_ctx *c = ctx;
	if (!c)
		return;
	if (c->handle)
		ox_destroy(c->handle);
	bfree(c->y);
	bfree(c->cb);
	bfree(c->cr);
	bfree(c);
}

static void *ox_ctx_create(const struct pr_params *p, char *err, size_t err_len)
{
	struct ox_ctx *c = bzalloc(sizeof(*c));
	c->width = p->width;

	c->handle = ox_create(p->width, p->height, (int)p->profile, p->fps_num, p->fps_den, p->color_primaries,
			      p->color_transfer, p->color_matrix);
	if (!c->handle) {
		snprintf(err, err_len, "%s", pr_last_error());
		ox_ctx_destroy(c);
		return NULL;
	}

	const size_t luma = (size_t)p->width * p->height;
	const size_t chroma = (size_t)(p->width / 2) * p->height;
	c->y = bmalloc(luma * sizeof(uint16_t));
	c->cb = bmalloc(chroma * sizeof(uint16_t));
	c->cr = bmalloc(chroma * sizeof(uint16_t));
	return c;
}

static bool ox_ctx_get_planes(void *ctx, struct pr_planes *planes, char *err, size_t err_len)
{
	struct ox_ctx *c = ctx;
	UNUSED_PARAMETER(err);
	UNUSED_PARAMETER(err_len);

	planes->y = c->y;
	planes->cb = c->cb;
	planes->cr = c->cr;
	planes->y_stride = c->width;
	planes->c_stride = c->width / 2;
	return true;
}

static bool ox_ctx_encode(void *ctx, uint8_t *out, size_t out_cap, size_t *out_size, char *err, size_t err_len)
{
	struct ox_ctx *c = ctx;

	const int64_t n = ox_encode(c->handle, c->y, c->cb, c->cr, out, out_cap);
	if (n < 0) {
		snprintf(err, err_len, "%s", pr_last_error());
		return false;
	}
	*out_size = (size_t)n;
	return true;
}

static void ox_add_properties(obs_properties_t *props)
{
	UNUSED_PARAMETER(props);
}

static void ox_set_defaults(obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
}

static const struct pr_backend oxideav_backend = {
	.name_key = "Encoder.OxideAV",
	.log_name = "oxideav",
	.codec_name = NULL,
	.default_in_flight = 0, /* automatic */
	.prores = true,
	.create = ox_ctx_create,
	.get_planes = ox_ctx_get_planes,
	.encode = ox_ctx_encode,
	.destroy = ox_ctx_destroy,
	.add_properties = ox_add_properties,
	.set_defaults = ox_set_defaults,
};

static void *oxideav_encoder_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return pr_encoder_create(&oxideav_backend, settings, encoder);
}

struct obs_encoder_info pr_oxideav_encoder_info = {
	.id = "obs_prores_oxideav",
	.type = OBS_ENCODER_VIDEO,
	.codec = "prores",
	.get_name = pr_encoder_get_name,
	.create = oxideav_encoder_create,
	.destroy = pr_encoder_destroy,
	.encode = pr_encoder_encode,
	.update = pr_encoder_update,
	.get_extra_data = pr_encoder_get_extra_data,
	.get_defaults2 = pr_encoder_get_defaults,
	.get_properties2 = pr_encoder_get_properties,
	.type_data = (void *)&oxideav_backend,
};
