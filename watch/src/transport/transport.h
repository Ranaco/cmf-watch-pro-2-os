#ifndef CMF_TRANSPORT_H
#define CMF_TRANSPORT_H
#include <stdbool.h>
#include <stdint.h>
#include "protocol/watch_protocol.h"

enum transport_event {
	TRANSPORT_EVENT_CONNECTED,
	TRANSPORT_EVENT_DISCONNECTED,
};

typedef int (*transport_message_handler_t)(const struct watch_protocol_message *message,
					    void *context);
typedef void (*transport_event_handler_t)(enum transport_event event, void *context);

void transport_init(transport_message_handler_t message_handler,
		    transport_event_handler_t event_handler, void *context);
void transport_poll(void);
bool transport_connected(void);
int transport_request_sync(uint64_t last_revision);
int transport_send_music_action(bool playing, uint32_t *message_id);
#endif
