/* SPDX-License-Identifier: Apache-2.0 */
#include "actions/watch_actions.h"

#include <string.h>

static bool payload_contains_status(const struct watch_protocol_message *message,
				    const char *status)
{
	char needle[32];
	size_t status_length = strlen(status);
	if (status_length + 12U >= sizeof(needle)) return false;
	memcpy(needle, "\"status\":\"", 10U);
	memcpy(needle + 10U, status, status_length);
	needle[10U + status_length] = '\"';
	needle[11U + status_length] = '\0';
	size_t needle_length = 11U + status_length;
	if (needle_length > message->payload_len) return false;
	for (size_t index = 0; index + needle_length <= message->payload_len; ++index) {
		if (memcmp(message->payload + index, needle, needle_length) == 0) return true;
	}
	return false;
}

static void set_feedback(struct watch_state *state, const char *feedback)
{
	strncpy(state->music_action_feedback, feedback,
		sizeof(state->music_action_feedback) - 1U);
	state->music_action_feedback[sizeof(state->music_action_feedback) - 1U] = '\0';
}

void watch_actions_init(struct watch_actions *actions)
{
	memset(actions, 0, sizeof(*actions));
}

bool watch_actions_begin_music(struct watch_actions *actions,
			       struct watch_state *state, uint64_t now_ms)
{
	if (actions->pending) return false;
	actions->pending = true;
	actions->previous_music_playing = state->music_playing;
	actions->message_id = 0U;
	actions->started_at_ms = now_ms;
	state->music_playing = !state->music_playing;
	state->music_action_pending = true;
	set_feedback(state, "SYNCING ACTION");
	return true;
}

void watch_actions_bind_message(struct watch_actions *actions, uint32_t message_id)
{
	actions->message_id = message_id;
}

void watch_actions_fail(struct watch_actions *actions, struct watch_state *state,
			const char *feedback)
{
	if (!actions->pending) return;
	state->music_playing = actions->previous_music_playing;
	state->music_action_pending = false;
	set_feedback(state, feedback);
	actions->pending = false;
}

bool watch_actions_handle_result(struct watch_actions *actions,
				 struct watch_state *state,
				 const struct watch_protocol_message *message)
{
	if (!actions->pending || message->type != WATCH_MESSAGE_COMMAND_RESULT ||
	    !message->has_reply_to || message->reply_to != actions->message_id) return false;
	if (payload_contains_status(message, "ok")) {
		state->music_action_pending = false;
		set_feedback(state, "HOST CONFIRMED");
		actions->pending = false;
	} else {
		watch_actions_fail(actions, state, "ACTION REJECTED");
	}
	return true;
}

bool watch_actions_poll_timeout(struct watch_actions *actions,
				struct watch_state *state, uint64_t now_ms)
{
	if (!actions->pending || now_ms - actions->started_at_ms < WATCH_ACTION_TIMEOUT_MS) {
		return false;
	}
	watch_actions_fail(actions, state, "ACTION TIMEOUT");
	return true;
}
