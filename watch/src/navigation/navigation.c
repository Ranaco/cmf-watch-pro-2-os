/* SPDX-License-Identifier: Apache-2.0 */
#include "navigation/navigation.h"

void navigation_init(struct watch_navigation *navigation)
{
	navigation->stack[0] = WATCH_SCREEN_HOME;
	navigation->depth = 1;
}

enum watch_screen navigation_current(const struct watch_navigation *navigation)
{
	return navigation->stack[navigation->depth - 1];
}

bool navigation_push(struct watch_navigation *navigation, enum watch_screen screen)
{
	if (navigation->depth >= WATCH_NAVIGATION_DEPTH) {
		return false;
	}
	navigation->stack[navigation->depth++] = screen;
	return true;
}

bool navigation_pop(struct watch_navigation *navigation)
{
	if (navigation->depth <= 1) {
		return false;
	}
	navigation->depth--;
	return true;
}

void navigation_replace(struct watch_navigation *navigation, enum watch_screen screen)
{
	navigation->stack[navigation->depth - 1] = screen;
}

bool navigation_next(struct watch_navigation *navigation)
{
	enum watch_screen current = navigation_current(navigation);
	navigation->depth = 1;
	navigation->stack[0] = (enum watch_screen)((current + 1) % WATCH_SCREEN_COUNT);
	return true;
}

bool navigation_previous(struct watch_navigation *navigation)
{
	enum watch_screen current = navigation_current(navigation);
	navigation->depth = 1;
	navigation->stack[0] = current == WATCH_SCREEN_HOME
		? WATCH_SCREEN_SETTINGS : (enum watch_screen)(current - 1);
	return true;
}
