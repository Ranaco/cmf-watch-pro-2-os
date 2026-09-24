/* SPDX-License-Identifier: Apache-2.0 */
#include "state/watch_state.h"

void watch_state_init(struct watch_state *state)
{
	*state = (struct watch_state) {
		.active_screen = WATCH_SCREEN_HOME,
		.connection = WATCH_CONNECTION_OFFLINE,
		.battery_percent = 72,
		.steps = 7421,
		.temperature_c = 28,
		.time_text = "21:45",
		.day_text = "THURSDAY",
		.date_text = "September 24",
		.music_title = "Nothing playing",
		.music_playing = false,
	};
}

void watch_state_set_screen(struct watch_state *state, enum watch_screen screen)
{
	state->active_screen = screen;
}

const char *watch_screen_name(enum watch_screen screen)
{
	switch (screen) {
	case WATCH_SCREEN_HOME: return "home";
	case WATCH_SCREEN_NOTIFICATIONS: return "notifications";
	case WATCH_SCREEN_MUSIC: return "music";
	case WATCH_SCREEN_ASSISTANT: return "assistant";
	case WATCH_SCREEN_SETTINGS: return "settings";
	default: return "unknown";
	}
}
