/*
 * ProRes 422 encoder: a standalone port of FFmpeg's prores_aw encoder with an adjustable rounding offset.
 *
 * Based on libavcodec/proresenc_anatoliy.c, jfdctint_template.c and proresdata.c from FFmpeg 8.0.
 * Copyright (c) 2011 Anatoliy Wasserman, Copyright (c) 2012 Konstantin Shishkov (proresdata),
 * Copyright (C) 1991-1996 Thomas G. Lane / Independent JPEG Group (forward DCT).
 * Licensed under the GNU Lesser General Public License version 2.1 or later; used here under the GPL (see LICENSE).
 *
 * Differences from FFmpeg: 4:2:2 progressive only, 10-bit planar input with sample strides, no FFmpeg dependency,
 * and quantisation that can round instead of truncating (see prores-aw.h).
 */

#include "prores-aw.h"

#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

/* ---- tables (proresenc_anatoliy.c, proresdata.c) ---- */

#define SLICE_MB_WIDTH 8 /* macroblocks per slice */
#define FIRST_DC_CB 0xB8
#define HEADER_SIZE 148

static const int qp_start_table[4] = {8, 3, 2, 1};
static const int qp_end_table[4] = {13, 9, 6, 6};
static const int bitrate_table[4] = {1000, 2100, 3500, 5400};

static const uint8_t QMAT_LUMA[4][64] = {
	{4,  7,  9,  11, 13, 14, 15, 63, 7,  7,  11, 12, 14, 15, 63, 63, 9,  11, 13, 14, 15, 63,
	 63, 63, 11, 11, 13, 14, 63, 63, 63, 63, 11, 13, 14, 63, 63, 63, 63, 63, 13, 14, 63, 63,
	 63, 63, 63, 63, 13, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63},
	{4,  5,  6,  7,  9,  11, 13, 15, 5,  5,  7,  8,  11, 13, 15, 17, 6,  7,  9,  11, 13, 15,
	 15, 17, 7,  7,  9,  11, 13, 15, 17, 19, 7,  9,  11, 13, 14, 16, 19, 23, 9,  11, 13, 14,
	 16, 19, 23, 29, 9,  11, 13, 15, 17, 21, 28, 35, 11, 13, 16, 17, 21, 28, 35, 41},
	{4, 4, 5, 5, 6, 7, 7, 9,  4, 4, 5, 6, 7, 7, 9,  9,  5, 5, 6, 7, 7, 9,  9,  10,
	 5, 5, 6, 7, 7, 9, 9, 10, 5, 6, 7, 7, 8, 9, 10, 12, 6, 7, 7, 8, 9, 10, 12, 15,
	 6, 7, 7, 9, 10, 11, 14, 17, 7, 7, 9, 10, 11, 14, 17, 21},
	{4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5,
	 4, 4, 4, 4, 4, 4, 5, 5, 4, 4, 4, 4, 4, 5, 5, 6, 4, 4, 4, 4, 5, 5, 6, 7, 4, 4, 4, 4, 5, 6, 7, 7},
};

static const uint8_t QMAT_CHROMA[4][64] = {
	{4,  7,  9,  11, 13, 14, 63, 63, 7,  7,  11, 12, 14, 63, 63, 63, 9,  11, 13, 14, 63, 63,
	 63, 63, 11, 11, 13, 14, 63, 63, 63, 63, 11, 13, 14, 63, 63, 63, 63, 63, 13, 14, 63, 63,
	 63, 63, 63, 63, 13, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63},
	{4,  5,  6,  7,  9,  11, 13, 15, 5,  5,  7,  8,  11, 13, 15, 17, 6,  7,  9,  11, 13, 15,
	 15, 17, 7,  7,  9,  11, 13, 15, 17, 19, 7,  9,  11, 13, 14, 16, 19, 23, 9,  11, 13, 14,
	 16, 19, 23, 29, 9,  11, 13, 15, 17, 21, 28, 35, 11, 13, 16, 17, 21, 28, 35, 41},
	{4, 4, 5, 5, 6, 7, 7, 9,  4, 4, 5, 6, 7, 7, 9,  9,  5, 5, 6, 7, 7, 9,  9,  10,
	 5, 5, 6, 7, 7, 9, 9, 10, 5, 6, 7, 7, 8, 9, 10, 12, 6, 7, 7, 8, 9, 10, 12, 15,
	 6, 7, 7, 9, 10, 11, 14, 17, 7, 7, 9, 10, 11, 14, 17, 21},
	{4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5,
	 4, 4, 4, 4, 4, 4, 5, 5, 4, 4, 4, 4, 4, 5, 5, 6, 4, 4, 4, 4, 5, 5, 6, 7, 4, 4, 4, 4, 5, 6, 7, 7},
};

static const uint8_t progressive_scan[64] = {0,  1,  8,  9,  2,  3,  10, 11, 16, 17, 24, 25, 18, 19, 26, 27,
					     4,  5,  12, 20, 13, 6,  7,  14, 21, 28, 29, 22, 15, 23, 30, 31,
					     32, 33, 40, 48, 41, 34, 35, 42, 49, 56, 57, 50, 43, 36, 37, 44,
					     51, 58, 59, 52, 45, 38, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

static const uint8_t dc_codebook[7] = {0x04, 0x28, 0x28, 0x4D, 0x4D, 0x70, 0x70};
static const uint8_t run_to_cb[16] = {0x06, 0x06, 0x05, 0x05, 0x04, 0x29, 0x29, 0x29,
				      0x29, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x4C};
static const uint8_t level_to_cb[10] = {0x04, 0x0A, 0x05, 0x06, 0x04, 0x28, 0x28, 0x28, 0x28, 0x4C};

static const int valid_primaries[] = {0, 1, 2, 5, 6, 9, 11, 12};
static const int valid_trc[] = {0, 1, 2, 16, 18};
static const int valid_matrix[] = {1, 2, 6, 9};

/* ---- context ---- */

struct pa_ctx {
	int width, height, profile, round8;
	uint8_t vendor[4];
	uint8_t primaries, transfer, matrix;
	int qmat_luma[16][64];
	int qmat_chroma[16][64];
	uint32_t rcp_luma[16][64]; /* ceil(2^32 / step), see encode_acs */
	uint32_t rcp_chroma[16][64];
	/* edge macroblocks are copied here and padded by repeating the last pixel/line */
	uint16_t fill_y[SLICE_MB_WIDTH * 16 * 16];
	uint16_t fill_u[SLICE_MB_WIDTH * 8 * 16];
	uint16_t fill_v[SLICE_MB_WIDTH * 8 * 16];
	int16_t blocks_y[SLICE_MB_WIDTH * 256];
	int16_t blocks_u[SLICE_MB_WIDTH * 128];
	int16_t blocks_v[SLICE_MB_WIDTH * 128];
};

/* ---- big-endian bit writer (put_bits.h semantics) ---- */

struct bitw {
	uint8_t *ptr, *end;
	uint64_t acc;
	int nbits;
	int overflow;
};

static inline void bw_init(struct bitw *b, uint8_t *buf, size_t size)
{
	b->ptr = buf;
	b->end = buf + size;
	b->acc = 0;
	b->nbits = 0;
	b->overflow = 0;
}

static inline void bw_put(struct bitw *b, int n, uint32_t v)
{
	if (n <= 0)
		return;
	if (n < 32)
		v &= (1u << n) - 1;
	b->acc = (b->acc << n) | v; /* fewer than 32 bits are pending, so this fits in 64 */
	b->nbits += n;
	if (b->nbits >= 32) {
		b->nbits -= 32;
		const uint32_t w = (uint32_t)(b->acc >> b->nbits);
		if (b->end - b->ptr >= 4) {
			b->ptr[0] = (uint8_t)(w >> 24);
			b->ptr[1] = (uint8_t)(w >> 16);
			b->ptr[2] = (uint8_t)(w >> 8);
			b->ptr[3] = (uint8_t)w;
			b->ptr += 4;
		} else {
			b->overflow = 1;
		}
	}
}

static inline size_t bw_flush(struct bitw *b, uint8_t *start)
{
	while (b->nbits >= 8) {
		b->nbits -= 8;
		if (b->ptr < b->end)
			*b->ptr++ = (uint8_t)(b->acc >> b->nbits);
		else
			b->overflow = 1;
	}
	if (b->nbits > 0) {
		if (b->ptr < b->end)
			*b->ptr++ = (uint8_t)(b->acc << (8 - b->nbits));
		else
			b->overflow = 1;
		b->nbits = 0;
	}
	return (size_t)(b->ptr - start);
}

static inline int log2_floor(unsigned v)
{
#ifdef _MSC_VER
	unsigned long idx;
	return _BitScanReverse(&idx, v | 1) ? (int)idx : 0;
#else
	return 31 - __builtin_clz(v | 1);
#endif
}

/* ---- entropy coding (proresenc_anatoliy.c) ---- */

static void encode_vlc_codeword(struct bitw *pb, unsigned codebook, int val)
{
	const unsigned switch_bits = (codebook & 3) + 1;
	const unsigned rice_order = codebook >> 5;
	const unsigned exp_order = (codebook >> 2) & 7;
	const unsigned switch_val = switch_bits << rice_order;

	if ((unsigned)val >= switch_val) {
		val -= (int)(switch_val - (1u << exp_order));
		const int exponent = log2_floor((unsigned)val);
		bw_put(pb, exponent - (int)exp_order + (int)switch_bits, 0);
		bw_put(pb, exponent + 1, (uint32_t)val);
	} else {
		const int exponent = val >> rice_order;
		if (exponent)
			bw_put(pb, exponent, 0);
		bw_put(pb, 1, 1);
		if (rice_order)
			bw_put(pb, (int)rice_order, (uint32_t)val);
	}
}

#define GET_SIGN(x) ((x) >> 31)
#define MAKE_CODE(x) ((int)((unsigned)(x) * 2) ^ GET_SIGN(x))

/* x / q with rounding: offset 0 truncates toward zero like FFmpeg's prores_aw, offset = q * round8 / 8 rounds
 * up from that fraction of a step. Symmetric around zero. */
static inline int quantise(int x, int q, int offset)
{
	if (x >= 0)
		return (x + offset) / q;
	return -((-x + offset) / q);
}

static void encode_dcs(struct bitw *pb, const int16_t *blocks, int blocks_per_slice, int scale, int round8)
{
	const int offset = scale * round8 / 8;
	int codebook = 5, sign = 0;
	int prev_dc = quantise(blocks[0] - 0x4000, scale, offset);
	encode_vlc_codeword(pb, FIRST_DC_CB, MAKE_CODE(prev_dc));
	blocks += 64;

	for (int i = 1; i < blocks_per_slice; i++, blocks += 64) {
		const int dc = quantise(blocks[0] - 0x4000, scale, offset);
		int delta = dc - prev_dc;
		const int new_sign = GET_SIGN(delta);
		delta = (delta ^ sign) - sign;
		const int code = MAKE_CODE(delta);
		encode_vlc_codeword(pb, dc_codebook[codebook], code);
		codebook = code < 6 ? code : 6;
		sign = new_sign;
		prev_dc = dc;
	}
}

/* Same result as quantise() for every 16-bit coefficient: (|x| + offset) / q is computed as a multiply by
 * rcp = ceil(2^32 / q), which is exact while |x| + offset < 2^16 and q < 2^16. */
static void encode_acs(struct bitw *pb, const int16_t *blocks, int blocks_per_slice, const int *qmat,
		       const uint32_t *rcp, int round8)
{
	int prev_run = 4, prev_level = 2, run = 0;
	const int max_coeffs = blocks_per_slice << 6;

	for (int i = 1; i < 64; i++) {
		const int pos = progressive_scan[i];
		const unsigned q = (unsigned)qmat[pos];
		const uint64_t r = rcp[pos];
		const unsigned offset = q * (unsigned)round8 / 8;
		for (int idx = pos; idx < max_coeffs; idx += 64) {
			const int x = blocks[idx];
			const unsigned a = (unsigned)(x < 0 ? -x : x) + offset;
			if (a < q) {
				run++;
				continue;
			}
			const int abs_level = (int)((a * r) >> 32);
			encode_vlc_codeword(pb, run_to_cb[prev_run], run);
			encode_vlc_codeword(pb, level_to_cb[prev_level], abs_level - 1);
			bw_put(pb, 1, x < 0 ? 1u : 0u);
			prev_run = run < 15 ? run : 15;
			prev_level = abs_level < 9 ? abs_level : 9;
			run = 0;
		}
	}
}

/* ---- forward DCT, 10-bit (jfdctint_template.c with BIT_DEPTH 10) ---- */

#define CONST_BITS 13
#define PASS1_BITS 1
#define OUT_SHIFT (PASS1_BITS + 1)
#define DESCALE(x, n) (((int)(x) + (1 << ((n)-1))) >> (n))
#define MULTIPLY(var, c) ((int)((var) * (unsigned)(c)))
#define FIX_0_298631336 2446
#define FIX_0_390180644 3196
#define FIX_0_541196100 4433
#define FIX_0_765366865 6270
#define FIX_0_899976223 7373
#define FIX_1_175875602 9633
#define FIX_1_501321110 12299
#define FIX_1_847759065 15137
#define FIX_1_961570560 16069
#define FIX_2_053119869 16819
#define FIX_2_562915447 20995
#define FIX_3_072711026 25172

static void fdct_10(int16_t *data)
{
	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp10, tmp11, tmp12, tmp13;
	unsigned z1, z2, z3, z4, z5;
	int16_t *d = data;

	for (int ctr = 0; ctr < 8; ctr++, d += 8) {
		tmp0 = d[0] + d[7];
		tmp7 = d[0] - d[7];
		tmp1 = d[1] + d[6];
		tmp6 = d[1] - d[6];
		tmp2 = d[2] + d[5];
		tmp5 = d[2] - d[5];
		tmp3 = d[3] + d[4];
		tmp4 = d[3] - d[4];
		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;
		d[0] = (int16_t)((tmp10 + tmp11) * (1 << PASS1_BITS));
		d[4] = (int16_t)((tmp10 - tmp11) * (1 << PASS1_BITS));
		z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);
		d[2] = (int16_t)DESCALE(z1 + MULTIPLY(tmp13, FIX_0_765366865), CONST_BITS - PASS1_BITS);
		d[6] = (int16_t)DESCALE(z1 + MULTIPLY(tmp12, -FIX_1_847759065), CONST_BITS - PASS1_BITS);
		z1 = tmp4 + tmp7;
		z2 = tmp5 + tmp6;
		z3 = tmp4 + tmp6;
		z4 = tmp5 + tmp7;
		z5 = MULTIPLY(z3 + z4, FIX_1_175875602);
		tmp4 = MULTIPLY(tmp4, FIX_0_298631336);
		tmp5 = MULTIPLY(tmp5, FIX_2_053119869);
		tmp6 = MULTIPLY(tmp6, FIX_3_072711026);
		tmp7 = MULTIPLY(tmp7, FIX_1_501321110);
		z1 = MULTIPLY(z1, -FIX_0_899976223);
		z2 = MULTIPLY(z2, -FIX_2_562915447);
		z3 = MULTIPLY(z3, -FIX_1_961570560);
		z4 = MULTIPLY(z4, -FIX_0_390180644);
		z3 += z5;
		z4 += z5;
		d[7] = (int16_t)DESCALE(tmp4 + z1 + z3, CONST_BITS - PASS1_BITS);
		d[5] = (int16_t)DESCALE(tmp5 + z2 + z4, CONST_BITS - PASS1_BITS);
		d[3] = (int16_t)DESCALE(tmp6 + z2 + z3, CONST_BITS - PASS1_BITS);
		d[1] = (int16_t)DESCALE(tmp7 + z1 + z4, CONST_BITS - PASS1_BITS);
	}

	d = data;
	for (int ctr = 0; ctr < 8; ctr++, d++) {
		tmp0 = d[8 * 0] + d[8 * 7];
		tmp7 = d[8 * 0] - d[8 * 7];
		tmp1 = d[8 * 1] + d[8 * 6];
		tmp6 = d[8 * 1] - d[8 * 6];
		tmp2 = d[8 * 2] + d[8 * 5];
		tmp5 = d[8 * 2] - d[8 * 5];
		tmp3 = d[8 * 3] + d[8 * 4];
		tmp4 = d[8 * 3] - d[8 * 4];
		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;
		d[8 * 0] = (int16_t)DESCALE(tmp10 + tmp11, OUT_SHIFT);
		d[8 * 4] = (int16_t)DESCALE(tmp10 - tmp11, OUT_SHIFT);
		z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);
		d[8 * 2] = (int16_t)DESCALE(z1 + MULTIPLY(tmp13, FIX_0_765366865), CONST_BITS + OUT_SHIFT);
		d[8 * 6] = (int16_t)DESCALE(z1 + MULTIPLY(tmp12, -FIX_1_847759065), CONST_BITS + OUT_SHIFT);
		z1 = tmp4 + tmp7;
		z2 = tmp5 + tmp6;
		z3 = tmp4 + tmp6;
		z4 = tmp5 + tmp7;
		z5 = MULTIPLY(z3 + z4, FIX_1_175875602);
		tmp4 = MULTIPLY(tmp4, FIX_0_298631336);
		tmp5 = MULTIPLY(tmp5, FIX_2_053119869);
		tmp6 = MULTIPLY(tmp6, FIX_3_072711026);
		tmp7 = MULTIPLY(tmp7, FIX_1_501321110);
		z1 = MULTIPLY(z1, -FIX_0_899976223);
		z2 = MULTIPLY(z2, -FIX_2_562915447);
		z3 = MULTIPLY(z3, -FIX_1_961570560);
		z4 = MULTIPLY(z4, -FIX_0_390180644);
		z3 += z5;
		z4 += z5;
		d[8 * 7] = (int16_t)DESCALE(tmp4 + z1 + z3, CONST_BITS + OUT_SHIFT);
		d[8 * 5] = (int16_t)DESCALE(tmp5 + z2 + z4, CONST_BITS + OUT_SHIFT);
		d[8 * 3] = (int16_t)DESCALE(tmp6 + z2 + z3, CONST_BITS + OUT_SHIFT);
		d[8 * 1] = (int16_t)DESCALE(tmp7 + z1 + z4, CONST_BITS + OUT_SHIFT);
	}
}

/* copy an 8x8 block of 10-bit samples and transform it (plain C) */
static void fdct_get_c(const uint16_t *src, size_t stride, int16_t *block)
{
	for (int r = 0; r < 8; r++, src += stride, block += 8)
		for (int c = 0; c < 8; c++)
			block[c] = (int16_t)src[c];
	fdct_10(block - 64);
}

/* ---- the same forward DCT with AVX2: eight rows (then eight columns) at once in 32-bit lanes ----
 * Every step is the integer operation fdct_10 performs: 32-bit wrapping adds and multiplies, arithmetic shifts,
 * and truncation to 16 bits where fdct_10 stores into int16_t. The result is bit-identical (pa_selftest_simd). */

#if defined(_M_X64) || defined(__x86_64__)
#define PA_HAVE_AVX2 1
#include <immintrin.h>

#if defined(__GNUC__) || defined(__clang__)
#define PA_AVX2_FN __attribute__((target("avx2")))
#include <cpuid.h>
#else
#define PA_AVX2_FN
#endif

/* 8x8 transpose of 16-bit values: on return r[k] holds column k */
#define PA_TRANSPOSE8(r)                                                         \
	do {                                                                     \
		const __m128i a0 = _mm_unpacklo_epi16(r[0], r[1]);               \
		const __m128i a1 = _mm_unpackhi_epi16(r[0], r[1]);               \
		const __m128i a2 = _mm_unpacklo_epi16(r[2], r[3]);               \
		const __m128i a3 = _mm_unpackhi_epi16(r[2], r[3]);               \
		const __m128i a4 = _mm_unpacklo_epi16(r[4], r[5]);               \
		const __m128i a5 = _mm_unpackhi_epi16(r[4], r[5]);               \
		const __m128i a6 = _mm_unpacklo_epi16(r[6], r[7]);               \
		const __m128i a7 = _mm_unpackhi_epi16(r[6], r[7]);               \
		const __m128i b0 = _mm_unpacklo_epi32(a0, a2);                   \
		const __m128i b1 = _mm_unpackhi_epi32(a0, a2);                   \
		const __m128i b2 = _mm_unpacklo_epi32(a1, a3);                   \
		const __m128i b3 = _mm_unpackhi_epi32(a1, a3);                   \
		const __m128i b4 = _mm_unpacklo_epi32(a4, a6);                   \
		const __m128i b5 = _mm_unpackhi_epi32(a4, a6);                   \
		const __m128i b6 = _mm_unpacklo_epi32(a5, a7);                   \
		const __m128i b7 = _mm_unpackhi_epi32(a5, a7);                   \
		r[0] = _mm_unpacklo_epi64(b0, b4);                               \
		r[1] = _mm_unpackhi_epi64(b0, b4);                               \
		r[2] = _mm_unpacklo_epi64(b1, b5);                               \
		r[3] = _mm_unpackhi_epi64(b1, b5);                               \
		r[4] = _mm_unpacklo_epi64(b2, b6);                               \
		r[5] = _mm_unpackhi_epi64(b2, b6);                               \
		r[6] = _mm_unpacklo_epi64(b3, b7);                               \
		r[7] = _mm_unpackhi_epi64(b3, b7);                               \
	} while (0)

#define PA_MUL(v, k) _mm256_mullo_epi32((v), _mm256_set1_epi32(k))
#define PA_DESCALE(v, n) _mm256_srai_epi32(_mm256_add_epi32((v), _mm256_set1_epi32(1 << ((n)-1))), (n))

/* one pass of the LL&M DCT over eight independent lines held in v[0..7] (lane = line); first = row pass */
static PA_AVX2_FN inline void pa_pass_avx2(__m256i v[8], __m128i out[8], int first)
{
	const __m256i tmp0 = _mm256_add_epi32(v[0], v[7]);
	const __m256i tmp7 = _mm256_sub_epi32(v[0], v[7]);
	const __m256i tmp1 = _mm256_add_epi32(v[1], v[6]);
	const __m256i tmp6 = _mm256_sub_epi32(v[1], v[6]);
	const __m256i tmp2 = _mm256_add_epi32(v[2], v[5]);
	const __m256i tmp5 = _mm256_sub_epi32(v[2], v[5]);
	const __m256i tmp3 = _mm256_add_epi32(v[3], v[4]);
	const __m256i tmp4 = _mm256_sub_epi32(v[3], v[4]);
	const __m256i tmp10 = _mm256_add_epi32(tmp0, tmp3);
	const __m256i tmp13 = _mm256_sub_epi32(tmp0, tmp3);
	const __m256i tmp11 = _mm256_add_epi32(tmp1, tmp2);
	const __m256i tmp12 = _mm256_sub_epi32(tmp1, tmp2);
	__m256i o[8];

	if (first) { /* row pass: scale up by 2^PASS1_BITS, descale odd/rotated terms by CONST_BITS - PASS1_BITS */
		o[0] = _mm256_slli_epi32(_mm256_add_epi32(tmp10, tmp11), PASS1_BITS);
		o[4] = _mm256_slli_epi32(_mm256_sub_epi32(tmp10, tmp11), PASS1_BITS);
	} else { /* column pass: descale by OUT_SHIFT */
		o[0] = PA_DESCALE(_mm256_add_epi32(tmp10, tmp11), OUT_SHIFT);
		o[4] = PA_DESCALE(_mm256_sub_epi32(tmp10, tmp11), OUT_SHIFT);
	}
	const int sh = first ? CONST_BITS - PASS1_BITS : CONST_BITS + OUT_SHIFT;
	const __m256i rnd = _mm256_set1_epi32(1 << (sh - 1));
	const __m128i shc = _mm_cvtsi32_si128(sh);
#define PA_DS(x) _mm256_sra_epi32(_mm256_add_epi32((x), rnd), shc)

	__m256i z1 = PA_MUL(_mm256_add_epi32(tmp12, tmp13), FIX_0_541196100);
	o[2] = PA_DS(_mm256_add_epi32(z1, PA_MUL(tmp13, FIX_0_765366865)));
	o[6] = PA_DS(_mm256_add_epi32(z1, PA_MUL(tmp12, -FIX_1_847759065)));

	z1 = _mm256_add_epi32(tmp4, tmp7);
	__m256i z2 = _mm256_add_epi32(tmp5, tmp6);
	__m256i z3 = _mm256_add_epi32(tmp4, tmp6);
	__m256i z4 = _mm256_add_epi32(tmp5, tmp7);
	const __m256i z5 = PA_MUL(_mm256_add_epi32(z3, z4), FIX_1_175875602);
	const __m256i t4 = PA_MUL(tmp4, FIX_0_298631336);
	const __m256i t5 = PA_MUL(tmp5, FIX_2_053119869);
	const __m256i t6 = PA_MUL(tmp6, FIX_3_072711026);
	const __m256i t7 = PA_MUL(tmp7, FIX_1_501321110);
	z1 = PA_MUL(z1, -FIX_0_899976223);
	z2 = PA_MUL(z2, -FIX_2_562915447);
	z3 = _mm256_add_epi32(PA_MUL(z3, -FIX_1_961570560), z5);
	z4 = _mm256_add_epi32(PA_MUL(z4, -FIX_0_390180644), z5);
	o[7] = PA_DS(_mm256_add_epi32(_mm256_add_epi32(t4, z1), z3));
	o[5] = PA_DS(_mm256_add_epi32(_mm256_add_epi32(t5, z2), z4));
	o[3] = PA_DS(_mm256_add_epi32(_mm256_add_epi32(t6, z2), z3));
	o[1] = PA_DS(_mm256_add_epi32(_mm256_add_epi32(t7, z1), z4));
#undef PA_DS

	/* store as int16_t like fdct_10 does: keep the low 16 bits, sign-extended, then pack (no saturation left) */
	for (int k = 0; k < 8; k++) {
		const __m256i w = _mm256_srai_epi32(_mm256_slli_epi32(o[k], 16), 16);
		out[k] = _mm_packs_epi32(_mm256_castsi256_si128(w), _mm256_extracti128_si256(w, 1));
	}
}

static PA_AVX2_FN void fdct_get_avx2(const uint16_t *src, size_t stride, int16_t *block)
{
	__m128i r[8];
	__m256i v[8];
	for (int k = 0; k < 8; k++)
		r[k] = _mm_loadu_si128((const __m128i *)(src + k * stride)); /* 10-bit samples: same bits as int16 */

	PA_TRANSPOSE8(r); /* r[k] = element k of every row: one row per lane */
	for (int k = 0; k < 8; k++)
		v[k] = _mm256_cvtepi16_epi32(r[k]);
	pa_pass_avx2(v, r, 1); /* r[k] = row-pass output k of every row */

	PA_TRANSPOSE8(r); /* r[k] = row k of the intermediate block: one column per lane */
	for (int k = 0; k < 8; k++)
		v[k] = _mm256_cvtepi16_epi32(r[k]);
	pa_pass_avx2(v, r, 0); /* r[k] = output row k */

	for (int k = 0; k < 8; k++)
		_mm_storeu_si128((__m128i *)(block + 8 * k), r[k]);
}

static int cpu_has_avx2(void)
{
#if defined(_MSC_VER)
	int info[4];
	__cpuid(info, 0);
	if (info[0] < 7)
		return 0;
	__cpuid(info, 1);
	const int osxsave = (info[2] >> 27) & 1, avx = (info[2] >> 28) & 1;
	if (!osxsave || !avx || (_xgetbv(0) & 6) != 6)
		return 0;
	__cpuidex(info, 7, 0);
	return (info[1] >> 5) & 1;
#else
	return __builtin_cpu_supports("avx2");
#endif
}
#endif /* x86-64 */

typedef void (*fdct_get_fn)(const uint16_t *src, size_t stride, int16_t *block);
static fdct_get_fn fdct_get = fdct_get_c;

void pa_set_simd(int enable)
{
#ifdef PA_HAVE_AVX2
	fdct_get = (enable && cpu_has_avx2()) ? fdct_get_avx2 : fdct_get_c;
#else
	(void)enable;
	fdct_get = fdct_get_c;
#endif
}

int pa_simd_active(void)
{
	return fdct_get != fdct_get_c;
}

long pa_selftest_simd(long blocks)
{
#ifdef PA_HAVE_AVX2
	if (!cpu_has_avx2())
		return -1;
	uint16_t src[64];
	int16_t a[64], b[64];
	uint32_t seed = 12345;
	long mismatches = 0;
	for (long n = 0; n < blocks; n++) {
		const int mode = (int)(n % 4);
		for (int i = 0; i < 64; i++) {
			seed = seed * 1664525u + 1013904223u;
			const unsigned r = seed >> 16;
			/* random 10-bit values, plus extremes and checkerboards that drive every term to its limit */
			src[i] = (uint16_t)(mode == 0   ? (r & 1023)
					    : mode == 1 ? ((r & 1) ? 1023 : 0)
					    : mode == 2 ? ((((i >> 3) + i) & 1) ? 1023 : 0)
							: (r % 3 == 0 ? 1023 : (r % 3 == 1 ? 0 : 512)));
		}
		fdct_get_c(src, 8, a);
		fdct_get_avx2(src, 8, b);
		if (memcmp(a, b, sizeof(a)) != 0)
			mismatches++;
	}
	return mismatches;
#else
	(void)blocks;
	return -1;
#endif
}

static void calc_plane_dct(const uint16_t *src, int16_t *blocks, size_t stride, int mb_count, int chroma)
{
	int16_t *block = blocks;
	if (!chroma) {
		for (int i = 0; i < mb_count; i++, block += 256, src += 16) {
			fdct_get(src, stride, block);
			fdct_get(src + 8, stride, block + 64);
			fdct_get(src + 8 * stride, stride, block + 128);
			fdct_get(src + 8 + 8 * stride, stride, block + 192);
		}
	} else {
		for (int i = 0; i < mb_count; i++, block += 128, src += 8) {
			fdct_get(src, stride, block);
			fdct_get(src + 8 * stride, stride, block + 64);
		}
	}
}

/* ---- slices and pictures ---- */

static size_t encode_slice_plane(const int16_t *blocks, int mb_count, uint8_t *buf, size_t buf_size, const int *qmat,
				 const uint32_t *rcp,
				 int chroma, int round8, int *overflow)
{
	struct bitw pb;
	const int blocks_per_slice = mb_count << (2 - chroma);
	bw_init(&pb, buf, buf_size);
	encode_dcs(&pb, blocks, blocks_per_slice, qmat[0], round8);
	encode_acs(&pb, blocks, blocks_per_slice, qmat, rcp, round8);
	const size_t n = bw_flush(&pb, buf);
	if (pb.overflow)
		*overflow = 1;
	return n;
}

static size_t encode_slice_data(struct pa_ctx *c, int mb_count, uint8_t *buf, size_t size, size_t *ys, size_t *us,
				size_t *vs, int qp, int *overflow)
{
	*ys = encode_slice_plane(c->blocks_y, mb_count, buf, size, c->qmat_luma[qp - 1], c->rcp_luma[qp - 1], 0, c->round8, overflow);
	*us = encode_slice_plane(c->blocks_u, mb_count, buf + *ys, size - *ys, c->qmat_chroma[qp - 1], c->rcp_chroma[qp - 1], 1, c->round8,
				 overflow);
	*vs = encode_slice_plane(c->blocks_v, mb_count, buf + *ys + *us, size - *ys - *us, c->qmat_chroma[qp - 1],
				 c->rcp_chroma[qp - 1], 1, c->round8, overflow);
	return *ys + *us + *vs;
}

/* copy a (dst_w x dst_h) box at (x, y), repeating the last pixel and line where the picture ends */
static void subimage_with_fill(const uint16_t *src, unsigned x, unsigned y, size_t stride, unsigned width,
			       unsigned height, uint16_t *dst, unsigned dst_w, unsigned dst_h)
{
	const unsigned box_w = width - x < dst_w ? width - x : dst_w;
	const unsigned box_h = height - y < dst_h ? height - y : dst_h;
	unsigned i, j;
	src += y * stride + x;
	for (i = 0; i < box_h; i++, src += stride, dst += dst_w) {
		for (j = 0; j < box_w; j++)
			dst[j] = src[j];
		const uint16_t last = dst[j - 1];
		for (; j < dst_w; j++)
			dst[j] = last;
	}
	const uint16_t *last_line = dst - dst_w;
	for (; i < dst_h; i++, dst += dst_w)
		memcpy(dst, last_line, dst_w * sizeof(uint16_t));
}

static int encode_slice(struct pa_ctx *c, const uint16_t *py, size_t y_stride, const uint16_t *pu, const uint16_t *pv,
			size_t c_stride, int mb_x, int mb_y, int mb_count, uint8_t *buf, size_t data_size, int unsafe,
			int *qp)
{
	const int hdr_size = 6;
	const int tgt_bits = (mb_count * bitrate_table[c->profile]) >> 2;
	const int low_bytes = (tgt_bits - (tgt_bits >> 3)) >> 3;
	const int high_bytes = (tgt_bits + (tgt_bits >> 3)) >> 3;
	size_t ys = 0, us = 0, vs = 0, slice_size;
	int overflow = 0;

	if (data_size < (size_t)hdr_size)
		return -1;

	if (unsafe) {
		subimage_with_fill(py, (unsigned)mb_x << 4, (unsigned)mb_y << 4, y_stride, (unsigned)c->width,
				   (unsigned)c->height, c->fill_y, (unsigned)mb_count << 4, 16);
		subimage_with_fill(pu, (unsigned)mb_x << 3, (unsigned)mb_y << 4, c_stride, (unsigned)c->width >> 1,
				   (unsigned)c->height, c->fill_u, (unsigned)mb_count << 3, 16);
		subimage_with_fill(pv, (unsigned)mb_x << 3, (unsigned)mb_y << 4, c_stride, (unsigned)c->width >> 1,
				   (unsigned)c->height, c->fill_v, (unsigned)mb_count << 3, 16);
		calc_plane_dct(c->fill_y, c->blocks_y, (size_t)mb_count << 4, mb_count, 0);
		calc_plane_dct(c->fill_u, c->blocks_u, (size_t)mb_count << 3, mb_count, 1);
		calc_plane_dct(c->fill_v, c->blocks_v, (size_t)mb_count << 3, mb_count, 1);
		slice_size = encode_slice_data(c, mb_count, buf + hdr_size, data_size - hdr_size, &ys, &us, &vs, *qp,
					       &overflow);
	} else {
		calc_plane_dct(py + ((size_t)mb_y << 4) * y_stride + ((size_t)mb_x << 4), c->blocks_y, y_stride, mb_count, 0);
		calc_plane_dct(pu + ((size_t)mb_y << 4) * c_stride + ((size_t)mb_x << 3), c->blocks_u, c_stride, mb_count, 1);
		calc_plane_dct(pv + ((size_t)mb_y << 4) * c_stride + ((size_t)mb_x << 3), c->blocks_v, c_stride, mb_count, 1);

		slice_size = encode_slice_data(c, mb_count, buf + hdr_size, data_size - hdr_size, &ys, &us, &vs, *qp,
					       &overflow);
		if (slice_size > (size_t)high_bytes && *qp < qp_end_table[c->profile]) {
			do {
				*qp += 1;
				overflow = 0;
				slice_size = encode_slice_data(c, mb_count, buf + hdr_size, data_size - hdr_size, &ys,
							       &us, &vs, *qp, &overflow);
			} while (slice_size > (size_t)high_bytes && *qp < qp_end_table[c->profile]);
		} else if (slice_size < (size_t)low_bytes && *qp > qp_start_table[c->profile]) {
			do {
				*qp -= 1;
				overflow = 0;
				slice_size = encode_slice_data(c, mb_count, buf + hdr_size, data_size - hdr_size, &ys,
							       &us, &vs, *qp, &overflow);
			} while (slice_size < (size_t)low_bytes && *qp > qp_start_table[c->profile]);
		}
	}
	if (overflow)
		return -1;

	buf[0] = (uint8_t)(hdr_size << 3);
	buf[1] = (uint8_t)*qp;
	buf[2] = (uint8_t)(ys >> 8);
	buf[3] = (uint8_t)ys;
	buf[4] = (uint8_t)(us >> 8);
	buf[5] = (uint8_t)us;
	return hdr_size + (int)(ys + us + vs);
}

static int64_t encode_picture(struct pa_ctx *c, const uint16_t *py, size_t y_stride, const uint16_t *pu,
			      const uint16_t *pv, size_t c_stride, uint8_t *buf, size_t buf_size)
{
	const int mb_width = (c->width + 15) >> 4;
	const int mb_height = (c->height + 15) >> 4;
	int slice_per_line = 0, rem = mb_width;

	for (int i = 3; i >= 0; --i) { /* log2(SLICE_MB_WIDTH) = 3 */
		slice_per_line += rem >> i;
		rem &= (1 << i) - 1;
	}

	const size_t hdr_size = 8;
	const size_t table = (size_t)slice_per_line * mb_height * 2;
	if (buf_size < hdr_size + table)
		return -1;
	uint8_t *sizes = buf + hdr_size;
	uint8_t *data = sizes + table;
	size_t left = buf_size - hdr_size - table;
	int qp = qp_start_table[c->profile];

	for (int mb_y = 0; mb_y < mb_height; mb_y++) {
		int mb_x = 0, slice_mb_count = SLICE_MB_WIDTH;
		while (mb_x < mb_width) {
			while (mb_width - mb_x < slice_mb_count)
				slice_mb_count >>= 1;
			const int unsafe_bot = (c->height & 0xf) && (mb_y == mb_height - 1);
			const int unsafe_right = (c->width & 0xf) && (mb_x + slice_mb_count == mb_width);
			const int sl = encode_slice(c, py, y_stride, pu, pv, c_stride, mb_x, mb_y, slice_mb_count, data,
						    left, unsafe_bot || unsafe_right, &qp);
			if (sl < 0)
				return -1;
			*sizes++ = (uint8_t)(sl >> 8);
			*sizes++ = (uint8_t)sl;
			data += sl;
			left -= (size_t)sl;
			mb_x += slice_mb_count;
		}
	}

	const size_t pic_size = (size_t)(data - buf);
	buf[0] = (uint8_t)(hdr_size << 3);
	buf[1] = (uint8_t)(pic_size >> 24);
	buf[2] = (uint8_t)(pic_size >> 16);
	buf[3] = (uint8_t)(pic_size >> 8);
	buf[4] = (uint8_t)pic_size;
	const int nslices = slice_per_line * mb_height;
	buf[5] = (uint8_t)(nslices >> 8);
	buf[6] = (uint8_t)nslices;
	buf[7] = 3 << 4;
	return (int64_t)pic_size;
}

/* ---- public API ---- */

static uint8_t from_list(int v, const int *list, size_t n)
{
	for (size_t i = 0; i < n; i++)
		if (list[i] == v)
			return (uint8_t)v;
	return 0;
}

struct pa_ctx *pa_create(int width, int height, int profile, int round8, const char *vendor, int primaries,
			 int transfer, int matrix)
{
	if (width <= 0 || height <= 0 || (width & 1) || width > 65534 || height > 65535 || profile < 0 || profile > 3 ||
	    round8 < 0 || round8 > 7 || !vendor || strlen(vendor) != 4)
		return NULL;

	static int simd_ready = 0;
	if (!simd_ready) { /* pick the DCT once: AVX2 when the CPU has it */
		pa_set_simd(1);
		simd_ready = 1;
	}

	struct pa_ctx *c = calloc(1, sizeof(*c));
	if (!c)
		return NULL;
	c->width = width;
	c->height = height;
	c->profile = profile;
	c->round8 = round8;
	memcpy(c->vendor, vendor, 4);
	c->primaries = from_list(primaries, valid_primaries, sizeof(valid_primaries) / sizeof(int));
	c->transfer = from_list(transfer, valid_trc, sizeof(valid_trc) / sizeof(int));
	c->matrix = from_list(matrix, valid_matrix, sizeof(valid_matrix) / sizeof(int));
	for (int q = 1; q <= 16; q++) {
		for (int i = 0; i < 64; i++) {
			c->qmat_luma[q - 1][i] = QMAT_LUMA[profile][i] * q;
			c->qmat_chroma[q - 1][i] = QMAT_CHROMA[profile][i] * q;
			c->rcp_luma[q - 1][i] = (uint32_t)(((1ull << 32) + (uint64_t)c->qmat_luma[q - 1][i] - 1) / (uint64_t)c->qmat_luma[q - 1][i]);
			c->rcp_chroma[q - 1][i] = (uint32_t)(((1ull << 32) + (uint64_t)c->qmat_chroma[q - 1][i] - 1) / (uint64_t)c->qmat_chroma[q - 1][i]);
		}
	}
	return c;
}

void pa_destroy(struct pa_ctx *ctx)
{
	free(ctx);
}

size_t pa_max_frame_size(int width, int height)
{
	return (size_t)((width + 15) & ~15) * (size_t)((height + 15) & ~15) * 4 + 65536;
}

int64_t pa_encode(struct pa_ctx *c, const uint16_t *y, size_t y_stride, const uint16_t *cb, const uint16_t *cr,
		  size_t c_stride, uint8_t *out, size_t out_cap)
{
	const size_t head = 8 + HEADER_SIZE;
	if (out_cap < head)
		return -1;

	uint8_t *b = out + 8;
	b[0] = HEADER_SIZE >> 8;
	b[1] = HEADER_SIZE & 0xff;
	b[2] = 0; /* version 0: 4:2:2 without alpha */
	b[3] = 0;
	memcpy(b + 4, c->vendor, 4);
	b[8] = (uint8_t)(c->width >> 8);
	b[9] = (uint8_t)c->width;
	b[10] = (uint8_t)(c->height >> 8);
	b[11] = (uint8_t)c->height;
	b[12] = 0x80; /* 4:2:2, progressive */
	b[13] = 0;
	b[14] = c->primaries;
	b[15] = c->transfer;
	b[16] = c->matrix;
	b[17] = 0; /* no alpha */
	b[18] = 0;
	b[19] = 3; /* luma and chroma matrices follow */
	memcpy(b + 20, QMAT_LUMA[c->profile], 64);
	memcpy(b + 84, QMAT_CHROMA[c->profile], 64);

	const int64_t pic = encode_picture(c, y, y_stride, cb, cr, c_stride, out + head, out_cap - head);
	if (pic < 0)
		return -1;

	const int64_t total = (int64_t)head + pic;
	out[0] = (uint8_t)(total >> 24);
	out[1] = (uint8_t)(total >> 16);
	out[2] = (uint8_t)(total >> 8);
	out[3] = (uint8_t)total;
	out[4] = 'i';
	out[5] = 'c';
	out[6] = 'p';
	out[7] = 'f';
	return total;
}

int pa_default_round8(int profile)
{
	static const int r[4] = {4, 3, 3, 2};
	return profile >= 0 && profile < 4 ? r[profile] : 0;
}
