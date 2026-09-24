#ifndef CMF_RENDERER_H
#define CMF_RENDERER_H
#include <stdbool.h>
#include "state/watch_state.h"

enum renderer_action {
	RENDERER_ACTION_OPEN_NOTIFICATIONS,
	RENDERER_ACTION_OPEN_MUSIC,
	RENDERER_ACTION_OPEN_ASSISTANT,
	RENDERER_ACTION_OPEN_SETTINGS,
	RENDERER_ACTION_BACK,
};
typedef void (*renderer_action_handler_t)(enum renderer_action action);
void renderer_init(renderer_action_handler_t action_handler);
void renderer_render(const struct watch_state *state, bool backwards);
#endif
