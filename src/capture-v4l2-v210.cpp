/* Linux capture for drivers that expose packed v210 through V4L2. */
#include <obs-module.h>
#include <plugin-support.h>
#include <util/platform.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <glob.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atomic>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
constexpr uint32_t v210 = v4l2_fourcc('v', '2', '1', '0');
constexpr auto capture_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

int control(int fd, unsigned long request, void *arg)
{
	int result;
	do {
		result = ioctl(fd, request, arg);
	} while (result < 0 && errno == EINTR);
	return result;
}

struct mapping {
	void *data;
	size_t size;
};

struct device {
	int fd;
	std::vector<mapping> buffers;
	bool streaming = false;

	explicit device(const char *path) : fd(open(path, O_RDWR | O_NONBLOCK | O_CLOEXEC)) {}
	~device()
	{
		if (streaming) {
			auto type = capture_type;
			control(fd, VIDIOC_STREAMOFF, &type);
		}
		for (const auto &buffer : buffers)
			munmap(buffer.data, buffer.size);
		if (fd >= 0)
			close(fd);
	}

	bool offers_v210() const
	{
		v4l2_fmtdesc desc = {};
		desc.type = capture_type;
		for (; control(fd, VIDIOC_ENUM_FMT, &desc) == 0; ++desc.index) {
			if (desc.pixelformat == v210)
				return true;
		}
		return false;
	}

	bool start()
	{
		v4l2_requestbuffers request = {};
		request.count = 4;
		request.type = capture_type;
		request.memory = V4L2_MEMORY_MMAP;
		if (control(fd, VIDIOC_REQBUFS, &request) < 0 || request.count == 0)
			return false;

		for (uint32_t i = 0; i < request.count; ++i) {
			v4l2_buffer buffer = {};
			buffer.index = i;
			buffer.type = capture_type;
			buffer.memory = V4L2_MEMORY_MMAP;
			if (control(fd, VIDIOC_QUERYBUF, &buffer) < 0)
				return false;
			void *data = mmap(nullptr, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
					  buffer.m.offset);
			if (data == MAP_FAILED)
				return false;
			buffers.push_back({data, buffer.length});
			if (control(fd, VIDIOC_QBUF, &buffer) < 0)
				return false;
		}

		auto type = capture_type;
		streaming = control(fd, VIDIOC_STREAMON, &type) == 0;
		return streaming;
	}
};

struct capture {
	obs_source_t *source;
	std::atomic<bool> stopping{false};
	std::thread worker;

	explicit capture(obs_source_t *source_) : source(source_) {}

	void stop()
	{
		stopping = true;
		if (worker.joinable())
			worker.join();
	}

	void run(const std::string &path, uint32_t width, uint32_t height, uint32_t rate_num, uint32_t rate_den,
		 video_colorspace colorspace, video_range_type range)
	{
		device input(path.c_str());
		if (input.fd < 0 || !input.offers_v210()) {
			obs_log(LOG_WARNING, "v210 V4L2: %s could not be opened or does not advertise v210",
				path.c_str());
			return;
		}
		v4l2_format format = {};
		format.type = capture_type;
		auto &pixel = format.fmt.pix;
		pixel.width = width;
		pixel.height = height;
		pixel.pixelformat = v210;
		pixel.field = V4L2_FIELD_NONE;
		if (control(input.fd, VIDIOC_S_FMT, &format) < 0 || pixel.pixelformat != v210 ||
		    pixel.width != width || pixel.height != height || pixel.field != V4L2_FIELD_NONE ||
		    pixel.bytesperline < ((width + 5) / 6) * 16) {
			obs_log(LOG_WARNING, "v210 V4L2: %s rejected progressive %ux%u v210", path.c_str(),
				width, height);
			return;
		}

		v4l2_streamparm parameters = {};
		parameters.type = capture_type;
		auto &timing = parameters.parm.capture;
		if (control(input.fd, VIDIOC_G_PARM, &parameters) == 0 &&
		    (timing.capability & V4L2_CAP_TIMEPERFRAME)) {
			timing.timeperframe = {rate_den, rate_num};
			if (control(input.fd, VIDIOC_S_PARM, &parameters) < 0 ||
			    !timing.timeperframe.numerator || !timing.timeperframe.denominator ||
			    uint64_t(timing.timeperframe.numerator) * rate_num !=
				    uint64_t(timing.timeperframe.denominator) * rate_den) {
				obs_log(LOG_WARNING, "v210 V4L2: %s rejected %u/%u fps", path.c_str(),
					rate_num, rate_den);
				return;
			}
		} else {
			obs_log(LOG_INFO, "v210 V4L2: %s uses the incoming signal's frame rate", path.c_str());
		}

		obs_source_frame frame = {};
		frame.format = VIDEO_FORMAT_V210;
		frame.width = width;
		frame.height = height;
		frame.linesize[0] = pixel.bytesperline;
		frame.full_range = range == VIDEO_RANGE_FULL;
		video_format_get_parameters_for_format(colorspace, range, frame.format, frame.color_matrix,
						       frame.color_range_min, frame.color_range_max);

		if (!input.start()) {
			obs_log(LOG_WARNING, "v210 V4L2: could not start %s: %s", path.c_str(), strerror(errno));
			return;
		}
		obs_log(LOG_INFO, "v210 V4L2: capturing %s at %ux%u", path.c_str(), width, height);
		pollfd ready = {input.fd, POLLIN, 0};
		const uint64_t required = uint64_t(pixel.bytesperline) * height;
		while (!stopping) {
			const int result = poll(&ready, 1, 200);
			if (result == 0 || (result < 0 && errno == EINTR))
				continue;
			if (result < 0 || (ready.revents & (POLLERR | POLLHUP | POLLNVAL)))
				break;

			v4l2_buffer buffer = {};
			buffer.type = capture_type;
			buffer.memory = V4L2_MEMORY_MMAP;
			if (control(input.fd, VIDIOC_DQBUF, &buffer) < 0) {
				if (errno == EAGAIN)
					continue;
				break;
			}
			if (buffer.index >= input.buffers.size())
				break;
			const auto &mapped = input.buffers[buffer.index];
			if (!(buffer.flags & V4L2_BUF_FLAG_ERROR) && buffer.bytesused >= required &&
			    buffer.bytesused <= mapped.size) {
				frame.data[0] = static_cast<uint8_t *>(mapped.data);
				frame.timestamp = (buffer.flags & V4L2_BUF_FLAG_TIMESTAMP_MASK) ==
							  V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC
							  ? uint64_t(buffer.timestamp.tv_sec) * 1000000000 +
								    uint64_t(buffer.timestamp.tv_usec) * 1000
							  : os_gettime_ns();
				obs_source_output_video(source, &frame);
			}
			if (control(input.fd, VIDIOC_QBUF, &buffer) < 0)
				break;
		}
		if (!stopping) {
			obs_log(LOG_WARNING, "v210 V4L2: %s stopped delivering frames", path.c_str());
			obs_source_output_video(source, nullptr);
		}
	}
};

const char *get_name(void *)
{
	return obs_module_text("Source.V210.V4L2");
}

void defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "resolution", "3840x2160");
	obs_data_set_default_string(settings, "frame_rate", "30000/1001");
	obs_data_set_default_int(settings, "color_space", 709);
}

void update(void *data, obs_data_t *settings)
{
	auto *source = static_cast<capture *>(data);
	source->stop();
	obs_source_output_video(source->source, nullptr);
	const std::string path = obs_data_get_string(settings, "device");
	if (path.empty())
		return;

	unsigned width, height, num, den;
	if (sscanf(obs_data_get_string(settings, "resolution"), "%ux%u", &width, &height) != 2 ||
	    width < 2 || width > 16384 || width % 2 || height < 1 || height > 16384 ||
	    sscanf(obs_data_get_string(settings, "frame_rate"), "%u/%u", &num, &den) != 2 || !num || !den)
		return;

	const auto colorspace = obs_data_get_int(settings, "color_space") == 601 ? VIDEO_CS_601 : VIDEO_CS_709;
	const auto range = obs_data_get_int(settings, "color_range") ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
	source->stopping = false;
	source->worker = std::thread(&capture::run, source, path, width, height, num, den, colorspace, range);
}

void *create(obs_data_t *settings, obs_source_t *source)
{
	auto *instance = new capture{source};
	update(instance, settings);
	return instance;
}

void destroy(void *data)
{
	auto *source = static_cast<capture *>(data);
	source->stop();
	delete source;
}

obs_properties_t *properties(void *)
{
	auto *props = obs_properties_create();
	auto *devices = obs_properties_add_list(props, "device", obs_module_text("P210.Device"),
						OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
	glob_t nodes = {};
	if (glob("/dev/video[0-9]*", 0, nullptr, &nodes) == 0) {
		for (size_t i = 0; i < nodes.gl_pathc; ++i) {
			const char *path = nodes.gl_pathv[i];
			device input(path);
			if (input.fd >= 0 && input.offers_v210())
				obs_property_list_add_string(devices, path, path);
		}
	}
	globfree(&nodes);

	auto *res = obs_properties_add_list(props, "resolution", obs_module_text("P210.Resolution"),
					    OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
	for (const char *value : {"3840x2160", "4096x2160", "1920x1080", "1280x720"})
		obs_property_list_add_string(res, value, value);

	auto *fps = obs_properties_add_list(props, "frame_rate", obs_module_text("P210.FrameRate"),
					    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	const std::pair<const char *, const char *> rates[] = {
		{"23.976", "24000/1001"}, {"24", "24/1"}, {"25", "25/1"}, {"29.97", "30000/1001"},
		{"30", "30/1"}, {"50", "50/1"}, {"59.94", "60000/1001"}, {"60", "60/1"},
	};
	for (const auto &rate : rates)
		obs_property_list_add_string(fps, rate.first, rate.second);

	auto *cs = obs_properties_add_list(props, "color_space", obs_module_text("P210.ColorSpace"),
					   OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(cs, "Rec. 709", 709);
	obs_property_list_add_int(cs, "Rec. 601", 601);
	auto *range = obs_properties_add_list(props, "color_range", obs_module_text("P210.ColorRange"),
					      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(range, obs_module_text("P210.Range.Limited"), 0);
	obs_property_list_add_int(range, obs_module_text("P210.Range.Full"), 1);
	return props;
}
} // namespace

extern "C" void se_register_v4l2_v210_capture(void)
{
	obs_source_info info = {};
	info.id = "studioencoder_v4l2_v210_capture";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_DO_NOT_DUPLICATE;
	info.icon_type = OBS_ICON_TYPE_CAMERA;
	info.get_name = get_name;
	info.get_defaults = defaults;
	info.get_properties = properties;
	info.create = create;
	info.update = update;
	info.destroy = destroy;
	obs_register_source(&info);
}
