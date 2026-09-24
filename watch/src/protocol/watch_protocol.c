/* SPDX-License-Identifier: Apache-2.0 */
#include "protocol/watch_protocol.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/data/json.h>

struct message_type_entry {
	const char *name;
	enum watch_message_type type;
	const char *required[5];
};

static const struct message_type_entry message_types[] = {
	{"state_set", WATCH_MESSAGE_STATE_SET, {"path", "value", "revision"}},
	{"state_patch", WATCH_MESSAGE_STATE_PATCH, {"path", "value", "revision"}},
	{"list_insert", WATCH_MESSAGE_LIST_INSERT, {"path", "index", "value", "revision"}},
	{"list_remove", WATCH_MESSAGE_LIST_REMOVE, {"path", "index", "revision"}},
	{"list_update", WATCH_MESSAGE_LIST_UPDATE, {"path", "index", "value", "revision"}},
	{"refresh", WATCH_MESSAGE_REFRESH, {"scope"}},
	{"app_open", WATCH_MESSAGE_APP_OPEN, {"app_id"}},
	{"show_toast", WATCH_MESSAGE_SHOW_TOAST, {"message", "duration_ms"}},
	{"show_dialog", WATCH_MESSAGE_SHOW_DIALOG, {"title", "message", "actions"}},
	{"cache_invalidate", WATCH_MESSAGE_CACHE_INVALIDATE, {"path"}},
	{"app_opened", WATCH_MESSAGE_APP_OPENED, {"app_id"}},
	{"action", WATCH_MESSAGE_ACTION, {"app_id", "action_id", "arguments"}},
	{"button", WATCH_MESSAGE_BUTTON, {"button", "gesture"}},
	{"gesture", WATCH_MESSAGE_GESTURE, {"gesture"}},
	{"input", WATCH_MESSAGE_INPUT, {"input_id", "value"}},
	{"refresh_request", WATCH_MESSAGE_REFRESH_REQUEST, {"scope"}},
	{"hello", WATCH_MESSAGE_HELLO,
	 {"device_id", "session_id", "runtime_version", "protocol_versions", "simulator"}},
	{"capabilities", WATCH_MESSAGE_CAPABILITIES, {"features", "apps", "display"}},
	{"ping", WATCH_MESSAGE_PING, {"nonce"}},
	{"pong", WATCH_MESSAGE_PONG, {"nonce"}},
	{"sync_request", WATCH_MESSAGE_SYNC_REQUEST, {"last_revision"}},
	{"sync_response", WATCH_MESSAGE_SYNC_RESPONSE, {"revision", "state"}},
	{"command_result", WATCH_MESSAGE_COMMAND_RESULT, {"status", "code"}},
};

static bool span_equal(const char *start, size_t length, const char *expected)
{
	return strlen(expected) == length && memcmp(start, expected, length) == 0;
}

static bool valid_utf8(const uint8_t *bytes, size_t length)
{
	size_t i = 0;
	while (i < length) {
		uint8_t first = bytes[i++];
		if (first < 0x80U) {
			continue;
		}
		uint32_t codepoint;
		size_t continuation;
		if ((first & 0xe0U) == 0xc0U) {
			codepoint = first & 0x1fU;
			continuation = 1;
		} else if ((first & 0xf0U) == 0xe0U) {
			codepoint = first & 0x0fU;
			continuation = 2;
		} else if ((first & 0xf8U) == 0xf0U) {
			codepoint = first & 0x07U;
			continuation = 3;
		} else {
			return false;
		}
		if (i + continuation > length) {
			return false;
		}
		for (size_t j = 0; j < continuation; ++j) {
			uint8_t next = bytes[i++];
			if ((next & 0xc0U) != 0x80U) {
				return false;
			}
			codepoint = (codepoint << 6) | (next & 0x3fU);
		}
		if ((continuation == 1 && codepoint < 0x80U) ||
		    (continuation == 2 && codepoint < 0x800U) ||
		    (continuation == 3 && codepoint < 0x10000U) || codepoint > 0x10ffffU ||
		    (codepoint >= 0xd800U && codepoint <= 0xdfffU)) {
			return false;
		}
	}
	return true;
}

static int parse_u32(const struct json_token *token, uint32_t *value)
{
	if (token->type != JSON_TOK_NUMBER || token->start == token->end) {
		return -EINVAL;
	}
	uint64_t result = 0;
	for (const char *cursor = token->start; cursor < token->end; ++cursor) {
		if (*cursor < '0' || *cursor > '9') {
			return -EINVAL;
		}
		result = result * 10U + (uint32_t)(*cursor - '0');
		if (result > UINT32_MAX) {
			return -ERANGE;
		}
	}
	*value = (uint32_t)result;
	return 0;
}

static const struct message_type_entry *find_type(const struct json_token *token)
{
	if (token->type != JSON_TOK_STRING) {
		return NULL;
	}
	size_t length = (size_t)(token->end - token->start);
	for (size_t i = 0; i < ARRAY_SIZE(message_types); ++i) {
		if (span_equal(token->start, length, message_types[i].name)) {
			return &message_types[i];
		}
	}
	return NULL;
}

static int validate_payload(char *payload, size_t length,
			    const struct message_type_entry *entry)
{
	struct json_obj object;
	struct json_obj_key_value pair;
	uint32_t found = 0;
	if (json_obj_separate_parse_init(&object, payload, length) < 0) {
		return WATCH_PROTOCOL_INVALID_JSON;
	}
	while (true) {
		int result = json_obj_next_key_value(&object, &pair);
		if (result < 0) {
			return WATCH_PROTOCOL_INVALID_JSON;
		}
		if (pair.key == NULL) {
			break;
		}
		for (size_t i = 0; i < ARRAY_SIZE(entry->required); ++i) {
			if (entry->required[i] != NULL &&
			    span_equal(pair.key, pair.key_len, entry->required[i])) {
				found |= BIT(i);
			}
		}
	}
	for (size_t i = 0; i < ARRAY_SIZE(entry->required); ++i) {
		if (entry->required[i] != NULL && (found & BIT(i)) == 0U) {
			return WATCH_PROTOCOL_MISSING_FIELD;
		}
	}
	return WATCH_PROTOCOL_OK;
}

int watch_protocol_decode(char *frame, size_t length, struct watch_protocol_message *message)
{
	if (length == 0U) {
		return WATCH_PROTOCOL_INVALID_JSON;
	}
	if (length > WATCH_PROTOCOL_MAX_FRAME_SIZE) {
		return WATCH_PROTOCOL_FRAME_TOO_LARGE;
	}
	if (!valid_utf8((const uint8_t *)frame, length)) {
		return WATCH_PROTOCOL_INVALID_UTF8;
	}
	struct json_obj object;
	struct json_obj_key_value pair;
	const struct message_type_entry *entry = NULL;
	uint32_t version = 0;
	uint32_t fields = 0;
	memset(message, 0, sizeof(*message));
	if (json_obj_separate_parse_init(&object, frame, length) < 0) {
		return WATCH_PROTOCOL_INVALID_JSON;
	}
	while (true) {
		int result = json_obj_next_key_value(&object, &pair);
		if (result < 0) {
			return WATCH_PROTOCOL_INVALID_JSON;
		}
		if (pair.key == NULL) {
			break;
		}
		if (span_equal(pair.key, pair.key_len, "version")) {
			if (parse_u32(&pair.value, &version) < 0) return WATCH_PROTOCOL_INVALID_JSON;
			fields |= BIT(0);
		} else if (span_equal(pair.key, pair.key_len, "type")) {
			entry = find_type(&pair.value);
			fields |= BIT(1);
		} else if (span_equal(pair.key, pair.key_len, "id")) {
			if (parse_u32(&pair.value, &message->id) < 0) return WATCH_PROTOCOL_INVALID_JSON;
			fields |= BIT(2);
		} else if (span_equal(pair.key, pair.key_len, "reply_to")) {
			if (parse_u32(&pair.value, &message->reply_to) < 0) return WATCH_PROTOCOL_INVALID_JSON;
			message->has_reply_to = true;
		} else if (span_equal(pair.key, pair.key_len, "payload")) {
			if (pair.value.type != JSON_TOK_OBJECT_START) return WATCH_PROTOCOL_INVALID_JSON;
			message->payload = pair.value.start;
			message->payload_len = (size_t)(pair.value.end - pair.value.start);
			fields |= BIT(3);
		}
	}
	if (fields != (BIT(0) | BIT(1) | BIT(2) | BIT(3))) {
		return WATCH_PROTOCOL_MISSING_FIELD;
	}
	if (version != WATCH_PROTOCOL_VERSION) return WATCH_PROTOCOL_UNSUPPORTED_VERSION;
	if (entry == NULL) return WATCH_PROTOCOL_UNKNOWN_TYPE;
	message->type = entry->type;
	if (message->type == WATCH_MESSAGE_COMMAND_RESULT && !message->has_reply_to) {
		return WATCH_PROTOCOL_MISSING_FIELD;
	}
	return validate_payload((char *)message->payload, message->payload_len, entry);
}

int watch_frame_decoder_push(struct watch_frame_decoder *decoder, const uint8_t *bytes,
			     size_t length, watch_protocol_message_handler handler, void *context)
{
	for (size_t i = 0; i < length; ++i) {
		if (bytes[i] == '\n') {
			size_t frame_length = decoder->length;
			if (frame_length > 0U && decoder->bytes[frame_length - 1U] == '\r') frame_length--;
			if (frame_length > 0U) {
				struct watch_protocol_message message;
				int result = watch_protocol_decode(decoder->bytes, frame_length, &message);
				if (result < 0 || (result = handler(&message, context)) < 0) {
					decoder->length = 0;
					return result;
				}
			}
			decoder->length = 0;
			continue;
		}
		if (decoder->length >= WATCH_PROTOCOL_MAX_FRAME_SIZE) {
			decoder->length = 0;
			return WATCH_PROTOCOL_FRAME_TOO_LARGE;
		}
		decoder->bytes[decoder->length++] = (char)bytes[i];
	}
	return WATCH_PROTOCOL_OK;
}

const char *watch_message_type_name(enum watch_message_type type)
{
	for (size_t i = 0; i < ARRAY_SIZE(message_types); ++i) {
		if (message_types[i].type == type) return message_types[i].name;
	}
	return "unknown";
}
