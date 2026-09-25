/* SPDX-License-Identifier: Apache-2.0 */
#include "cache/watch_cache.h"

#include <string.h>

static uint32_t checksum(const struct watch_cache_record *cache)
{
	struct watch_cache_record copy = *cache;
	copy.checksum = 0U;
	const uint8_t *bytes = (const uint8_t *)&copy;
	uint32_t hash = 2166136261U;
	for (size_t index = 0; index < sizeof(copy); ++index) {
		hash ^= bytes[index];
		hash *= 16777619U;
	}
	return hash;
}

static void copy_text(char *destination, size_t capacity, const char *source)
{
	if (capacity == 0U) return;
	strncpy(destination, source, capacity - 1U);
	destination[capacity - 1U] = '\0';
}

static void refresh(struct watch_cache_metadata *metadata, uint64_t now_ms)
{
	metadata->version++;
	metadata->updated_at_ms = now_ms;
	metadata->stale = false;
}

void watch_cache_init(struct watch_cache_record *cache)
{
	memset(cache, 0, sizeof(*cache));
	cache->magic = WATCH_CACHE_MAGIC;
	cache->schema_version = WATCH_CACHE_SCHEMA_VERSION;
	cache->record_size = sizeof(*cache);
	cache->assets_meta.version = 1U;
	cache->assets_meta.stale = false;
	cache->asset_generation = 1U;
	watch_cache_seal(cache);
}

bool watch_cache_valid(const struct watch_cache_record *cache)
{
	return cache->magic == WATCH_CACHE_MAGIC &&
		cache->schema_version == WATCH_CACHE_SCHEMA_VERSION &&
		cache->record_size == sizeof(*cache) &&
		cache->active_screen >= WATCH_SCREEN_HOME &&
		cache->active_screen < WATCH_SCREEN_COUNT &&
		cache->notifications.stored_count <= WATCH_NOTIFICATION_CAPACITY &&
		((cache->notifications.stored_count == 0U &&
		  cache->notifications.active_index == 0U) ||
		 cache->notifications.active_index < cache->notifications.stored_count) &&
		cache->checksum == checksum(cache);
}

void watch_cache_seal(struct watch_cache_record *cache)
{
	cache->checksum = checksum(cache);
}

bool watch_cache_restore(const struct watch_cache_record *cache,
			 struct watch_state *state)
{
	if (!watch_cache_valid(cache)) return false;
	bool restored = false;
	bool restored_host_data = false;
	uint64_t updated_at_ms = 0U;
	if (cache->weather_meta.version > 0U) {
		state->temperature_c = cache->temperature_c;
		copy_text(state->weather_condition, sizeof(state->weather_condition),
			  cache->weather_condition);
		updated_at_ms = cache->weather_meta.updated_at_ms;
		restored = restored_host_data = true;
	}
	if (cache->activity_meta.version > 0U) {
		state->steps = cache->steps;
		if (cache->activity_meta.updated_at_ms > updated_at_ms) {
			updated_at_ms = cache->activity_meta.updated_at_ms;
		}
		restored = restored_host_data = true;
	}
	if (cache->notifications_meta.version > 0U) {
		state->notifications = cache->notifications;
		if (cache->notifications_meta.updated_at_ms > updated_at_ms) {
			updated_at_ms = cache->notifications_meta.updated_at_ms;
		}
		restored = restored_host_data = true;
	}
	if (cache->music_meta.version > 0U) {
		copy_text(state->music_title, sizeof(state->music_title), cache->music_title);
		state->music_playing = cache->music_playing;
		if (cache->music_meta.updated_at_ms > updated_at_ms) {
			updated_at_ms = cache->music_meta.updated_at_ms;
		}
		restored = restored_host_data = true;
	}
	if (cache->application_meta.version > 0U) {
		state->active_screen = cache->active_screen;
		restored = true;
	}
	state->connection = WATCH_CONNECTION_OFFLINE;
	state->host_data_stale = restored_host_data;
	state->host_data_updated_at_ms = updated_at_ms;
	return restored;
}

void watch_cache_capture_sync(struct watch_cache_record *cache,
			      const struct watch_state *state,
			      uint64_t revision, uint64_t now_ms)
{
	refresh(&cache->weather_meta, now_ms);
	cache->temperature_c = state->temperature_c;
	copy_text(cache->weather_condition, sizeof(cache->weather_condition),
		  state->weather_condition);
	refresh(&cache->activity_meta, now_ms);
	cache->steps = state->steps;
	refresh(&cache->notifications_meta, now_ms);
	cache->notifications = state->notifications;
	refresh(&cache->music_meta, now_ms);
	copy_text(cache->music_title, sizeof(cache->music_title), state->music_title);
	cache->music_playing = state->music_playing;
	refresh(&cache->connection_meta, now_ms);
	cache->synchronized_revision = revision;
	watch_cache_seal(cache);
}

void watch_cache_capture_application(struct watch_cache_record *cache,
				     const struct watch_state *state,
				     uint64_t now_ms)
{
	refresh(&cache->application_meta, now_ms);
	cache->active_screen = state->active_screen;
	cache->notifications.active_index = state->notifications.active_index;
	watch_cache_seal(cache);
}

void watch_cache_mark_stale(struct watch_cache_record *cache, uint64_t now_ms)
{
	if (cache->weather_meta.version == 0U) return;
	cache->weather_meta.stale = true;
	cache->activity_meta.stale = true;
	cache->notifications_meta.stale = true;
	cache->music_meta.stale = true;
	refresh(&cache->connection_meta, now_ms);
	cache->connection_meta.stale = true;
	watch_cache_seal(cache);
}
