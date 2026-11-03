#include <obs-module.h>
#include <plugin-support.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

extern struct obs_encoder_info pr_rust_encoder_info;
extern struct obs_encoder_info pr_ffmpeg_encoder_info;
extern struct obs_encoder_info pr_ffmpeg_aw_encoder_info;
extern struct obs_encoder_info pr_nvenc_h264_encoder_info;

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Plugin.Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("Plugin.Name");
}

bool obs_module_load(void)
{
	obs_register_encoder(&pr_rust_encoder_info);
	obs_register_encoder(&pr_ffmpeg_encoder_info);
	obs_register_encoder(&pr_ffmpeg_aw_encoder_info);
	obs_register_encoder(&pr_nvenc_h264_encoder_info);
	obs_log(LOG_INFO, "loaded version %s", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "unloaded");
}
