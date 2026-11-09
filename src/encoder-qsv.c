/* HEVC 10-bit 4:2:2 all-intra on Intel QuickSync, through Intel's oneVPL API (the same API OBS's own QuickSync plugin
 * uses). OBS's bundled FFmpeg has no QuickSync encoders, so FFmpeg's hevc_qsv is not an option inside OBS.
 *
 * OBS delivers P216 (Color Format P216). The hardware takes 10-bit 4:2:2 as packed Y210, so each frame is repacked on
 * the worker thread: Y0 Cb Y1 Cr per pixel pair, each a 16-bit word with the 10 bits at the top, rounded from OBS's
 * 16-bit values. OBS's own QuickSync encoder refuses P216 ("Cannot perform 16-bit encode"), which is why this encoder
 * exists. */

#include "encoder-common.h"

#include <plugin-support.h>
#include <util/bmem.h>

#include <vpl/mfx.h>

#include <stdio.h>
#include <string.h>

#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define SE_HAVE_SSE2 1
#endif

#ifdef _WIN32
#include <windows.h>
#define qsv_sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#define qsv_sleep_ms(ms) usleep((ms) * 1000)
#endif

#define S_QSV_QUALITY "qsv_quality"
/* Frames queued in the hardware at once. Waiting for each frame before sending the next left the encoder idle between
 * frames (about 37 ms per 4K frame on an Arc B390, too slow for 30 fps); with frames queued it runs at full speed and
 * each packet is collected QSV_PIPE - 1 frames later. */
#define QSV_PIPE 3
#define QSV_SURFACES (QSV_PIPE + 3)

/* ICQ (intelligent constant quality) levels. Measured with the same hardware encoder (through FFmpeg's hevc_qsv) on a
 * live 4K 30p camera clip on an Arc B390 (Core Ultra X7 358H): 24 about 32 Mbit/s, 21 about 89, 18 about 120, 12 about
 * 160. Noisy footage gives bigger files. */
static const int qsv_icq[] = {24, 21, 18, 12};
static const char *const qsv_quality_names[] = {"Small", "Standard", "High", "Maximum"};

static int qsv_icq_for(int quality)
{
	if (quality < 0 || quality > 3)
		quality = 2;
	return qsv_icq[quality];
}

struct qsv_surface {
	mfxFrameSurface1 s;
	uint8_t *buf; /* Y210, width x 4 bytes per row */
};

struct qsv_ctx {
	mfxLoader loader;
	mfxSession session;
	bool initialized;
	uint32_t width, height;

	/* OBS's P216 frame is copied here by the shared code (Y plane, then the full-height interleaved CbCr plane) */
	uint16_t *p216;
	struct pr_planes *planes;

	struct qsv_surface surf[QSV_SURFACES];

	/* frames sent to the hardware and not yet collected, oldest first (a ring of QSV_PIPE entries) */
	struct {
		mfxSyncPoint sync;
		mfxBitstream bs;
	} pend[QSV_PIPE];
	int pend_head;
	int pend_count;

	uint8_t extra[2048]; /* VPS + SPS + PPS, Annex B */
	size_t extra_size;
};

/* ---- P216 to Y210 ---- */

/* P216 (Y plane + interleaved CbCr plane, 16-bit) to Y210 (Y0 Cb Y1 Cr, 16-bit words with 10 significant bits). Each
 * value is rounded to 10 bits: (v + 32) with saturation, low 6 bits cleared. Strides are in samples. */
static void p216_to_y210(const uint16_t *src_y, const uint16_t *src_uv, size_t src_stride, uint32_t width,
			 uint32_t height, uint16_t *dst, size_t dst_stride)
{
	for (uint32_t row = 0; row < height; row++) {
		const uint16_t *y = src_y + (size_t)row * src_stride;
		const uint16_t *uv = src_uv + (size_t)row * src_stride;
		uint16_t *d = dst + (size_t)row * dst_stride;
		uint32_t x = 0;
#ifdef SE_HAVE_SSE2
		const __m128i round = _mm_set1_epi16(32);
		const __m128i mask = _mm_set1_epi16((short)0xFFC0);
		for (; x + 8 <= width; x += 8) {
			/* 8 luma samples and the 4 CbCr pairs that go with them */
			const __m128i vy =
				_mm_and_si128(_mm_adds_epu16(_mm_loadu_si128((const __m128i *)(y + x)), round), mask);
			const __m128i vc =
				_mm_and_si128(_mm_adds_epu16(_mm_loadu_si128((const __m128i *)(uv + x)), round), mask);
			/* Y0 Cb0 Y1 Cr0 Y2 Cb1 Y3 Cr1 | Y4 Cb2 Y5 Cr2 Y6 Cb3 Y7 Cr3 */
			_mm_storeu_si128((__m128i *)(d + 2 * x), _mm_unpacklo_epi16(vy, vc));
			_mm_storeu_si128((__m128i *)(d + 2 * x + 8), _mm_unpackhi_epi16(vy, vc));
		}
#endif
		for (; x < width; x++) {
			const uint32_t vy = (uint32_t)y[x] + 32;
			const uint32_t vc = (uint32_t)uv[x] + 32;
			d[2 * x] = (uint16_t)((vy > 0xFFFF ? 0xFFFF : vy) & 0xFFC0);
			d[2 * x + 1] = (uint16_t)((vc > 0xFFFF ? 0xFFFF : vc) & 0xFFC0);
		}
	}
}

/* ---- session ---- */

static void qsv_close(struct qsv_ctx *c)
{
	if (c->initialized)
		MFXVideoENCODE_Close(c->session);
	c->initialized = false;
	if (c->session)
		MFXClose(c->session);
	c->session = NULL;
	if (c->loader)
		MFXUnload(c->loader);
	c->loader = NULL;
}

/* Opens a hardware session and starts the encoder in 10-bit 4:2:2 intra-only mode. p may be NULL (start-up check). */
static bool qsv_open(struct qsv_ctx *c, uint32_t width, uint32_t height, uint32_t fps_num, uint32_t fps_den,
		     int quality, const struct pr_params *p, char *err, size_t err_len)
{
	c->loader = MFXLoad();
	if (!c->loader) {
		snprintf(err, err_len, "the oneVPL dispatcher could not start");
		return false;
	}
	mfxConfig cfg = MFXCreateConfig(c->loader);
	mfxVariant v;
	memset(&v, 0, sizeof(v));
	v.Type = MFX_VARIANT_TYPE_U32;
	v.Data.U32 = MFX_IMPL_TYPE_HARDWARE;
	MFXSetConfigFilterProperty(cfg, (const mfxU8 *)"mfxImplDescription.Impl", v);

	mfxStatus sts = MFXCreateSession(c->loader, 0, &c->session);
	if (sts != MFX_ERR_NONE) {
		snprintf(err, err_len, "no Intel graphics with a QuickSync runtime was found (MFXCreateSession %d)",
			 (int)sts);
		return false;
	}

	mfxVideoParam par;
	memset(&par, 0, sizeof(par));
	par.mfx.CodecId = MFX_CODEC_HEVC;
	par.mfx.CodecProfile = MFX_PROFILE_HEVC_REXT;
	par.mfx.TargetUsage = MFX_TARGETUSAGE_BALANCED;
	/* all-intra: a GOP of one frame, every I-frame an IDR frame */
	par.mfx.GopPicSize = 1;
	par.mfx.GopRefDist = 1;
	par.mfx.IdrInterval = 1; /* HEVC: 1 = every I-frame is an IDR frame (0 would mean only the first) */
	par.mfx.RateControlMethod = MFX_RATECONTROL_ICQ;
	par.mfx.ICQQuality = (mfxU16)qsv_icq_for(quality);

	mfxFrameInfo *fi = &par.mfx.FrameInfo;
	fi->FourCC = MFX_FOURCC_Y210;
	fi->ChromaFormat = MFX_CHROMAFORMAT_YUV422;
	fi->BitDepthLuma = 10;
	fi->BitDepthChroma = 10;
	fi->Shift = 1; /* the 10 bits sit at the top of each 16-bit word */
	fi->PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
	fi->FrameRateExtN = fps_num;
	fi->FrameRateExtD = fps_den;
	fi->Width = (mfxU16)((width + 15) & ~15u);
	fi->Height = (mfxU16)((height + 15) & ~15u);
	fi->CropW = (mfxU16)width;
	fi->CropH = (mfxU16)height;

	par.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
	par.AsyncDepth = QSV_PIPE;

	mfxExtVideoSignalInfo vsi;
	memset(&vsi, 0, sizeof(vsi));
	vsi.Header.BufferId = MFX_EXTBUFF_VIDEO_SIGNAL_INFO;
	vsi.Header.BufferSz = sizeof(vsi);
	vsi.VideoFormat = 5; /* unspecified */
	vsi.VideoFullRange = p && p->full_range ? 1 : 0;
	vsi.ColourDescriptionPresent = 1;
	vsi.ColourPrimaries = p ? p->color_primaries : 1;
	vsi.TransferCharacteristics = p ? p->color_transfer : 1;
	vsi.MatrixCoefficients = p ? p->color_matrix : 1;
	mfxExtBuffer *ext[1] = {&vsi.Header};
	par.ExtParam = ext;
	par.NumExtParam = 1;

	sts = MFXVideoENCODE_Init(c->session, &par);
	if (sts < MFX_ERR_NONE) {
		snprintf(err, err_len,
			 "the graphics driver refused HEVC 10-bit 4:2:2 intra (MFXVideoENCODE_Init %d); this needs Intel "
			 "graphics with a 4:2:2 HEVC encoder (Arc, or Core Ultra graphics)",
			 (int)sts);
		return false;
	}
	if (sts > MFX_ERR_NONE)
		obs_log(LOG_INFO, "qsv-hevc422: encoder started with warning %d (parameters adjusted by the driver)",
			(int)sts);
	c->initialized = true;
	c->width = width;
	c->height = height;
	return true;
}

/* Reads the VPS, SPS and PPS for the MOV header. */
static bool qsv_read_headers(struct qsv_ctx *c, char *err, size_t err_len)
{
	mfxU8 vps[512], sps[1024], pps[512];
	mfxExtCodingOptionVPS ov;
	mfxExtCodingOptionSPSPPS os;
	memset(&ov, 0, sizeof(ov));
	memset(&os, 0, sizeof(os));
	ov.Header.BufferId = MFX_EXTBUFF_CODING_OPTION_VPS;
	ov.Header.BufferSz = sizeof(ov);
	ov.VPSBuffer = vps;
	ov.VPSBufSize = sizeof(vps);
	os.Header.BufferId = MFX_EXTBUFF_CODING_OPTION_SPSPPS;
	os.Header.BufferSz = sizeof(os);
	os.SPSBuffer = sps;
	os.SPSBufSize = sizeof(sps);
	os.PPSBuffer = pps;
	os.PPSBufSize = sizeof(pps);
	mfxExtBuffer *ext[2] = {&ov.Header, &os.Header};

	mfxVideoParam par;
	memset(&par, 0, sizeof(par));
	par.ExtParam = ext;
	par.NumExtParam = 2;
	mfxStatus sts = MFXVideoENCODE_GetVideoParam(c->session, &par);
	if (sts != MFX_ERR_NONE) {
		snprintf(err, err_len, "could not read the stream headers (GetVideoParam %d)", (int)sts);
		return false;
	}
	const size_t total = (size_t)ov.VPSBufSize + os.SPSBufSize + os.PPSBufSize;
	if (total > sizeof(c->extra)) {
		snprintf(err, err_len, "stream headers too large (%zu bytes)", total);
		return false;
	}
	memcpy(c->extra, vps, ov.VPSBufSize);
	memcpy(c->extra + ov.VPSBufSize, sps, os.SPSBufSize);
	memcpy(c->extra + ov.VPSBufSize + os.SPSBufSize, pps, os.PPSBufSize);
	c->extra_size = total;

	/* bitstream buffer: the driver's suggestion, but at least 4 bytes per pixel (intra frames of noisy footage) */
	size_t cap = (size_t)par.mfx.BufferSizeInKB * 1000 * (par.mfx.BRCParamMultiplier ? par.mfx.BRCParamMultiplier : 1);
	const size_t floor = (size_t)c->width * c->height * 4;
	if (cap < floor)
		cap = floor;
	for (int i = 0; i < QSV_PIPE; i++) {
		c->pend[i].bs.Data = bmalloc(cap);
		c->pend[i].bs.MaxLength = (mfxU32)cap;
	}
	return true;
}

static void qsv_destroy(void *ctx)
{
	struct qsv_ctx *c = ctx;
	if (!c)
		return;
	qsv_close(c);
	for (int i = 0; i < QSV_SURFACES; i++)
		bfree(c->surf[i].buf);
	for (int i = 0; i < QSV_PIPE; i++)
		bfree(c->pend[i].bs.Data);
	bfree(c->p216);
	bfree(c);
}

static void *qsv_create(const struct pr_params *p, char *err, size_t err_len)
{
	char why[256] = "";
	struct qsv_ctx *c = bzalloc(sizeof(*c));

	if (!qsv_open(c, p->width, p->height, p->fps_num, p->fps_den, p->qsv_quality, p, why, sizeof(why)) ||
	    !qsv_read_headers(c, why, sizeof(why))) {
		snprintf(err, err_len, "QuickSync could not start HEVC 10-bit 4:2:2: %s.", why);
		qsv_destroy(c);
		return NULL;
	}

	c->p216 = bmalloc((size_t)p->width * p->height * 2 * sizeof(uint16_t));

	/* system-memory Y210 surfaces; the encoder may keep one locked while it holds a frame back */
	const uint32_t aw = (p->width + 15) & ~15u, ah = (p->height + 15) & ~15u;
	for (int i = 0; i < QSV_SURFACES; i++) {
		struct qsv_surface *s = &c->surf[i];
		s->buf = bzalloc((size_t)aw * ah * 4);
		s->s.Info.FourCC = MFX_FOURCC_Y210;
		s->s.Info.ChromaFormat = MFX_CHROMAFORMAT_YUV422;
		s->s.Info.BitDepthLuma = 10;
		s->s.Info.BitDepthChroma = 10;
		s->s.Info.Shift = 1;
		s->s.Info.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
		s->s.Info.Width = (mfxU16)aw;
		s->s.Info.Height = (mfxU16)ah;
		s->s.Info.CropW = (mfxU16)p->width;
		s->s.Info.CropH = (mfxU16)p->height;
		s->s.Info.FrameRateExtN = p->fps_num;
		s->s.Info.FrameRateExtD = p->fps_den;
		s->s.Data.Y16 = (mfxU16 *)s->buf;
		s->s.Data.U16 = (mfxU16 *)s->buf + 1;
		s->s.Data.V16 = (mfxU16 *)s->buf + 3;
		s->s.Data.Pitch = (mfxU16)(aw * 4);
	}
	return c;
}

static bool qsv_get_planes(void *ctx, struct pr_planes *planes, char *err, size_t err_len)
{
	(void)err;
	(void)err_len;
	struct qsv_ctx *c = ctx;
	c->planes = planes; /* the planes live in the slot, which outlives this call */
	planes->semi_planar = true;
	planes->y = c->p216;
	planes->uv = c->p216 + (size_t)c->width * c->height;
	planes->y_stride = c->width;
	planes->uv_stride = c->width;
	planes->cb = planes->cr = NULL;
	planes->c_stride = 0;
	return true;
}

static bool qsv_encode(void *ctx, uint8_t *out, size_t out_cap, size_t *out_size, char *err, size_t err_len)
{
	struct qsv_ctx *c = ctx;

	struct qsv_surface *s = NULL;
	for (int i = 0; i < QSV_SURFACES && !s; i++)
		if (!c->surf[i].s.Data.Locked)
			s = &c->surf[i];
	if (!s) {
		snprintf(err, err_len, "all QuickSync input surfaces are in use");
		return false;
	}

	p216_to_y210(c->p216, c->p216 + (size_t)c->width * c->height, c->width, c->width, c->height,
		     (uint16_t *)s->buf, s->s.Data.Pitch / 2);
	s->s.Data.TimeStamp = (mfxU64)c->planes->pts;

	mfxEncodeCtrl ctrl;
	memset(&ctrl, 0, sizeof(ctrl));
	ctrl.FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;

	/* the ring always has a free entry here: it is drained to QSV_PIPE - 1 at the end of every call */
	const int slot = (c->pend_head + c->pend_count) % QSV_PIPE;
	mfxBitstream *bs = &c->pend[slot].bs;
	bs->DataOffset = 0;
	bs->DataLength = 0;
	c->pend[slot].sync = NULL;
	mfxStatus sts;
	for (int tries = 0;; tries++) {
		sts = MFXVideoENCODE_EncodeFrameAsync(c->session, &ctrl, &s->s, bs, &c->pend[slot].sync);
		if (sts != MFX_WRN_DEVICE_BUSY || tries >= 1000)
			break;
		qsv_sleep_ms(1);
	}
	if (sts == MFX_ERR_NONE || (sts > MFX_ERR_NONE && c->pend[slot].sync)) {
		c->pend_count++;
	} else if (sts != MFX_ERR_MORE_DATA) {
		/* MORE_DATA means the encoder kept the frame without output yet; anything else is a failure */
		snprintf(err, err_len, "EncodeFrameAsync failed (%d)", (int)sts);
		return false;
	}

	/* until the queue is full there is nothing to hand back; OBS gets its timestamps back with each packet */
	*out_size = 0;
	c->planes->out_pts_valid = false;
	if (c->pend_count < QSV_PIPE)
		return true;

	const int old = c->pend_head;
	c->pend_head = (c->pend_head + 1) % QSV_PIPE;
	c->pend_count--;
	sts = MFXVideoCORE_SyncOperation(c->session, c->pend[old].sync, 60000);
	if (sts != MFX_ERR_NONE) {
		snprintf(err, err_len, "SyncOperation failed (%d)", (int)sts);
		return false;
	}
	bs = &c->pend[old].bs;
	if (bs->DataLength > out_cap) {
		snprintf(err, err_len, "packet of %u bytes exceeds buffer of %zu", (unsigned)bs->DataLength, out_cap);
		return false;
	}
	memcpy(out, bs->Data + bs->DataOffset, bs->DataLength);
	*out_size = bs->DataLength;
	c->planes->out_pts = (int64_t)bs->TimeStamp;
	c->planes->out_pts_valid = true;
	return true;
}

static bool qsv_get_extra_data(void *ctx, uint8_t **extra_data, size_t *size)
{
	struct qsv_ctx *c = ctx;
	if (!c || !c->extra_size)
		return false;
	*extra_data = c->extra;
	*size = c->extra_size;
	return true;
}

static void qsv_add_properties(obs_properties_t *props)
{
	obs_property_t *q = obs_properties_add_list(props, S_QSV_QUALITY, obs_module_text("QsvQuality"),
						    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(q, obs_module_text("QsvQuality.Small"), 0);
	obs_property_list_add_int(q, obs_module_text("QsvQuality.Standard"), 1);
	obs_property_list_add_int(q, obs_module_text("QsvQuality.High"), 2);
	obs_property_list_add_int(q, obs_module_text("QsvQuality.Maximum"), 3);
	obs_property_set_long_description(q, obs_module_text("QsvQuality.Description"));
}

static void qsv_set_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_QSV_QUALITY, 2);
}

static void qsv_describe(const struct pr_params *p, char *buf, size_t len)
{
	const int q = p->qsv_quality < 0 || p->qsv_quality > 3 ? 2 : p->qsv_quality;
	snprintf(buf, len, "%s (ICQ %d)", qsv_quality_names[q], qsv_icq[q]);
}

static const struct pr_backend qsv_hevc422_backend = {
	.name_key = "Encoder.QsvHevc422",
	.log_name = "qsv-hevc422",
	.codec_name = NULL,
	.default_in_flight = 0, /* automatic */
	.max_in_flight = 1,     /* one encoder session, so frames held back inside it stay in order */
	.prores = false,
	.input_p010 = false, /* P216 */
	.container_tag = FOURCC('h', 'v', 'c', '1'),
	.describe = qsv_describe,
	.create = qsv_create,
	.get_planes = qsv_get_planes,
	.encode = qsv_encode,
	.destroy = qsv_destroy,
	.get_extra_data = qsv_get_extra_data,
	.add_properties = qsv_add_properties,
	.set_defaults = qsv_set_defaults,
};

static void *qsv_hevc422_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return pr_encoder_create(&qsv_hevc422_backend, settings, encoder);
}

struct obs_encoder_info pr_qsv_hevc422_encoder_info = {
	.id = "obs_prores_qsv_hevc422_intra",
	.type = OBS_ENCODER_VIDEO,
	.codec = "hevc",
	.get_name = pr_encoder_get_name,
	.create = qsv_hevc422_create,
	.destroy = pr_encoder_destroy,
	.encode = pr_encoder_encode,
	.update = pr_encoder_update,
	.get_extra_data = pr_encoder_get_extra_data,
	.get_defaults2 = pr_encoder_get_defaults,
	.get_properties2 = pr_encoder_get_properties,
	.type_data = (void *)&qsv_hevc422_backend,
};

/* Asks the graphics driver whether it can start HEVC 10-bit 4:2:2 on QuickSync, by starting an encoder on a small
 * frame and closing it again. Used once at start-up to decide whether the encoder is offered. */
bool pr_qsv_hevc422_probe(char *reason, size_t reason_len)
{
	struct qsv_ctx c;
	memset(&c, 0, sizeof(c));
	const bool ok = qsv_open(&c, 640, 360, 30, 1, 2, NULL, reason, reason_len);
	qsv_close(&c);
	return ok;
}
