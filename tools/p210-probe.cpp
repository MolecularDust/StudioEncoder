/* p210-probe [device-substring] [width height] [seconds]
 * Opens a DirectShow capture device in the P210 format (10-bit 4:2:2, two planes, 16-bit samples),
 * collects frames for a few seconds and reports whether the samples carry real 10-bit detail or
 * 8-bit data padded or bit-replicated to 10 bits. Also writes a small 8-bit luma preview (PGM).
 *
 * Build (VS developer prompt): cl /EHsc /O2 p210-probe.cpp ole32.lib oleaut32.lib strmiids.lib */
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <dshow.h>
#include <dvdmedia.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

/* qedit.h (Sample Grabber, Null Renderer) is no longer in the Windows SDK; qedit.dll still ships. */
static const CLSID CLSID_SampleGrabber_ = {0xC1F400A0, 0x3F08, 0x11d3, {0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37}};
static const CLSID CLSID_NullRenderer_ = {0xC1F400A4, 0x3F08, 0x11d3, {0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37}};
static const IID IID_ISampleGrabber_ = {0x6B652FFF, 0x11FE, 0x4fce, {0x92, 0xAD, 0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F}};
static const IID IID_ISampleGrabberCB_ = {0x0579154A, 0x2B53, 0x4994, {0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85}};

struct ISampleGrabberCB : public IUnknown {
	virtual HRESULT STDMETHODCALLTYPE SampleCB(double time, IMediaSample *sample) = 0;
	virtual HRESULT STDMETHODCALLTYPE BufferCB(double time, BYTE *buf, long len) = 0;
};

struct ISampleGrabber : public IUnknown {
	virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL one_shot) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE *type) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE *type) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL buffer) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long *size, long *buf) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample **sample) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB *cb, long which) = 0;
};

static const DWORD FOURCC_P210 = MAKEFOURCC('P', '2', '1', '0');

static GUID fourcc_guid(DWORD fourcc)
{
	GUID g = {0x00000000, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};
	g.Data1 = fourcc;
	return g;
}

static std::string fourcc_str(const GUID &g)
{
	GUID base = fourcc_guid(g.Data1);
	if (memcmp(&base, &g, sizeof(GUID)) != 0)
		return "(non-FourCC subtype)";
	char s[5] = {(char)(g.Data1 & 0xFF), (char)((g.Data1 >> 8) & 0xFF), (char)((g.Data1 >> 16) & 0xFF),
		     (char)((g.Data1 >> 24) & 0xFF), 0};
	for (int i = 0; i < 4; i++)
		if (s[i] < 32 || s[i] > 126)
			s[i] = '?';
	return s;
}

static void free_mt(AM_MEDIA_TYPE *mt)
{
	if (!mt)
		return;
	if (mt->cbFormat)
		CoTaskMemFree(mt->pbFormat);
	if (mt->pUnk)
		mt->pUnk->Release();
	CoTaskMemFree(mt);
}

static BITMAPINFOHEADER *bmi_of(AM_MEDIA_TYPE *mt, REFERENCE_TIME **avg)
{
	if (mt->formattype == FORMAT_VideoInfo && mt->cbFormat >= sizeof(VIDEOINFOHEADER)) {
		VIDEOINFOHEADER *v = (VIDEOINFOHEADER *)mt->pbFormat;
		*avg = &v->AvgTimePerFrame;
		return &v->bmiHeader;
	}
	if (mt->formattype == FORMAT_VideoInfo2 && mt->cbFormat >= sizeof(VIDEOINFOHEADER2)) {
		VIDEOINFOHEADER2 *v = (VIDEOINFOHEADER2 *)mt->pbFormat;
		*avg = &v->AvgTimePerFrame;
		return &v->bmiHeader;
	}
	return nullptr;
}

/* --- analysis ------------------------------------------------------------------------------- */

struct PlaneStats {
	uint64_t samples = 0;
	uint64_t low6_nonzero = 0;  /* bits below the 10-bit value; 0 for MSB-aligned P210 */
	uint64_t lsb2_nonzero = 0;  /* lowest 2 of the 10 bits; always 0 if 8-bit data was shifted up */
	uint64_t replicated = 0;    /* lowest 2 bits == top 2 bits: what 8-to-10-bit replication gives */
	uint32_t vmin = 0xFFFF, vmax = 0;
	std::vector<uint32_t> hist = std::vector<uint32_t>(1024, 0);

	void add(uint16_t v)
	{
		samples++;
		if (v & 0x3F)
			low6_nonzero++;
		const uint32_t t = v >> 6;
		if (t & 3)
			lsb2_nonzero++;
		if ((t & 3) == (t >> 8))
			replicated++;
		hist[t]++;
		if (t < vmin)
			vmin = t;
		if (t > vmax)
			vmax = t;
	}
	uint32_t distinct() const
	{
		uint32_t n = 0;
		for (uint32_t c : hist)
			n += c != 0;
		return n;
	}
	/* distinct values whose lowest 2 bits are not 0: impossible for 8-bit data shifted up */
	uint32_t distinct_fine() const
	{
		uint32_t n = 0;
		for (uint32_t i = 0; i < 1024; i++)
			n += hist[i] && (i & 3);
		return n;
	}
};

struct Grabber : public ISampleGrabberCB {
	int width = 0, height = 0;
	std::atomic<long> frames{0};
	std::atomic<long> analysed{0};
	std::atomic<long> bad_size{0};
	long last_len = 0;
	std::mutex lock;
	PlaneStats y, uv;
	std::vector<uint8_t> preview;
	double first_time = -1, last_time = 0;

	STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override
	{
		if (riid == IID_IUnknown || riid == IID_ISampleGrabberCB_) {
			*ppv = this;
			return S_OK;
		}
		*ppv = nullptr;
		return E_NOINTERFACE;
	}
	STDMETHODIMP_(ULONG) AddRef() override { return 2; }
	STDMETHODIMP_(ULONG) Release() override { return 1; }
	STDMETHODIMP SampleCB(double, IMediaSample *) override { return E_NOTIMPL; }

	STDMETHODIMP BufferCB(double time, BYTE *buf, long len) override
	{
		const long n = ++frames;
		if (first_time < 0)
			first_time = time;
		last_time = time;
		last_len = len;
		/* P210: luma plane (h rows) then interleaved CbCr plane (h rows), 16-bit samples */
		const long stride = width * 2;
		if (len < stride * height * 2) {
			bad_size++;
			return S_OK;
		}
		if (n < 10 || n % 15 != 0) /* skip start-up frames; analyse every 15th to keep up */
			return S_OK;
		std::lock_guard<std::mutex> g(lock);
		const uint16_t *p = (const uint16_t *)buf;
		const size_t plane = (size_t)stride / 2 * height;
		for (size_t i = 0; i < plane; i++)
			y.add(p[i]);
		for (size_t i = 0; i < plane; i++)
			uv.add(p[plane + i]);
		if (preview.empty()) {
			const int pw = width / 4, ph = height / 4;
			preview.resize((size_t)pw * ph);
			for (int r = 0; r < ph; r++)
				for (int c = 0; c < pw; c++)
					preview[(size_t)r * pw + c] = (uint8_t)(p[(size_t)r * 4 * width + c * 4] >> 8);
		}
		analysed++;
		return S_OK;
	}
};

static void report(const char *name, const PlaneStats &s)
{
	if (!s.samples) {
		printf("%s: no samples\n", name);
		return;
	}
	printf("%s: %llu samples, 10-bit range %u..%u, %u distinct values (%u with non-zero lowest 2 bits)\n", name,
	       (unsigned long long)s.samples, s.vmin, s.vmax, s.distinct(), s.distinct_fine());
	printf("    lowest 6 bits non-zero (expect 0 for P210): %.4f%%\n", 100.0 * s.low6_nonzero / s.samples);
	printf("    lowest 2 of the 10 bits non-zero:          %.2f%%\n", 100.0 * s.lsb2_nonzero / s.samples);
	printf("    lowest 2 bits equal top 2 (replication):    %.2f%%\n", 100.0 * s.replicated / s.samples);
}

#define CHECK(hr, what)                                                       \
	do {                                                                  \
		if (FAILED(hr)) {                                             \
			fprintf(stderr, "%s failed: 0x%08lx\n", what, (long)(hr)); \
			return 1;                                             \
		}                                                             \
	} while (0)

int main(int argc, char **argv)
{
	std::string want = argc > 1 ? argv[1] : "Pro Capture";
	int req_w = argc > 3 ? atoi(argv[2]) : 3840;
	int req_h = argc > 3 ? atoi(argv[3]) : 2160;
	int seconds = argc > 4 ? atoi(argv[4]) : 6;

	CHECK(CoInitializeEx(nullptr, COINIT_MULTITHREADED), "CoInitializeEx");

	ICreateDevEnum *devenum = nullptr;
	CHECK(CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
			       (void **)&devenum),
	      "device enumerator");
	IEnumMoniker *monikers = nullptr;
	if (devenum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &monikers, 0) != S_OK) {
		fprintf(stderr, "no video capture devices\n");
		return 1;
	}
	IBaseFilter *source = nullptr;
	IMoniker *m = nullptr;
	std::string chosen;
	while (!source && monikers->Next(1, &m, nullptr) == S_OK) {
		IPropertyBag *bag = nullptr;
		if (SUCCEEDED(m->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void **)&bag))) {
			VARIANT v;
			VariantInit(&v);
			if (SUCCEEDED(bag->Read(L"FriendlyName", &v, nullptr))) {
				char name[256];
				WideCharToMultiByte(CP_UTF8, 0, v.bstrVal, -1, name, sizeof(name), nullptr, nullptr);
				if (strstr(name, want.c_str()) &&
				    SUCCEEDED(m->BindToObject(nullptr, nullptr, IID_IBaseFilter, (void **)&source)))
					chosen = name;
				VariantClear(&v);
			}
			bag->Release();
		}
		m->Release();
	}
	if (!source) {
		fprintf(stderr, "no capture device whose name contains \"%s\"\n", want.c_str());
		return 1;
	}
	printf("device: %s\n", chosen.c_str());

	IGraphBuilder *graph = nullptr;
	CHECK(CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&graph),
	      "filter graph");
	ICaptureGraphBuilder2 *builder = nullptr;
	CHECK(CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2,
			       (void **)&builder),
	      "capture graph builder");
	builder->SetFiltergraph(graph);
	CHECK(graph->AddFilter(source, L"source"), "add source");

	IAMStreamConfig *config = nullptr;
	CHECK(builder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, source, IID_IAMStreamConfig,
				     (void **)&config),
	      "capture pin stream config");

	/* list the device's formats and pick the P210 entry */
	int count = 0, size = 0;
	config->GetNumberOfCapabilities(&count, &size);
	std::vector<BYTE> caps(size);
	AM_MEDIA_TYPE *p210 = nullptr;
	printf("formats offered by the capture pin:\n");
	for (int i = 0; i < count; i++) {
		AM_MEDIA_TYPE *mt = nullptr;
		if (FAILED(config->GetStreamCaps(i, &mt, caps.data())))
			continue;
		VIDEO_STREAM_CONFIG_CAPS *c = (VIDEO_STREAM_CONFIG_CAPS *)caps.data();
		REFERENCE_TIME *avg = nullptr;
		BITMAPINFOHEADER *b = bmi_of(mt, &avg);
		printf("  %2d %-4s %s  default %ldx%ld %u bpp, range %ldx%ld..%ldx%ld\n", i,
		       fourcc_str(mt->subtype).c_str(), mt->formattype == FORMAT_VideoInfo2 ? "VIH2" : "VIH ",
		       b ? b->biWidth : 0, b ? b->biHeight : 0, b ? b->biBitCount : 0, c->MinOutputSize.cx,
		       c->MinOutputSize.cy, c->MaxOutputSize.cx, c->MaxOutputSize.cy);
		if (!p210 && mt->subtype == fourcc_guid(FOURCC_P210) && b)
			p210 = mt;
		else
			free_mt(mt);
	}
	if (!p210) {
		fprintf(stderr, "the device does not offer P210\n");
		return 1;
	}

	REFERENCE_TIME *avg = nullptr;
	BITMAPINFOHEADER *bmi = bmi_of(p210, &avg);
	bmi->biWidth = req_w;
	bmi->biHeight = req_h;
	bmi->biSizeImage = (DWORD)req_w * req_h * 4;
	p210->lSampleSize = bmi->biSizeImage;
	*avg = 333667; /* 29.97 fps */
	HRESULT hr = config->SetFormat(p210);
	printf("SetFormat P210 %dx%d 29.97: %s (0x%08lx)\n", req_w, req_h, SUCCEEDED(hr) ? "accepted" : "rejected",
	       (long)hr);
	if (FAILED(hr))
		return 1;

	IBaseFilter *grab_filter = nullptr, *null_filter = nullptr;
	CHECK(CoCreateInstance(CLSID_SampleGrabber_, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter,
			       (void **)&grab_filter),
	      "sample grabber (qedit.dll)");
	CHECK(CoCreateInstance(CLSID_NullRenderer_, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter,
			       (void **)&null_filter),
	      "null renderer");
	ISampleGrabber *grabber = nullptr;
	CHECK(grab_filter->QueryInterface(IID_ISampleGrabber_, (void **)&grabber), "sample grabber interface");
	AM_MEDIA_TYPE want_mt = {};
	want_mt.majortype = MEDIATYPE_Video;
	want_mt.subtype = fourcc_guid(FOURCC_P210);
	CHECK(grabber->SetMediaType(&want_mt), "grabber media type");
	Grabber cb;
	cb.width = req_w;
	cb.height = req_h;
	grabber->SetBufferSamples(FALSE);
	grabber->SetOneShot(FALSE);
	CHECK(grabber->SetCallback(&cb, 1), "grabber callback");
	CHECK(graph->AddFilter(grab_filter, L"grabber"), "add grabber");
	CHECK(graph->AddFilter(null_filter, L"null"), "add null renderer");
	CHECK(builder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, source, grab_filter, null_filter),
	      "connect source -> grabber -> null renderer");

	AM_MEDIA_TYPE connected = {};
	if (SUCCEEDED(grabber->GetConnectedMediaType(&connected))) {
		REFERENCE_TIME *cavg = nullptr;
		BITMAPINFOHEADER *cb_bmi = bmi_of(&connected, &cavg);
		printf("connected as %s %ldx%ld, %.3f fps, sample size %lu\n", fourcc_str(connected.subtype).c_str(),
		       cb_bmi ? cb_bmi->biWidth : 0, cb_bmi ? cb_bmi->biHeight : 0,
		       cavg && *cavg ? 1e7 / (double)*cavg : 0.0, connected.lSampleSize);
		if (connected.cbFormat)
			CoTaskMemFree(connected.pbFormat);
	}

	IMediaControl *control = nullptr;
	CHECK(graph->QueryInterface(IID_IMediaControl, (void **)&control), "media control");
	CHECK(control->Run(), "run graph");
	printf("capturing for %d s...\n", seconds);
	Sleep(seconds * 1000);
	control->Stop();

	const double span = cb.last_time - cb.first_time;
	printf("\nframes received: %ld (%.2f fps over %.2f s of sample time), analysed: %ld, buffer %ld bytes, "
	       "short buffers: %ld\n",
	       cb.frames.load(), span > 0 ? (cb.frames - 1) / span : 0.0, span, cb.analysed.load(), cb.last_len,
	       cb.bad_size.load());
	report("Y   ", cb.y);
	report("CbCr", cb.uv);

	if (!cb.preview.empty()) {
		FILE *f = fopen("p210-preview.pgm", "wb");
		if (f) {
			fprintf(f, "P5\n%d %d\n255\n", req_w / 4, req_h / 4);
			fwrite(cb.preview.data(), 1, cb.preview.size(), f);
			fclose(f);
			printf("wrote p210-preview.pgm (luma, 1/4 size, 8-bit)\n");
		}
	}

	if (cb.y.samples) {
		const bool msb = cb.y.low6_nonzero == 0 && cb.uv.low6_nonzero == 0;
		const bool fine = cb.y.distinct_fine() > 0 || cb.uv.distinct_fine() > 0;
		const bool repl = cb.y.replicated == cb.y.samples && cb.uv.replicated == cb.uv.samples;
		printf("\nverdict: ");
		if (!msb)
			printf("data is not MSB-aligned P210 (lowest 6 bits used): check the layout\n");
		else if (!fine)
			printf("8-bit data shifted up to 10 bits (lowest 2 bits always 0)\n");
		else if (repl)
			printf("8-bit data bit-replicated to 10 bits\n");
		else if (cb.y.distinct() > 256 || cb.uv.distinct() > 256)
			printf("real 10-bit detail (more than 256 distinct values per plane)\n");
		else
			printf("10-bit codes present but few distinct values: test again with camera footage\n");
	}
	return 0;
}
