#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* C ABI of the Rust FilmCraft ProRes encoder (rust/src/lib.rs). Input is planar 10-bit 4:2:2. */

void *pr_create(uint32_t width, uint32_t height, int profile, int slice_threads, uint8_t primaries, uint8_t transfer,
		uint8_t matrix);
int64_t pr_encode(void *handle, const uint16_t *y, const uint16_t *cb, const uint16_t *cr, uint8_t *out,
		  size_t out_cap);
void pr_destroy(void *handle);
const char *pr_last_error(void);

/* OxideAV ProRes encoder (same input layout). */
void *ox_create(uint32_t width, uint32_t height, int profile, uint32_t fps_num, uint32_t fps_den, uint8_t primaries,
		uint8_t transfer, uint8_t matrix);
int64_t ox_encode(void *handle, const uint16_t *y, const uint16_t *cb, const uint16_t *cr, uint8_t *out,
		  size_t out_cap);
void ox_destroy(void *handle);

#ifdef __cplusplus
}
#endif
