/* SPDX-License-Identifier: Apache-2.0 */
#include "state/watch_state.h"
#include <string.h>

void watch_state_init(struct watch_state *state)
{
	*state = (struct watch_state) {
		.active_screen = WATCH_SCREEN_HOME,
		.connection = WATCH_CONNECTION_OFFLINE,
		.battery_percent = 72,
		.steps = 7421,
		.temperature_c = 28,
		.time_text = "21:45",
		.day_text = "THU",
		.date_text = "SEP 24",
	};
	watch_state_reset_host_owned(state);
	state->steps = 7421;
	state->temperature_c = 28;
	strcpy(state->weather_condition, "Clear");
}

void watch_state_reset_host_owned(struct watch_state *state)
{
	state->steps = 0;
	state->temperature_c = 0;
	strcpy(state->weather_condition, "Unavailable");
	strcpy(state->music_title, "Nothing playing");
	state->music_playing = false;
	memset(&state->notifications, 0, sizeof(state->notifications));
	state->host_data_stale = false;
	state->host_data_updated_at_ms = 0U;
}

bool watch_state_notification_next(struct watch_state *state)
{
	if (state->notifications.active_index + 1U >= state->notifications.stored_count) {
		return false;
	}
	state->notifications.active_index++;
	return true;
}

bool watch_state_notification_previous(struct watch_state *state)
{
	if (state->notifications.active_index == 0U) return false;
	state->notifications.active_index--;
	return true;
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
