#ifndef CMF_NAVIGATION_H
#define CMF_NAVIGATION_H
#include <stdbool.h>
#include <stddef.h>
#include "state/watch_state.h"

#define WATCH_NAVIGATION_DEPTH 8
struct watch_navigation {
	enum watch_screen stack[WATCH_NAVIGATION_DEPTH];
	size_t depth;
};

void navigation_init(struct watch_navigation *navigation);
enum watch_screen navigation_current(const struct watch_navigation *navigation);
bool navigation_push(struct watch_navigation *navigation, enum watch_screen screen);
bool navigation_pop(struct watch_navigation *navigation);
void navigation_replace(struct watch_navigation *navigation, enum watch_screen screen);
bool navigation_next(struct watch_navigation *navigation);
bool navigation_previous(struct watch_navigation *navigation);
#endif
