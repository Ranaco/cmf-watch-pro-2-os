#ifndef CMF_WATCH_CACHE_H
#define CMF_WATCH_CACHE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "state/watch_state.h"

#define WATCH_CACHE_MAGIC 0x434d4643U
#define WATCH_CACHE_SCHEMA_VERSION 1U

struct watch_cache_metadata {
	uint32_t version;
	uint64_t updated_at_ms;
	bool stale;
};

struct watch_cache_record {
	uint32_t magic;
	uint32_t schema_version;
	uint32_t record_size;
	uint32_t checksum;
	struct watch_cache_metadata weather_meta;
	int16_t temperature_c;
	char weather_condition[32];
	struct watch_cache_metadata activity_meta;
	uint32_t steps;
	struct watch_cache_metadata notifications_meta;
	struct watch_notification_list notifications;
	struct watch_cache_metadata music_meta;
	char music_title[64];
	bool music_playing;
	struct watch_cache_metadata application_meta;
	enum watch_screen active_screen;
	struct watch_cache_metadata connection_meta;
	uint64_t synchronized_revision;
	struct watch_cache_metadata assets_meta;
	uint32_t asset_generation;
};

void watch_cache_init(struct watch_cache_record *cache);
bool watch_cache_valid(const struct watch_cache_record *cache);
void watch_cache_seal(struct watch_cache_record *cache);
bool watch_cache_restore(const struct watch_cache_record *cache,
			 struct watch_state *state);
void watch_cache_capture_sync(struct watch_cache_record *cache,
			      const struct watch_state *state,
			      uint64_t revision, uint64_t now_ms);
void watch_cache_capture_application(struct watch_cache_record *cache,
				     const struct watch_state *state,
				     uint64_t now_ms);
void watch_cache_mark_stale(struct watch_cache_record *cache, uint64_t now_ms);

#endif
