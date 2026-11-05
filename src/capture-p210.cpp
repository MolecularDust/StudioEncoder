/* SE: 10-bit 4:2:2 capture (P210), Windows only.
 * Opens a DirectShow capture device in the P210 format (10-bit 4:2:2, luma plane + interleaved CbCr plane,
 * 16-bit samples with the 10 bits at the top) and hands each frame to OBS as I210 (three planes, 10 bits at the
 * bottom): a shift and a CbCr split, no loss. OBS 33 takes P216 frames from a source but has no shader to draw
 * them (it shows nothing), while I210 is drawn. OBS's own Video Capture Device source cannot do any of this:
 * its DirectShow library stops at P010 (4:2:0). Frames come from the Sample Grabber filter (qedit.dll), as in
 * tools/p210-probe.cpp. */
#include <obs-module.h>
#include <plugin-support.h>
#include <util/platform.h>

#include <windows.h>
#include <dshow.h>
#include <dvdmedia.h>
#include <wrl/client.h>
#include <emmintrin.h>

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using Microsoft::WRL::ComPtr;

/* qedit.h is no longer in the Windows SDK; qedit.dll still ships with Windows. */
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

static GUID p210_subtype()
{
	GUID g = {0x00000000, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};
	g.Data1 = MAKEFOURCC('P', '2', '1', '0');
	return g;
}

struct frame_rate {
	uint32_t num, den;
	const char *label;
};

static const frame_rate frame_rates[] = {
	{30000, 1001, "29.97"}, {30, 1, "30"},       {25, 1, "25"}, {24000, 1001, "23.976"},
	{24, 1, "24"},          {60000, 1001, "59.94"}, {60, 1, "60"}, {50, 1, "50"},
};

static void free_media_type(AM_MEDIA_TYPE *mt)
{
	if (!mt)
		return;
	if (mt->cbFormat)
		CoTaskMemFree(mt->pbFormat);
	if (mt->pUnk)
		mt->pUnk->Release();
	CoTaskMemFree(mt);
}

static BITMAPINFOHEADER *video_header(AM_MEDIA_TYPE *mt, REFERENCE_TIME **avg)
{
	if (mt->formattype == FORMAT_VideoInfo && mt->cbFormat >= sizeof(VIDEOINFOHEADER)) {
		auto *v = (VIDEOINFOHEADER *)mt->pbFormat;
		*avg = &v->AvgTimePerFrame;
		return &v->bmiHeader;
	}
	if (mt->formattype == FORMAT_VideoInfo2 && mt->cbFormat >= sizeof(VIDEOINFOHEADER2)) {
		auto *v = (VIDEOINFOHEADER2 *)mt->pbFormat;
		*avg = &v->AvgTimePerFrame;
		return &v->bmiHeader;
	}
	return nullptr;
}

static std::string bstr_to_utf8(BSTR s)
{
	char buf[512];
	WideCharToMultiByte(CP_UTF8, 0, s, -1, buf, sizeof(buf), nullptr, nullptr);
	return buf;
}

/* --- devices ---------------------------------------------------------------------------------- */

struct device_info {
	std::string name;
	std::string path; /* DevicePath, or the name if the device has none */
	ComPtr<IMoniker> moniker;
};

static std::vector<device_info> list_video_devices()
{
	std::vector<device_info> out;
	ComPtr<ICreateDevEnum> devenum;
	if (FAILED(CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&devenum))))
		return out;
	ComPtr<IEnumMoniker> monikers;
	if (devenum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &monikers, 0) != S_OK)
		return out;
	ComPtr<IMoniker> m;
	while (monikers->Next(1, &m, nullptr) == S_OK) {
		ComPtr<IPropertyBag> bag;
		if (SUCCEEDED(m->BindToStorage(nullptr, nullptr, IID_PPV_ARGS(&bag)))) {
			device_info d;
			VARIANT v;
			VariantInit(&v);
			if (SUCCEEDED(bag->Read(L"FriendlyName", &v, nullptr))) {
				d.name = bstr_to_utf8(v.bstrVal);
				VariantClear(&v);
			}
			if (SUCCEEDED(bag->Read(L"DevicePath", &v, nullptr))) {
				d.path = bstr_to_utf8(v.bstrVal);
				VariantClear(&v);
			}
			if (d.path.empty())
				d.path = d.name;
			d.moniker = m;
			if (!d.name.empty())
				out.push_back(d);
		}
		m.Reset();
	}
	return out;
}

/* Finds the P210 entry of a filter's capture pin (in a graph built with `builder`). */
static AM_MEDIA_TYPE *find_p210(ICaptureGraphBuilder2 *builder, IBaseFilter *filter, ComPtr<IAMStreamConfig> &config)
{
	if (FAILED(builder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, filter, IID_PPV_ARGS(&config))))
		return nullptr;
	int count = 0, size = 0;
	if (FAILED(config->GetNumberOfCapabilities(&count, &size)))
		return nullptr;
	std::vector<BYTE> caps(size);
	for (int i = 0; i < count; i++) {
		AM_MEDIA_TYPE *mt = nullptr;
		if (FAILED(config->GetStreamCaps(i, &mt, caps.data())))
			continue;
		REFERENCE_TIME *avg = nullptr;
		if (mt->subtype == p210_subtype() && video_header(mt, &avg))
			return mt;
		free_media_type(mt);
	}
	return nullptr;
}

static bool device_offers_p210(IMoniker *moniker)
{
	ComPtr<IBaseFilter> filter;
	ComPtr<IGraphBuilder> graph;
	ComPtr<ICaptureGraphBuilder2> builder;
	if (FAILED(moniker->BindToObject(nullptr, nullptr, IID_PPV_ARGS(&filter))) ||
	    FAILED(CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&graph))) ||
	    FAILED(CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&builder))))
		return false;
	builder->SetFiltergraph(graph.Get());
	if (FAILED(graph->AddFilter(filter.Get(), L"source")))
		return false;
	ComPtr<IAMStreamConfig> config;
	AM_MEDIA_TYPE *mt = find_p210(builder.Get(), filter.Get(), config);
	free_media_type(mt);
	graph->RemoveFilter(filter.Get());
	return mt != nullptr;
}

/* --- source ----------------------------------------------------------------------------------- */

struct capture_config {
	std::string device; /* device path; empty = first device that offers P210 */
	uint32_t width = 3840, height = 2160;
	frame_rate rate = frame_rates[0];
	video_colorspace colorspace = VIDEO_CS_709;
	video_range_type range = VIDEO_RANGE_PARTIAL;
};

struct p210_capture;

class FrameSink : public ISampleGrabberCB {
public:
	p210_capture *owner = nullptr;
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override
	{
		if (riid == IID_IUnknown || riid == IID_ISampleGrabberCB_) {
			*ppv = this;
			return S_OK;
		}
		*ppv = nullptr;
		return E_NOINTERFACE;
	}
	/* lifetime is owned by p210_capture, not by COM */
	STDMETHODIMP_(ULONG) AddRef() override { return 2; }
	STDMETHODIMP_(ULONG) Release() override { return 1; }
	STDMETHODIMP SampleCB(double, IMediaSample *) override { return E_NOTIMPL; }
	STDMETHODIMP BufferCB(double time, BYTE *buf, long len) override;
};

struct p210_capture {
	obs_source_t *source = nullptr;

	/* worker thread: owns COM and the DirectShow graph */
	std::thread worker;
	std::mutex lock;
	std::condition_variable wake;
	bool restart = false;
	bool quit = false;
	capture_config pending; /* guarded by lock */

	/* graph state (worker thread only, read by the streaming thread through `running`) */
	ComPtr<IGraphBuilder> graph;
	ComPtr<IMediaControl> control;
	ComPtr<ISampleGrabber> grabber;
	FrameSink sink;
	std::atomic<bool> running{false};
	uint32_t width = 0, height = 0;
	uint64_t base_ns = 0;
	float color_matrix[16];
	float range_min[3], range_max[3];
	bool full_range = false;
	video_colorspace colorspace = VIDEO_CS_709;
	std::atomic<uint64_t> frames{0};
	std::atomic<uint64_t> short_buffers{0};
	std::atomic<uint64_t> convert_ns{0};
	std::vector<uint16_t> i210; /* converted frame: Y, then Cb, then Cr (streaming thread only) */
};

/* P210 row (16-bit, 10 bits at the top) -> I210 row (10 bits at the bottom) */
static void shift_row(const uint16_t *src, uint16_t *dst, uint32_t n)
{
	uint32_t i = 0;
	for (; i + 8 <= n; i += 8)
		_mm_storeu_si128((__m128i *)(dst + i), _mm_srli_epi16(_mm_loadu_si128((const __m128i *)(src + i)), 6));
	for (; i < n; i++)
		dst[i] = src[i] >> 6;
}

/* P210 interleaved CbCr row -> separate I210 Cb and Cr rows, with the same shift. After the shift every value
 * fits in 15 bits, so the signed 32-to-16-bit pack is exact. */
static void split_row(const uint16_t *src, uint16_t *cb, uint16_t *cr, uint32_t pairs)
{
	uint32_t i = 0;
	for (; i + 8 <= pairs; i += 8) {
		const __m128i a = _mm_srli_epi16(_mm_loadu_si128((const __m128i *)(src + 2 * i)), 6);
		const __m128i b = _mm_srli_epi16(_mm_loadu_si128((const __m128i *)(src + 2 * i + 8)), 6);
		const __m128i a_lo = _mm_srai_epi32(_mm_slli_epi32(a, 16), 16);
		const __m128i b_lo = _mm_srai_epi32(_mm_slli_epi32(b, 16), 16);
		_mm_storeu_si128((__m128i *)(cb + i), _mm_packs_epi32(a_lo, b_lo));
		_mm_storeu_si128((__m128i *)(cr + i), _mm_packs_epi32(_mm_srai_epi32(a, 16), _mm_srai_epi32(b, 16)));
	}
	for (; i < pairs; i++) {
		cb[i] = src[2 * i] >> 6;
		cr[i] = src[2 * i + 1] >> 6;
	}
}

STDMETHODIMP FrameSink::BufferCB(double time, BYTE *buf, long len)
{
	p210_capture *c = owner;
	if (!c || !c->running)
		return S_OK;
	/* P210: luma plane (height rows) then the interleaved CbCr plane (height rows) */
	const long rows = (long)c->height * 2;
	const long stride = len / rows;
	if (stride < (long)c->width * 2) {
		if (c->short_buffers++ == 0)
			obs_log(LOG_WARNING, "p210: buffer of %ld bytes is too small for %ux%u P210; frame dropped", len,
				c->width, c->height);
		return S_OK;
	}
	const uint64_t t0 = os_gettime_ns();
	const uint32_t w = c->width, h = c->height, half = w / 2;
	c->i210.resize((size_t)w * h * 2);
	uint16_t *y = c->i210.data();
	uint16_t *cb = y + (size_t)w * h;
	uint16_t *cr = cb + (size_t)half * h;
	for (uint32_t r = 0; r < h; r++) {
		shift_row((const uint16_t *)(buf + (size_t)stride * r), y + (size_t)w * r, w);
		split_row((const uint16_t *)(buf + (size_t)stride * (h + r)), cb + (size_t)half * r,
			  cr + (size_t)half * r, half);
	}
	c->convert_ns += os_gettime_ns() - t0;

	obs_source_frame frame = {};
	frame.format = VIDEO_FORMAT_I210;
	frame.width = w;
	frame.height = h;
	frame.data[0] = (uint8_t *)y;
	frame.linesize[0] = w * 2;
	frame.data[1] = (uint8_t *)cb;
	frame.linesize[1] = half * 2;
	frame.data[2] = (uint8_t *)cr;
	frame.linesize[2] = half * 2;
	frame.timestamp = c->base_ns + (uint64_t)(time * 1e9);
	frame.full_range = c->full_range;
	memcpy(frame.color_matrix, c->color_matrix, sizeof(frame.color_matrix));
	memcpy(frame.color_range_min, c->range_min, sizeof(frame.color_range_min));
	memcpy(frame.color_range_max, c->range_max, sizeof(frame.color_range_max));
	obs_source_output_video(c->source, &frame); /* OBS copies the frame */
	c->frames++;
	return S_OK;
}

static void stop_graph(p210_capture *c)
{
	if (!c->graph)
		return;
	c->running = false;
	if (c->control)
		c->control->Stop();
	if (c->grabber)
		c->grabber->SetCallback(nullptr, 1);
	const uint64_t n = c->frames.load();
	obs_log(LOG_INFO, "p210: stopped after %llu frames, P210 to I210 conversion %.2f ms per frame%s",
		(unsigned long long)n, n ? c->convert_ns.load() / 1e6 / (double)n : 0.0,
		c->short_buffers ? " (some frames dropped: buffer too small)" : "");
	c->grabber.Reset();
	c->control.Reset();
	c->graph.Reset();
	if (!c->quit) /* clear the last frame, but not while the source is being destroyed */
		obs_source_output_video(c->source, nullptr);
}

static bool start_graph(p210_capture *c, const capture_config &cfg)
{
	std::vector<device_info> devices = list_video_devices();
	const device_info *dev = nullptr;
	for (const device_info &d : devices) {
		if (cfg.device.empty() ? device_offers_p210(d.moniker.Get()) : d.path == cfg.device) {
			dev = &d;
			break;
		}
	}
	if (!dev) {
		obs_log(LOG_WARNING, "p210: %s", cfg.device.empty() ? "no capture device offers P210"
							       : "the selected capture device is not connected");
		return false;
	}

	ComPtr<IBaseFilter> source, grab_filter, null_filter;
	ComPtr<ICaptureGraphBuilder2> builder;
	ComPtr<IAMStreamConfig> config;
	HRESULT hr = dev->moniker->BindToObject(nullptr, nullptr, IID_PPV_ARGS(&source));
	if (SUCCEEDED(hr))
		hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&c->graph));
	if (SUCCEEDED(hr))
		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&builder));
	if (SUCCEEDED(hr))
		hr = CoCreateInstance(CLSID_SampleGrabber_, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&grab_filter));
	if (SUCCEEDED(hr))
		hr = CoCreateInstance(CLSID_NullRenderer_, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&null_filter));
	if (SUCCEEDED(hr))
		hr = grab_filter->QueryInterface(IID_ISampleGrabber_, (void **)c->grabber.GetAddressOf());
	if (FAILED(hr)) {
		obs_log(LOG_WARNING, "p210: could not create the DirectShow graph (0x%08lx)", (long)hr);
		c->graph.Reset();
		c->grabber.Reset();
		return false;
	}
	builder->SetFiltergraph(c->graph.Get());
	c->graph->AddFilter(source.Get(), L"source");

	AM_MEDIA_TYPE *mt = find_p210(builder.Get(), source.Get(), config);
	if (!mt) {
		obs_log(LOG_WARNING, "p210: %s does not offer P210", dev->name.c_str());
		c->graph.Reset();
		c->grabber.Reset();
		return false;
	}
	REFERENCE_TIME *avg = nullptr;
	BITMAPINFOHEADER *bmi = video_header(mt, &avg);
	bmi->biWidth = (LONG)cfg.width;
	bmi->biHeight = (LONG)cfg.height;
	bmi->biSizeImage = cfg.width * cfg.height * 4;
	mt->lSampleSize = bmi->biSizeImage;
	*avg = (REFERENCE_TIME)(10000000ULL * cfg.rate.den / cfg.rate.num);
	hr = config->SetFormat(mt);
	free_media_type(mt);
	if (FAILED(hr)) {
		obs_log(LOG_WARNING, "p210: %s rejected P210 %ux%u at %s fps (0x%08lx)", dev->name.c_str(), cfg.width,
			cfg.height, cfg.rate.label, (long)hr);
		c->graph.Reset();
		c->grabber.Reset();
		return false;
	}

	AM_MEDIA_TYPE want = {};
	want.majortype = MEDIATYPE_Video;
	want.subtype = p210_subtype();
	c->grabber->SetMediaType(&want);
	c->grabber->SetBufferSamples(FALSE);
	c->grabber->SetOneShot(FALSE);
	c->sink.owner = c;
	c->grabber->SetCallback(&c->sink, 1);
	c->graph->AddFilter(grab_filter.Get(), L"grabber");
	c->graph->AddFilter(null_filter.Get(), L"null");
	hr = builder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, source.Get(), grab_filter.Get(),
				   null_filter.Get());
	if (SUCCEEDED(hr))
		hr = c->graph.As(&c->control);
	if (FAILED(hr)) {
		obs_log(LOG_WARNING, "p210: could not connect %s (0x%08lx)", dev->name.c_str(), (long)hr);
		c->grabber->SetCallback(nullptr, 1);
		c->graph.Reset();
		c->grabber.Reset();
		c->control.Reset();
		return false;
	}
	c->width = cfg.width;
	c->height = cfg.height;
	c->colorspace = cfg.colorspace;
	c->full_range = cfg.range == VIDEO_RANGE_FULL;
	video_format_get_parameters_for_format(cfg.colorspace, cfg.range, VIDEO_FORMAT_I210, c->color_matrix,
					       c->range_min, c->range_max);
	c->frames = 0;
	c->short_buffers = 0;
	c->convert_ns = 0;
	c->base_ns = os_gettime_ns();
	c->running = true;
	hr = c->control->Run();
	if (FAILED(hr)) {
		obs_log(LOG_WARNING, "p210: %s did not start (0x%08lx)", dev->name.c_str(), (long)hr);
		stop_graph(c);
		return false;
	}
	obs_log(LOG_INFO, "p210: capturing %s as P210 %ux%u at %s fps, %s %s range (passed to OBS as I210)",
		dev->name.c_str(), cfg.width, cfg.height, cfg.rate.label,
		cfg.colorspace == VIDEO_CS_601 ? "Rec. 601" : "Rec. 709", c->full_range ? "full" : "limited");
	return true;
}

static void worker_main(p210_capture *c)
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	for (;;) {
		capture_config cfg;
		{
			std::unique_lock<std::mutex> g(c->lock);
			c->wake.wait(g, [c] { return c->restart || c->quit; });
			if (c->quit)
				break;
			c->restart = false;
			cfg = c->pending;
		}
		stop_graph(c);
		start_graph(c, cfg);
	}
	stop_graph(c);
	CoUninitialize();
}

static capture_config read_config(obs_data_t *settings)
{
	capture_config cfg;
	cfg.device = obs_data_get_string(settings, "device");
	unsigned w = 0, h = 0;
	if (sscanf(obs_data_get_string(settings, "resolution"), "%ux%u", &w, &h) == 2 && w >= 64 && h >= 64 &&
	    w % 2 == 0) {
		cfg.width = w;
		cfg.height = h;
	}
	const long long rate = obs_data_get_int(settings, "frame_rate");
	if (rate >= 0 && rate < (long long)(sizeof(frame_rates) / sizeof(frame_rates[0])))
		cfg.rate = frame_rates[rate];
	cfg.colorspace = obs_data_get_int(settings, "color_space") == 601 ? VIDEO_CS_601 : VIDEO_CS_709;
	cfg.range = obs_data_get_int(settings, "color_range") == 1 ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
	return cfg;
}

static void p210_update(void *data, obs_data_t *settings)
{
	auto *c = (p210_capture *)data;
	std::lock_guard<std::mutex> g(c->lock);
	c->pending = read_config(settings);
	c->restart = true;
	c->wake.notify_one();
}

static void *p210_create(obs_data_t *settings, obs_source_t *source)
{
	auto *c = new p210_capture;
	c->source = source;
	c->worker = std::thread(worker_main, c);
	p210_update(c, settings);
	return c;
}

static void p210_destroy(void *data)
{
	auto *c = (p210_capture *)data;
	{
		std::lock_guard<std::mutex> g(c->lock);
		c->quit = true;
		c->wake.notify_one();
	}
	c->worker.join();
	delete c;
}

static const char *p210_get_name(void *)
{
	return obs_module_text("Source.P210");
}

static void p210_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "device", "");
	obs_data_set_default_string(settings, "resolution", "3840x2160");
	obs_data_set_default_int(settings, "frame_rate", 0);
	obs_data_set_default_int(settings, "color_space", 709);
	obs_data_set_default_int(settings, "color_range", 0);
}

static obs_properties_t *p210_get_properties(void *)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *dev = obs_properties_add_list(props, "device", obs_module_text("P210.Device"),
						      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(dev, obs_module_text("P210.Device.Auto"), "");
	/* properties are built on the UI thread; DirectShow enumeration needs COM there too */
	const HRESULT co = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	for (const device_info &d : list_video_devices())
		if (device_offers_p210(d.moniker.Get()))
			obs_property_list_add_string(dev, d.name.c_str(), d.path.c_str());
	if (SUCCEEDED(co))
		CoUninitialize();

	obs_property_t *res = obs_properties_add_list(props, "resolution", obs_module_text("P210.Resolution"),
						      OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
	for (const char *r : {"3840x2160", "4096x2160", "2560x1440", "1920x1080", "1280x720"})
		obs_property_list_add_string(res, r, r);

	obs_property_t *fps = obs_properties_add_list(props, "frame_rate", obs_module_text("P210.FrameRate"),
						      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	for (size_t i = 0; i < sizeof(frame_rates) / sizeof(frame_rates[0]); i++)
		obs_property_list_add_int(fps, frame_rates[i].label, (long long)i);

	obs_property_t *cs = obs_properties_add_list(props, "color_space", obs_module_text("P210.ColorSpace"),
						     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(cs, "Rec. 709", 709);
	obs_property_list_add_int(cs, "Rec. 601", 601);

	obs_property_t *range = obs_properties_add_list(props, "color_range", obs_module_text("P210.ColorRange"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(range, obs_module_text("P210.Range.Limited"), 0);
	obs_property_list_add_int(range, obs_module_text("P210.Range.Full"), 1);

	obs_properties_add_text(props, "info", obs_module_text("P210.Info"), OBS_TEXT_INFO);
	return props;
}

extern "C" void se_register_p210_capture(void)
{
	obs_source_info info = {};
	info.id = "studioencoder_p210_capture";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_DO_NOT_DUPLICATE;
	info.icon_type = OBS_ICON_TYPE_CAMERA;
	info.get_name = p210_get_name;
	info.create = p210_create;
	info.destroy = p210_destroy;
	info.update = p210_update;
	info.get_defaults = p210_get_defaults;
	info.get_properties = p210_get_properties;
	obs_register_source(&info);
}
