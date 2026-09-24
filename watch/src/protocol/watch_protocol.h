#ifndef CMF_WATCH_PROTOCOL_H
#define CMF_WATCH_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WATCH_PROTOCOL_VERSION 1U
#define WATCH_PROTOCOL_MAX_FRAME_SIZE 16384U

enum watch_message_type {
	WATCH_MESSAGE_STATE_SET,
	WATCH_MESSAGE_STATE_PATCH,
	WATCH_MESSAGE_LIST_INSERT,
	WATCH_MESSAGE_LIST_REMOVE,
	WATCH_MESSAGE_LIST_UPDATE,
	WATCH_MESSAGE_REFRESH,
	WATCH_MESSAGE_APP_OPEN,
	WATCH_MESSAGE_SHOW_TOAST,
	WATCH_MESSAGE_SHOW_DIALOG,
	WATCH_MESSAGE_CACHE_INVALIDATE,
	WATCH_MESSAGE_APP_OPENED,
	WATCH_MESSAGE_ACTION,
	WATCH_MESSAGE_BUTTON,
	WATCH_MESSAGE_GESTURE,
	WATCH_MESSAGE_INPUT,
	WATCH_MESSAGE_REFRESH_REQUEST,
	WATCH_MESSAGE_HELLO,
	WATCH_MESSAGE_CAPABILITIES,
	WATCH_MESSAGE_PING,
	WATCH_MESSAGE_PONG,
	WATCH_MESSAGE_SYNC_REQUEST,
	WATCH_MESSAGE_SYNC_RESPONSE,
	WATCH_MESSAGE_COMMAND_RESULT,
};

enum watch_protocol_result {
	WATCH_PROTOCOL_OK = 0,
	WATCH_PROTOCOL_INVALID_JSON = -1,
	WATCH_PROTOCOL_INVALID_UTF8 = -2,
	WATCH_PROTOCOL_FRAME_TOO_LARGE = -3,
	WATCH_PROTOCOL_UNSUPPORTED_VERSION = -4,
	WATCH_PROTOCOL_UNKNOWN_TYPE = -5,
	WATCH_PROTOCOL_MISSING_FIELD = -6,
};

struct watch_protocol_message {
	enum watch_message_type type;
	uint32_t id;
	uint32_t reply_to;
	bool has_reply_to;
	const char *payload;
	size_t payload_len;
};

typedef int (*watch_protocol_message_handler)(const struct watch_protocol_message *message,
					       void *context);

struct watch_frame_decoder {
	char bytes[WATCH_PROTOCOL_MAX_FRAME_SIZE + 1U];
	size_t length;
};

int watch_protocol_decode(char *frame, size_t length, struct watch_protocol_message *message);
int watch_frame_decoder_push(struct watch_frame_decoder *decoder, const uint8_t *bytes,
			     size_t length, watch_protocol_message_handler handler, void *context);
const char *watch_message_type_name(enum watch_message_type type);

#endif
