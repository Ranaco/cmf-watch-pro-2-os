/* SPDX-License-Identifier: Apache-2.0 */
#include <assert.h>
#include <stdio.h>
#include "navigation/navigation.h"
#include "state/watch_state.h"
#include "apps/app_registry.h"

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
