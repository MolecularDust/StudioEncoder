#include <obs-module.h>
#include <plugin-support.h>

#include "settings.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

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
	pr_settings_load();

	size_t count = 0;
	const struct pr_encoder_entry *entries = pr_encoder_entries(&count);
	for (size_t i = 0; i < count; i++) {
		/* An encoder that is switched off is hidden from OBS's encoder list, but stays registered so that
		 * profiles which already use it keep working. */
		if (pr_encoder_enabled(&entries[i]) && pr_encoder_available(&entries[i]))
			entries[i].info->caps &= ~(uint32_t)OBS_ENCODER_CAP_DEPRECATED;
		else
			entries[i].info->caps |= OBS_ENCODER_CAP_DEPRECATED;
		obs_register_encoder(entries[i].info);
	}
	obs_log(LOG_INFO, "loaded version %s", PLUGIN_VERSION);
	return true;
}

void obs_module_post_load(void)
{
	pr_ui_init();
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "unloaded");
}
