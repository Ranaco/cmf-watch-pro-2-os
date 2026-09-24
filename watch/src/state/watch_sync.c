/* SPDX-License-Identifier: Apache-2.0 */
#include "state/watch_sync.h"

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <zephyr/data/json.h>
#include <zephyr/sys/util.h>

static bool span_equal(const char *start, size_t length, const char *expected)
{
	return strlen(expected) == length && memcmp(start, expected, length) == 0;
}

static int parse_u64(const struct json_token *token, uint64_t *value)
{
	if (token->type != JSON_TOK_NUMBER || token->start == token->end) return -EINVAL;
	uint64_t result = 0;
	for (const char *cursor = token->start; cursor < token->end; ++cursor) {
		if (*cursor < '0' || *cursor > '9') return -EINVAL;
		uint8_t digit = (uint8_t)(*cursor - '0');
		if (result > (UINT64_MAX - digit) / 10U) return -ERANGE;
		result = result * 10U + digit;
	}
	*value = result;
	return 0;
}

static int parse_i64(const struct json_token *token, int64_t *value)
{
	if (token->type != JSON_TOK_NUMBER || token->start == token->end) return -EINVAL;
	const char *cursor = token->start;
	bool negative = *cursor == '-';
	if (negative && ++cursor == token->end) return -EINVAL;
	uint64_t magnitude = 0;
	for (; cursor < token->end; ++cursor) {
		if (*cursor < '0' || *cursor > '9') return -EINVAL;
		uint8_t digit = (uint8_t)(*cursor - '0');
		if (magnitude > (uint64_t)(INT64_MAX - digit) / 10U) return -ERANGE;
		magnitude = magnitude * 10U + digit;
	}
	*value = negative ? -(int64_t)magnitude : (int64_t)magnitude;
	return 0;
}

static int copy_string(const struct json_token *token, char *destination, size_t capacity)
{
	if (token->type != JSON_TOK_STRING || capacity == 0U) return -EINVAL;
	size_t length = (size_t)(token->end - token->start);
	if (length >= capacity || memchr(token->start, '\\', length) != NULL) return -ENOSPC;
	memcpy(destination, token->start, length);
	destination[length] = '\0';
	return 0;
}

static int find_member(char *object_text, size_t length, const char *name,
			struct json_token *value)
{
	struct json_obj object;
	struct json_obj_key_value pair;
	if (json_obj_separate_parse_init(&object, object_text, length) < 0) return -EINVAL;
	while (true) {
		if (json_obj_next_key_value(&object, &pair) < 0) return -EINVAL;
		if (pair.key == NULL) return -ENOENT;
		if (span_equal(pair.key, pair.key_len, name)) {
			*value = pair.value;
			return 0;
		}
	}
}

static int parse_notifications(struct json_token *token, uint16_t *count,
			       char *title, size_t title_capacity,
			       char *body, size_t body_capacity)
{
	if (token->type != JSON_TOK_ARRAY_START) return -EINVAL;
	struct notification_json {
		char *id;
		char *title;
		char *body;
	};
	static const struct json_obj_descr fields[] = {
		JSON_OBJ_DESCR_PRIM(struct notification_json, id, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM(struct notification_json, title, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM(struct notification_json, body, JSON_TOK_STRING),
	};
	struct json_obj array;
	if (json_arr_separate_object_parse_init(&array, token->start,
						(size_t)(token->end - token->start)) < 0) return -EINVAL;
	*count = 0;
	title[0] = '\0';
	body[0] = '\0';
	while (true) {
		struct notification_json item = {0};
		int result = json_arr_separate_parse_object(&array, fields, ARRAY_SIZE(fields), &item);
		if (result < 0) return -EINVAL;
		if (result == 0) return 0;
		if (*count == 0U) {
			if (item.title != NULL) {
				strncpy(title, item.title, title_capacity - 1U);
				title[title_capacity - 1U] = '\0';
			}
			if (item.body != NULL) {
				strncpy(body, item.body, body_capacity - 1U);
				body[body_capacity - 1U] = '\0';
			}
		}
		if (*count < UINT16_MAX) (*count)++;
	}
}

static int parse_weather(struct json_token *token, struct watch_state *candidate)
{
	if (token->type != JSON_TOK_OBJECT_START) return -EINVAL;
	struct json_token value;
	if (find_member(token->start, (size_t)(token->end - token->start),
			"temperature", &value) == 0) {
		int64_t temperature;
		if (parse_i64(&value, &temperature) < 0 || temperature < INT16_MIN ||
		    temperature > INT16_MAX) return -EINVAL;
		candidate->temperature_c = (int16_t)temperature;
	}
	if (find_member(token->start, (size_t)(token->end - token->start),
			"condition", &value) == 0 &&
	    copy_string(&value, candidate->weather_condition,
			sizeof(candidate->weather_condition)) < 0) return -EINVAL;
	return 0;
}

static int parse_music(struct json_token *token, struct watch_state *candidate)
{
	if (token->type != JSON_TOK_OBJECT_START) return -EINVAL;
	struct json_token value;
	if (find_member(token->start, (size_t)(token->end - token->start), "title", &value) == 0 &&
	    copy_string(&value, candidate->music_title, sizeof(candidate->music_title)) < 0) return -EINVAL;
	if (find_member(token->start, (size_t)(token->end - token->start), "playing", &value) == 0) {
		if (value.type == JSON_TOK_TRUE) candidate->music_playing = true;
		else if (value.type == JSON_TOK_FALSE) candidate->music_playing = false;
		else return -EINVAL;
	}
	return 0;
}

static int parse_snapshot(const struct watch_protocol_message *message,
			  const struct watch_state *state, struct watch_state *candidate,
			  uint64_t *revision)
{
	struct json_token revision_token;
	struct json_token state_token;
	if (find_member((char *)message->payload, message->payload_len, "revision", &revision_token) < 0 ||
	    parse_u64(&revision_token, revision) < 0 ||
	    find_member((char *)message->payload, message->payload_len, "state", &state_token) < 0 ||
	    state_token.type != JSON_TOK_OBJECT_START) return -EINVAL;

	*candidate = *state;
	watch_state_reset_host_owned(candidate);
	struct json_obj object;
	struct json_obj_key_value pair;
	if (json_obj_separate_parse_init(&object, state_token.start,
					(size_t)(state_token.end - state_token.start)) < 0) return -EINVAL;
	while (true) {
		if (json_obj_next_key_value(&object, &pair) < 0) return -EINVAL;
		if (pair.key == NULL) return 0;
		if (span_equal(pair.key, pair.key_len, "weather")) {
			if (parse_weather(&pair.value, candidate) < 0) return -EINVAL;
		} else if (span_equal(pair.key, pair.key_len, "music")) {
			if (parse_music(&pair.value, candidate) < 0) return -EINVAL;
		} else if (span_equal(pair.key, pair.key_len, "steps")) {
			uint64_t steps;
			if (parse_u64(&pair.value, &steps) < 0 || steps > UINT32_MAX) return -EINVAL;
			candidate->steps = (uint32_t)steps;
		} else if (span_equal(pair.key, pair.key_len, "notifications")) {
			if (parse_notifications(&pair.value, &candidate->notification_count,
				candidate->notification_title, sizeof(candidate->notification_title),
				candidate->notification_body, sizeof(candidate->notification_body)) < 0) return -EINVAL;
		}
	}
}

static int parse_mutation(const struct watch_protocol_message *message,
			  struct watch_sync_mutation *mutation)
{
	struct json_token path_token;
	struct json_token value_token;
	struct json_token revision_token;
	char path[40];
	if (find_member((char *)message->payload, message->payload_len, "path", &path_token) < 0 ||
	    find_member((char *)message->payload, message->payload_len, "value", &value_token) < 0 ||
	    find_member((char *)message->payload, message->payload_len, "revision", &revision_token) < 0 ||
	    copy_string(&path_token, path, sizeof(path)) < 0 ||
	    parse_u64(&revision_token, &mutation->revision) < 0) return -EINVAL;

	if (strcmp(path, "weather.temperature") == 0) {
		int64_t temperature;
		if (parse_i64(&value_token, &temperature) < 0 || temperature < INT16_MIN ||
		    temperature > INT16_MAX) return -EINVAL;
		mutation->path = WATCH_SYNC_PATH_TEMPERATURE;
		mutation->value.temperature = (int16_t)temperature;
	} else if (strcmp(path, "weather.condition") == 0) {
		mutation->path = WATCH_SYNC_PATH_WEATHER_CONDITION;
		if (copy_string(&value_token, mutation->value.text, sizeof(mutation->value.text)) < 0) return -EINVAL;
	} else if (strcmp(path, "music.title") == 0) {
		mutation->path = WATCH_SYNC_PATH_MUSIC_TITLE;
		if (copy_string(&value_token, mutation->value.text, sizeof(mutation->value.text)) < 0) return -EINVAL;
	} else if (strcmp(path, "music.playing") == 0) {
		mutation->path = WATCH_SYNC_PATH_MUSIC_PLAYING;
		if (value_token.type == JSON_TOK_TRUE) mutation->value.flag = true;
		else if (value_token.type == JSON_TOK_FALSE) mutation->value.flag = false;
		else return -EINVAL;
	} else if (strcmp(path, "steps") == 0) {
		uint64_t steps;
		if (parse_u64(&value_token, &steps) < 0 || steps > UINT32_MAX) return -EINVAL;
		mutation->path = WATCH_SYNC_PATH_STEPS;
		mutation->value.steps = (uint32_t)steps;
	} else if (strcmp(path, "notifications") == 0 &&
		   message->type == WATCH_MESSAGE_STATE_SET) {
		mutation->path = WATCH_SYNC_PATH_NOTIFICATIONS;
		if (parse_notifications(&value_token, &mutation->value.notifications.count,
			mutation->value.notifications.title, sizeof(mutation->value.notifications.title),
			mutation->value.notifications.body, sizeof(mutation->value.notifications.body)) < 0) return -EINVAL;
	} else {
		return -EPERM;
	}
	mutation->occupied = true;
	return 0;
}

static void apply_mutation(struct watch_state *state, const struct watch_sync_mutation *mutation)
{
	switch (mutation->path) {
	case WATCH_SYNC_PATH_TEMPERATURE:
		state->temperature_c = mutation->value.temperature;
		break;
	case WATCH_SYNC_PATH_WEATHER_CONDITION:
		strncpy(state->weather_condition, mutation->value.text,
			sizeof(state->weather_condition) - 1U);
		state->weather_condition[sizeof(state->weather_condition) - 1U] = '\0';
		break;
	case WATCH_SYNC_PATH_MUSIC_TITLE:
		strncpy(state->music_title, mutation->value.text, sizeof(state->music_title) - 1U);
		state->music_title[sizeof(state->music_title) - 1U] = '\0';
		break;
	case WATCH_SYNC_PATH_MUSIC_PLAYING:
		state->music_playing = mutation->value.flag;
		break;
	case WATCH_SYNC_PATH_STEPS:
		state->steps = mutation->value.steps;
		break;
	case WATCH_SYNC_PATH_NOTIFICATIONS:
		state->notification_count = mutation->value.notifications.count;
		strcpy(state->notification_title, mutation->value.notifications.title);
		strcpy(state->notification_body, mutation->value.notifications.body);
		break;
	}
}

static int buffer_mutation(struct watch_sync *sync, const struct watch_sync_mutation *mutation)
{
	for (size_t i = 0; i < ARRAY_SIZE(sync->buffered); ++i) {
		if (sync->buffered[i].occupied && sync->buffered[i].revision == mutation->revision) return 0;
	}
	for (size_t i = 0; i < ARRAY_SIZE(sync->buffered); ++i) {
		if (!sync->buffered[i].occupied) {
			sync->buffered[i] = *mutation;
			return 0;
		}
	}
	return -ENOSPC;
}

static bool replay_buffer(struct watch_sync *sync, struct watch_state *state)
{
	bool changed = false;
	while (true) {
		struct watch_sync_mutation *next = NULL;
		bool has_gap = false;
		for (size_t i = 0; i < ARRAY_SIZE(sync->buffered); ++i) {
			if (!sync->buffered[i].occupied) continue;
			if (sync->buffered[i].revision <= sync->revision) {
				sync->buffered[i].occupied = false;
			} else if (sync->buffered[i].revision == sync->revision + 1U) {
				next = &sync->buffered[i];
			} else {
				has_gap = true;
			}
		}
		if (next == NULL) {
			sync->syncing = has_gap;
			return changed;
		}
		apply_mutation(state, next);
		sync->revision = next->revision;
		next->occupied = false;
		changed = true;
	}
}

void watch_sync_init(struct watch_sync *sync)
{
	memset(sync, 0, sizeof(*sync));
}

void watch_sync_connected(struct watch_sync *sync)
{
	sync->syncing = true;
}

void watch_sync_disconnected(struct watch_sync *sync)
{
	sync->syncing = false;
	memset(sync->buffered, 0, sizeof(sync->buffered));
}

uint64_t watch_sync_revision(const struct watch_sync *sync)
{
	return sync->revision;
}

bool watch_sync_is_synchronized(const struct watch_sync *sync)
{
	return !sync->syncing;
}

enum watch_sync_result watch_sync_handle(struct watch_sync *sync, struct watch_state *state,
					 const struct watch_protocol_message *message)
{
	if (message->type == WATCH_MESSAGE_SYNC_RESPONSE) {
		struct watch_state candidate;
		uint64_t revision;
		if (parse_snapshot(message, state, &candidate, &revision) < 0) return WATCH_SYNC_ERROR;
		*state = candidate;
		sync->revision = revision;
		(void)replay_buffer(sync, state);
		return sync->syncing ? WATCH_SYNC_NEEDS_SNAPSHOT :
			WATCH_SYNC_CHANGED;
	}
	if (message->type != WATCH_MESSAGE_STATE_SET && message->type != WATCH_MESSAGE_STATE_PATCH) {
		return WATCH_SYNC_UNCHANGED;
	}
	struct watch_sync_mutation mutation = {0};
	if (parse_mutation(message, &mutation) < 0) return WATCH_SYNC_ERROR;
	if (mutation.revision <= sync->revision) return WATCH_SYNC_UNCHANGED;
	if (sync->syncing) {
		return buffer_mutation(sync, &mutation) == 0 ? WATCH_SYNC_BUFFERED : WATCH_SYNC_ERROR;
	}
	if (mutation.revision > sync->revision + 1U) {
		sync->syncing = true;
		return buffer_mutation(sync, &mutation) == 0 ? WATCH_SYNC_NEEDS_SNAPSHOT : WATCH_SYNC_ERROR;
	}
	apply_mutation(state, &mutation);
	sync->revision = mutation.revision;
	return WATCH_SYNC_CHANGED;
}
