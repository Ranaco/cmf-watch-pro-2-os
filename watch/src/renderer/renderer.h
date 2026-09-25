#ifndef CMF_RENDERER_H
#define CMF_RENDERER_H
#include <stdbool.h>
#include <stdint.h>
#include "state/watch_state.h"

enum renderer_action {
	RENDERER_ACTION_NEXT,
	RENDERER_ACTION_PREVIOUS,
	RENDERER_ACTION_NEXT_ITEM,
	RENDERER_ACTION_PREVIOUS_ITEM,
	RENDERER_ACTION_ACTIVATE,
};
typedef void (*renderer_action_handler_t)(enum renderer_action action);
void renderer_init(renderer_action_handler_t action_handler);
void renderer_render(const struct watch_state *state, bool backwards);
void renderer_refresh(const struct watch_state *state);
uint32_t renderer_frame_count(void);
#endif
