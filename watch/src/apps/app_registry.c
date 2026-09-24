/* SPDX-License-Identifier: Apache-2.0 */
#include "apps/app_registry.h"

static const struct watch_app_descriptor apps[] = {
	{ "home", "Home", WATCH_SCREEN_HOME, "Your day at a glance",
	  "Clock and cached cards remain local.", true },
	{ "notifications", "Notifications", WATCH_SCREEN_NOTIFICATIONS,
	  "You're all caught up", "Cached notifications remain available.", true },
	{ "music", "Music", WATCH_SCREEN_MUSIC, "Nothing playing",
	  "Cached controls remain available offline.", true },
	{ "assistant", "Assistant", WATCH_SCREEN_ASSISTANT, "Assistant ready",
	  "Connect the host for AI requests.", true },
	{ "settings", "Settings", WATCH_SCREEN_SETTINGS, "Local settings",
	  "Display and device controls stay local.", true },
};

size_t app_registry_count(void)
{
	return sizeof(apps) / sizeof(apps[0]);
}

const struct watch_app_descriptor *app_registry_at(size_t index)
{
	return index < app_registry_count() ? &apps[index] : NULL;
}

const struct watch_app_descriptor *app_registry_find(enum watch_screen screen)
{
	for (size_t index = 0; index < app_registry_count(); ++index) {
		if (apps[index].screen == screen) {
			return &apps[index];
		}
	}
	return NULL;
}
