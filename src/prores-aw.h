#pragma once

/* ProRes 422 encoder (Proxy, LT, 422, HQ), progressive only, 10-bit planar 4:2:2 input.
 *
 * A port of FFmpeg's prores_aw encoder (libavcodec/proresenc_anatoliy.c, LGPL 2.1 or later; original authors
 * Anatoliy Wasserman and Serge van den Boom) into a standalone file, so it can be changed. The change: FFmpeg's
 * prores_aw divides every coefficient by its quantiser step and truncates toward zero, which throws away all detail
 * smaller than one step. At Proxy that blurs fine texture. Here coefficients are rounded, with an adjustable
 * rounding offset; offset 0 reproduces FFmpeg's output byte for byte. */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pa_ctx;

/* profile: 0 Proxy, 1 LT, 2 422, 3 HQ. round8: rounding offset in eighths of a quantiser step (0 = truncate, as
 * FFmpeg does; 4 = round to nearest). vendor: 4-character creator code written into the frame header. Colour
 * values are H.273 codes; ones FFmpeg's prores_aw does not accept are written as 0, as it does. Returns NULL on
 * bad arguments. */
struct pa_ctx *pa_create(int width, int height, int profile, int round8, const char *vendor, int primaries,
			 int transfer, int matrix);
void pa_destroy(struct pa_ctx *ctx);

/* The rounding offset AW+ uses for each profile: the best quality per bit measured on 4K camera footage
 * (Proxy 4, LT 3, 422 3, HQ 2). Larger offsets keep more detail but cost bits, and at HQ make the rate control
 * raise the quantiser, which loses quality again. */
int pa_default_round8(int profile);

/* The forward DCT runs with AVX2 when the CPU supports it (chosen on the first pa_create), otherwise in plain C;
 * both give identical output. pa_set_simd(0) forces plain C. pa_selftest_simd compares the two on generated
 * blocks and returns the number of mismatching blocks, or -1 without AVX2. */
void pa_set_simd(int enable);
int pa_simd_active(void);
long pa_selftest_simd(long blocks);

/* Worst-case size of one encoded frame, for sizing the output buffer. */
size_t pa_max_frame_size(int width, int height);

/* Encodes one frame. Planes are 10-bit samples in uint16_t; strides are in samples. Chroma planes are half width.
 * Returns the frame size in bytes, or -1 if out_cap is too small. */
int64_t pa_encode(struct pa_ctx *ctx, const uint16_t *y, size_t y_stride, const uint16_t *cb, const uint16_t *cr,
		  size_t c_stride, uint8_t *out, size_t out_cap);

#ifdef __cplusplus
}
#endif
