/* SPDX-License-Identifier: Apache-2.0 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "navigation/navigation.h"
#include "state/watch_state.h"
#include "apps/app_registry.h"
#include "input/watch_gesture.h"
#include "cache/watch_cache.h"
#include "actions/watch_actions.h"

int main(void)
{
	struct watch_navigation navigation;
	struct watch_state state;

	watch_state_init(&state);
	navigation_init(&navigation);
	assert(navigation_current(&navigation) == WATCH_SCREEN_HOME);
	assert(state.connection == WATCH_CONNECTION_OFFLINE);
	assert(navigation_push(&navigation, WATCH_SCREEN_NOTIFICATIONS));
	assert(navigation_push(&navigation, WATCH_SCREEN_MUSIC));
	assert(navigation_current(&navigation) == WATCH_SCREEN_MUSIC);
	assert(navigation_pop(&navigation));
	assert(navigation_current(&navigation) == WATCH_SCREEN_NOTIFICATIONS);
	assert(navigation_pop(&navigation));
	assert(navigation_current(&navigation) == WATCH_SCREEN_HOME);
	assert(!navigation_pop(&navigation));
	assert(navigation_next(&navigation));
	assert(navigation_current(&navigation) == WATCH_SCREEN_NOTIFICATIONS);
	assert(navigation_next(&navigation));
	assert(navigation_current(&navigation) == WATCH_SCREEN_MUSIC);
	assert(navigation_previous(&navigation));
	assert(navigation_current(&navigation) == WATCH_SCREEN_NOTIFICATIONS);
	assert(navigation_previous(&navigation));
	assert(navigation_current(&navigation) == WATCH_SCREEN_HOME);
	assert(navigation_previous(&navigation));
	assert(navigation_current(&navigation) == WATCH_SCREEN_SETTINGS);
	assert(navigation_next(&navigation));
	assert(navigation_current(&navigation) == WATCH_SCREEN_HOME);
	assert(watch_swipe_classify(-80, 4) == WATCH_SWIPE_NEXT);
	assert(watch_swipe_classify(80, -4) == WATCH_SWIPE_PREVIOUS);
	assert(watch_swipe_classify(-49, 0) == WATCH_SWIPE_NONE);
	assert(watch_swipe_classify(-80, 41) == WATCH_SWIPE_NONE);
	assert(watch_swipe_classify(4, 90) == WATCH_SWIPE_PREVIOUS_ITEM);
	assert(watch_swipe_classify(-4, -90) == WATCH_SWIPE_NEXT_ITEM);

	struct watch_cache_record cache;
	watch_cache_init(&cache);
	assert(watch_cache_valid(&cache));
	strcpy(state.weather_condition, "Rain");
	state.temperature_c = 19;
	state.steps = 12345;
	state.notifications.total_count = 2;
	state.notifications.stored_count = 2;
	strcpy(state.notifications.entries[0].title, "Cached title");
	strcpy(state.notifications.entries[0].body, "Cached body");
	strcpy(state.notifications.entries[1].title, "Second title");
	strcpy(state.notifications.entries[1].body, "Second body");
	assert(watch_state_notification_next(&state));
	assert(state.notifications.active_index == 1U);
	assert(!watch_state_notification_next(&state));
	assert(watch_state_notification_previous(&state));
	strcpy(state.music_title, "Cached track");
	state.music_playing = true;
	watch_cache_capture_sync(&cache, &state, 42U, 1234000U);
	watch_state_set_screen(&state, WATCH_SCREEN_MUSIC);
	watch_cache_capture_application(&cache, &state, 1235000U);
	assert(watch_cache_valid(&cache));
	watch_state_init(&state);
	assert(watch_cache_restore(&cache, &state));
	assert(state.temperature_c == 19);
	assert(state.steps == 12345);
	assert(strcmp(state.weather_condition, "Rain") == 0);
	assert(state.notifications.total_count == 2);
	assert(state.notifications.stored_count == 2);
	assert(strcmp(state.notifications.entries[1].title, "Second title") == 0);
	assert(strcmp(state.music_title, "Cached track") == 0);
	assert(state.music_playing);
	assert(state.active_screen == WATCH_SCREEN_MUSIC);
	assert(state.host_data_stale);
	watch_cache_mark_stale(&cache, 1236000U);
	assert(cache.weather_meta.stale);
	assert(cache.activity_meta.stale);
	assert(cache.notifications_meta.stale);
	assert(cache.music_meta.stale);
	assert(watch_cache_valid(&cache));
	cache.temperature_c++;
	assert(!watch_cache_valid(&cache));

	watch_cache_init(&cache);
	watch_state_init(&state);
	watch_state_set_screen(&state, WATCH_SCREEN_SETTINGS);
	watch_cache_capture_application(&cache, &state, 2000000U);
	watch_state_init(&state);
	assert(watch_cache_restore(&cache, &state));
	assert(state.active_screen == WATCH_SCREEN_SETTINGS);
	assert(!state.host_data_stale);

	struct watch_actions actions;
	watch_actions_init(&actions);
	state.music_playing = false;
	assert(watch_actions_begin_music(&actions, &state, 100U));
	assert(state.music_playing && state.music_action_pending);
	watch_actions_bind_message(&actions, 77U);
	const char ok_payload[] = "{\"status\":\"ok\",\"code\":\"none\"}";
	struct watch_protocol_message result = {
		.type = WATCH_MESSAGE_COMMAND_RESULT,
		.reply_to = 77U,
		.has_reply_to = true,
		.payload = ok_payload,
		.payload_len = sizeof(ok_payload) - 1U,
	};
	assert(watch_actions_handle_result(&actions, &state, &result));
	assert(state.music_playing && !state.music_action_pending);
	assert(watch_actions_begin_music(&actions, &state, 200U));
	watch_actions_bind_message(&actions, 78U);
	const char error_payload[] = "{\"status\":\"rejected\",\"code\":\"busy\"}";
	result.reply_to = 78U;
	result.payload = error_payload;
	result.payload_len = sizeof(error_payload) - 1U;
	assert(watch_actions_handle_result(&actions, &state, &result));
	assert(state.music_playing && !state.music_action_pending);
	assert(watch_actions_begin_music(&actions, &state, 1000U));
	assert(watch_actions_poll_timeout(&actions, &state,
					 1000U + WATCH_ACTION_TIMEOUT_MS));
	assert(state.music_playing && !state.music_action_pending);

	watch_state_set_screen(&state, WATCH_SCREEN_SETTINGS);
	assert(state.active_screen == WATCH_SCREEN_SETTINGS);
	assert(watch_screen_name(state.active_screen) != NULL);
	assert(app_registry_count() == 5);
	for (size_t index = 0; index < app_registry_count(); ++index) {
		const struct watch_app_descriptor *app = app_registry_at(index);
		assert(app != NULL);
		assert(app->version >= 1U);
		assert(app->entry != NULL && app->entry[0] != '\0');
		assert(app->offline_available);
		assert(app_registry_find(app->screen) == app);
	}
	puts("Core navigation and state tests passed.");
	return 0;
}
