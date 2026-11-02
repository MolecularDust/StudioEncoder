#include "encoder-common.h"
#include "prores-rs.h"

#include <util/bmem.h>

#include <stdio.h>

struct rust_ctx {
	void *handle;
	uint32_t width;
	uint32_t height;
	uint16_t *y;
	uint16_t *cb;
	uint16_t *cr;
};

static void rust_destroy(void *ctx)
{
	struct rust_ctx *c = ctx;
	if (!c)
		return;
	if (c->handle)
		pr_destroy(c->handle);
	bfree(c->y);
	bfree(c->cb);
	bfree(c->cr);
	bfree(c);
}

static void *rust_create(const struct pr_params *p, char *err, size_t err_len)
{
	struct rust_ctx *c = bzalloc(sizeof(*c));
	c->width = p->width;
	c->height = p->height;

	/* The Rust encoder can spread one frame over all cores. When several frames are in flight each worker
	 * already has a frame of its own, and per-frame parallelism would only make the workers compete. */
	const int slice_threads = (p->in_flight > 1 || p->threads == 1) ? 0 : 1;
	c->handle = pr_create(p->width, p->height, (int)p->profile, slice_threads, p->color_primaries,
			      p->color_transfer, p->color_matrix);
	if (!c->handle) {
		snprintf(err, err_len, "%s", pr_last_error());
		rust_destroy(c);
		return NULL;
	}

	const size_t luma = (size_t)p->width * p->height;
	const size_t chroma = (size_t)(p->width / 2) * p->height;
	c->y = bmalloc(luma * sizeof(uint16_t));
	c->cb = bmalloc(chroma * sizeof(uint16_t));
	c->cr = bmalloc(chroma * sizeof(uint16_t));
	return c;
}

static bool rust_get_planes(void *ctx, struct pr_planes *planes, char *err, size_t err_len)
{
	struct rust_ctx *c = ctx;
	UNUSED_PARAMETER(err);
	UNUSED_PARAMETER(err_len);

	planes->y = c->y;
	planes->cb = c->cb;
	planes->cr = c->cr;
	planes->y_stride = c->width;
	planes->c_stride = c->width / 2;
	return true;
}

static bool rust_encode(void *ctx, uint8_t *out, size_t out_cap, size_t *out_size, char *err, size_t err_len)
{
	struct rust_ctx *c = ctx;

	const int64_t n = pr_encode(c->handle, c->y, c->cb, c->cr, out, out_cap);
	if (n < 0) {
		snprintf(err, err_len, "%s", pr_last_error());
		return false;
	}
	*out_size = (size_t)n;
	return true;
}

static void rust_add_properties(obs_properties_t *props)
{
	UNUSED_PARAMETER(props);
}

static void rust_set_defaults(obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
}

static const struct pr_backend rust_backend = {
	.name_key = "Encoder.Rust",
	.log_name = "rust",
	.codec_name = NULL,
	.default_in_flight = 0, /* automatic */
	.create = rust_create,
	.get_planes = rust_get_planes,
	.encode = rust_encode,
	.destroy = rust_destroy,
	.add_properties = rust_add_properties,
	.set_defaults = rust_set_defaults,
};

static void *rust_encoder_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return pr_encoder_create(&rust_backend, settings, encoder);
}

struct obs_encoder_info pr_rust_encoder_info = {
	.id = "obs_prores_rust",
	.type = OBS_ENCODER_VIDEO,
	.codec = "prores",
	.get_name = pr_encoder_get_name,
	.create = rust_encoder_create,
	.destroy = pr_encoder_destroy,
	.encode = pr_encoder_encode,
	.update = pr_encoder_update,
	.get_extra_data = pr_encoder_get_extra_data,
	.get_defaults2 = pr_encoder_get_defaults,
	.get_properties2 = pr_encoder_get_properties,
	.type_data = (void *)&rust_backend,
};
