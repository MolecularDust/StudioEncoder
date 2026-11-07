#pragma once

/* Big-endian FourCC as an integer, the form the OBS muxer expects in the "codec_type" encoder setting. */
#define FOURCC(a, b, c, d) (((int64_t)(a) << 24) | ((int64_t)(b) << 16) | ((int64_t)(c) << 8) | (int64_t)(d))

#include <obs-module.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum pr_profile {
	PR_PROFILE_PROXY = 0,
	PR_PROFILE_LT = 1,
	PR_PROFILE_422 = 2,
	PR_PROFILE_HQ = 3,
};

enum pr_log_level {
	PR_LOG_ERRORS = 0,
	PR_LOG_WARNINGS = 1,
	PR_LOG_INFO = 2,
	PR_LOG_DEBUG = 3,
};

/* Everything a backend needs to build its encoder. Colour values are H.273 codes. */
struct pr_params {
	enum pr_profile profile;
	int threads;   /* 0 = automatic; slice threads when only one frame is in flight */
	int in_flight; /* frames encoded in parallel */
	bool vendor_apl0;
	int log_level;
	const char *codec_name; /* FFmpeg encoder name, or NULL for the Rust backend */
	int bitrate_mbps; /* NVENC only */
	bool nvenc_fast;  /* NVENC only: preset p1 instead of p5 */
	int amd_quality;  /* AMD only: 0 Small, 1 Standard, 2 High, 3 Maximum */
	uint32_t width;
	uint32_t height;
	uint32_t fps_num;
	uint32_t fps_den;
	uint8_t color_primaries;
	uint8_t color_transfer;
	uint8_t color_matrix;
	bool full_range;
};

/* Destination buffers owned by a backend context. Strides are in samples. Planar 10-bit 4:2:2 (ProRes), or a
 * semi-planar Y plus interleaved CbCr copy of what OBS delivers (P216 for NVENC, P010 for AMD). */
struct pr_planes {
	uint16_t *y;
	uint16_t *cb;
	uint16_t *cr;
	size_t y_stride;
	size_t c_stride;
	/* semi-planar P216 destination (NVENC): Y plane plus interleaved CbCr, copied as is */
	bool semi_planar;
	uint16_t *uv;
	size_t uv_stride;
	/* For backends whose hardware holds a frame back (AMD AMF): the shared code sets pts (OBS's timestamp of this
	 * frame) before encode(), and encode() reports the timestamp of the packet it returns in out_pts. An encode()
	 * that returns out_size 0 means "no packet yet" and OBS is given nothing for that call. */
	int64_t pts;
	int64_t out_pts;
	bool out_pts_valid;
};

struct pr_backend {
	const char *name_key;   /* locale key for the encoder name */
	const char *log_name;   /* short name used in log lines */
	const char *codec_name; /* FFmpeg encoder name, or NULL */
	int default_in_flight;
	int max_in_flight; /* 0 = no limit; NVENC sessions are limited on consumer GPUs */
	bool prores;       /* ProRes profile list and container FourCC */
	bool input_p010;   /* takes P010 (10-bit 4:2:0) from OBS instead of P216 (10-bit 4:2:2) */
	int64_t container_tag; /* FourCC for the MOV sample entry (e.g. hvc1), or 0 for the muxer's default */
	/* Optional: describes the quality setting for the log line written when the encoder starts. */
	void (*describe)(const struct pr_params *params, char *buf, size_t buf_len);

	/* One context is created per frame in flight; each is used by one worker thread at a time. */
	void *(*create)(const struct pr_params *params, char *err, size_t err_len);
	/* Returns writable destination planes for the next frame (called on the OBS encode thread). */
	bool (*get_planes)(void *ctx, struct pr_planes *planes, char *err, size_t err_len);
	/* Encodes the planes filled after get_planes() (called on a worker thread). */
	bool (*encode)(void *ctx, uint8_t *out, size_t out_cap, size_t *out_size, char *err, size_t err_len);
	void (*destroy)(void *ctx);
	void (*add_properties)(obs_properties_t *props);
	void (*set_defaults)(obs_data_t *settings);
	/* Optional: codec headers (for example H.264 SPS/PPS) from a created context. */
	bool (*get_extra_data)(void *ctx, uint8_t **extra_data, size_t *size);
};

/* Shared encoder plumbing used by all backends. */
void *pr_encoder_create(const struct pr_backend *backend, obs_data_t *settings, obs_encoder_t *encoder);
void pr_encoder_destroy(void *data);
bool pr_encoder_update(void *data, obs_data_t *settings);
bool pr_encoder_get_extra_data(void *data, uint8_t **extra_data, size_t *size);
bool pr_encoder_encode(void *data, struct encoder_frame *frame, struct encoder_packet *packet, bool *received_packet);
const char *pr_encoder_get_name(void *type_data);
void pr_encoder_get_defaults(obs_data_t *settings, void *type_data);
obs_properties_t *pr_encoder_get_properties(void *data, void *type_data);

/* P216 (OBS output, 10-bit values in the top bits of 16-bit words, interleaved UV) to planar 10-bit.
 * Destination strides are in samples. */
void pr_p216_to_planar10(const uint8_t *src_y, size_t src_y_stride, const uint8_t *src_uv, size_t src_uv_stride,
			 uint32_t width, uint32_t height, uint16_t *dst_y, size_t dst_y_stride, uint16_t *dst_cb,
			 uint16_t *dst_cr, size_t dst_c_stride);

#ifdef __cplusplus
}
#endif
