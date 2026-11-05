#pragma once

#include <obs-module.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One entry per encoder this plugin registers. */
struct pr_encoder_entry {
	const char *id;       /* encoder id as registered with OBS */
	const char *desc_key; /* locale key for the one-line description in the settings window */
	bool experimental;    /* experimental encoders are off by default */
	struct obs_encoder_info *info;
	/* Optional start-up check that the hardware can run this encoder; NULL = always available. On failure it
	 * writes the reason (for the log) into the buffer. */
	bool (*probe)(char *reason, size_t reason_len);
	/* Locale key for the sentence shown when the probe failed; NULL = "Settings.Unavailable". */
	const char *unavailable_key;
};

const struct pr_encoder_entry *pr_encoder_entries(size_t *count);

/* Reads the saved choices (settings.json in the plugin's config folder) and runs the hardware checks. */
void pr_settings_load(void);

/* False when the hardware check failed: the encoder is hidden and cannot be switched on. */
bool pr_encoder_available(const struct pr_encoder_entry *entry);

/* The saved choice (what the user ticked), regardless of availability. */
bool pr_encoder_enabled(const struct pr_encoder_entry *entry);
void pr_encoder_set_enabled(const struct pr_encoder_entry *entry, bool enabled);
void pr_settings_save(void);

/* Tools menu entry with the settings window (settings-dialog.cpp). */
void pr_ui_init(void);

#ifdef __cplusplus
}
#endif
