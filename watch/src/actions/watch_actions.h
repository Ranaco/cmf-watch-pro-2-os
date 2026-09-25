#ifndef CMF_WATCH_ACTIONS_H
#define CMF_WATCH_ACTIONS_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol/watch_protocol.h"
#include "state/watch_state.h"

#define WATCH_ACTION_TIMEOUT_MS 5000U

struct watch_actions {
	bool pending;
	bool previous_music_playing;
	uint32_t message_id;
	uint64_t started_at_ms;
};

void watch_actions_init(struct watch_actions *actions);
bool watch_actions_begin_music(struct watch_actions *actions,
			       struct watch_state *state, uint64_t now_ms);
void watch_actions_bind_message(struct watch_actions *actions, uint32_t message_id);
void watch_actions_fail(struct watch_actions *actions, struct watch_state *state,
			const char *feedback);
bool watch_actions_handle_result(struct watch_actions *actions,
				 struct watch_state *state,
				 const struct watch_protocol_message *message);
bool watch_actions_poll_timeout(struct watch_actions *actions,
				struct watch_state *state, uint64_t now_ms);

#endif
