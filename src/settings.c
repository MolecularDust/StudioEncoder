#include "settings.h"

#include <plugin-support.h>
#include <util/bmem.h>
#include <util/platform.h>

extern struct obs_encoder_info pr_rust_encoder_info;
extern struct obs_encoder_info pr_ffmpeg_aw_encoder_info;
extern struct obs_encoder_info pr_awplus_encoder_info;
extern struct obs_encoder_info pr_nvenc_h264_encoder_info;
extern struct obs_encoder_info pr_oxideav_encoder_info;
extern struct obs_encoder_info pr_amd_hevc_encoder_info;
extern struct obs_encoder_info pr_nvenc_hevc_encoder_info;

bool pr_nvenc_h264_422_probe(char *reason, size_t reason_len);
bool pr_amd_hevc_probe(char *reason, size_t reason_len);
bool pr_nvenc_hevc_probe(char *reason, size_t reason_len);

static const struct pr_encoder_entry entries[] = {
	{"obs_prores_ffmpeg_aw", "Desc.AwPlus", false, &pr_awplus_encoder_info, NULL, NULL},
	{"obs_prores_nvenc_h264_422_intra", "Desc.NvencH264", false, &pr_nvenc_h264_encoder_info, pr_nvenc_h264_422_probe,
	 "Settings.Unavailable"},
	{"obs_prores_nvenc_hevc_intra", "Desc.NvencHevc", false, &pr_nvenc_hevc_encoder_info, pr_nvenc_hevc_probe,
	 "Settings.UnavailableNvencHevc", true},
	{"obs_prores_amd_hevc_intra", "Desc.AmdHevc", false, &pr_amd_hevc_encoder_info, pr_amd_hevc_probe,
	 "Settings.UnavailableAmd", true},
	{"obs_prores_ffmpeg_aw_original", "Desc.FFmpegAw", true, &pr_ffmpeg_aw_encoder_info, NULL},
	{"obs_prores_rust", "Desc.Rust", true, &pr_rust_encoder_info, NULL},
	{"obs_prores_oxideav", "Desc.OxideAV", true, &pr_oxideav_encoder_info, NULL},
};
#define ENTRY_COUNT (sizeof(entries) / sizeof(entries[0]))

/* -1 = not saved yet: use the default (on, except for experimental encoders); set in pr_settings_load */
static int saved[ENTRY_COUNT];

/* false when the start-up hardware check failed */
static bool available[ENTRY_COUNT];

const struct pr_encoder_entry *pr_encoder_entries(size_t *count)
{
	*count = ENTRY_COUNT;
	return entries;
}

static size_t index_of(const struct pr_encoder_entry *entry)
{
	return (size_t)(entry - entries);
}

bool pr_encoder_available(const struct pr_encoder_entry *entry)
{
	return available[index_of(entry)];
}

bool pr_encoder_enabled(const struct pr_encoder_entry *entry)
{
	const int v = saved[index_of(entry)];
	return v < 0 ? !entry->experimental : v != 0;
}

void pr_encoder_set_enabled(const struct pr_encoder_entry *entry, bool enabled)
{
	saved[index_of(entry)] = enabled ? 1 : 0;
}

static void run_probes(void)
{
	for (size_t i = 0; i < ENTRY_COUNT; i++) {
		if (!entries[i].probe)
			continue;
		char reason[256] = "";
		const uint64_t t0 = os_gettime_ns();
		available[i] = entries[i].probe(reason, sizeof(reason));
		const double ms = (double)(os_gettime_ns() - t0) / 1e6;
		if (available[i])
			obs_log(LOG_INFO, "%s: hardware check passed (%.0f ms)", entries[i].id, ms);
		else
			obs_log(LOG_INFO, "%s: not available on this system, hidden (%.0f ms): %s", entries[i].id, ms,
				reason);
	}
}

void pr_settings_load(void)
{
	for (size_t i = 0; i < ENTRY_COUNT; i++) {
		saved[i] = -1;
		available[i] = true;
	}
	run_probes();

	char *path = obs_module_config_path("settings.json");
	if (!path)
		return;
	obs_data_t *data = obs_data_create_from_json_file(path);
	bfree(path);
	if (!data)
		return;

	obs_data_t *enabled = obs_data_get_obj(data, "enabled");
	if (enabled) {
		for (size_t i = 0; i < ENTRY_COUNT; i++) {
			if (obs_data_has_user_value(enabled, entries[i].id))
				saved[i] = obs_data_get_bool(enabled, entries[i].id) ? 1 : 0;
		}
		obs_data_release(enabled);
	}
	obs_data_release(data);
}

void pr_settings_save(void)
{
	char *dir = obs_module_config_path("");
	if (dir) {
		os_mkdirs(dir);
		bfree(dir);
	}
	char *path = obs_module_config_path("settings.json");
	if (!path)
		return;

	obs_data_t *data = obs_data_create();
	obs_data_t *enabled = obs_data_create();
	for (size_t i = 0; i < ENTRY_COUNT; i++)
		obs_data_set_bool(enabled, entries[i].id, pr_encoder_enabled(&entries[i]));
	obs_data_set_obj(data, "enabled", enabled);
	obs_data_release(enabled);

	if (!obs_data_save_json_safe(data, path, "tmp", "bak"))
		obs_log(LOG_WARNING, "could not save settings to %s", path);
	obs_data_release(data);
	bfree(path);
}
