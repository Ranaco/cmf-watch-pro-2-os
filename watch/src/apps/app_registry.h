#ifndef CMF_APP_REGISTRY_H
#define CMF_APP_REGISTRY_H
#include <stdbool.h>
#include <stddef.h>
#include "state/watch_state.h"

struct watch_app_descriptor {
	const char *id;
	const char *name;
	enum watch_screen screen;
	const char *headline;
	const char *offline_message;
	bool offline_available;
};

size_t app_registry_count(void);
const struct watch_app_descriptor *app_registry_at(size_t index);
const struct watch_app_descriptor *app_registry_find(enum watch_screen screen);
#endif
