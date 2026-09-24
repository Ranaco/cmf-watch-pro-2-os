#ifndef CMF_WATCH_SYNC_H
#define CMF_WATCH_SYNC_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol/watch_protocol.h"
#include "state/watch_state.h"

#define WATCH_SYNC_BUFFER_CAPACITY 16U

enum watch_sync_result {
	WATCH_SYNC_UNCHANGED,
	WATCH_SYNC_CHANGED,
	WATCH_SYNC_BUFFERED,
	WATCH_SYNC_NEEDS_SNAPSHOT,
	WATCH_SYNC_ERROR,
};

enum watch_sync_path {
	WATCH_SYNC_PATH_TEMPERATURE,
	WATCH_SYNC_PATH_WEATHER_CONDITION,
	WATCH_SYNC_PATH_MUSIC_TITLE,
	WATCH_SYNC_PATH_MUSIC_PLAYING,
	WATCH_SYNC_PATH_STEPS,
	WATCH_SYNC_PATH_NOTIFICATIONS,
};

struct watch_sync_mutation {
	bool occupied;
	uint64_t revision;
	enum watch_sync_path path;
	union {
		int16_t temperature;
		uint32_t steps;
		bool flag;
		char text[96];
		struct {
			uint16_t count;
			char title[48];
			char body[96];
		} notifications;
	} value;
};

struct watch_sync {
	uint64_t revision;
	bool syncing;
	struct watch_sync_mutation buffered[WATCH_SYNC_BUFFER_CAPACITY];
};

void watch_sync_init(struct watch_sync *sync);
void watch_sync_connected(struct watch_sync *sync);
void watch_sync_disconnected(struct watch_sync *sync);
uint64_t watch_sync_revision(const struct watch_sync *sync);
bool watch_sync_is_synchronized(const struct watch_sync *sync);
enum watch_sync_result watch_sync_handle(struct watch_sync *sync, struct watch_state *state,
					 const struct watch_protocol_message *message);

#endif
