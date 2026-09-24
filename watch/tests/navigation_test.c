/* SPDX-License-Identifier: Apache-2.0 */
#include <assert.h>
#include <stdio.h>
#include "navigation/navigation.h"
#include "state/watch_state.h"
#include "apps/app_registry.h"
#include "input/watch_gesture.h"

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
	assert(watch_swipe_classify(4, 90) == WATCH_SWIPE_NONE);

	watch_state_set_screen(&state, WATCH_SCREEN_SETTINGS);
	assert(state.active_screen == WATCH_SCREEN_SETTINGS);
	assert(watch_screen_name(state.active_screen) != NULL);
	assert(app_registry_count() == 5);
	for (size_t index = 0; index < app_registry_count(); ++index) {
		const struct watch_app_descriptor *app = app_registry_at(index);
		assert(app != NULL);
		assert(app->offline_available);
		assert(app_registry_find(app->screen) == app);
	}
	puts("Core navigation and state tests passed.");
	return 0;
}
