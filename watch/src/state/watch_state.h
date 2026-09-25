#ifndef CMF_WATCH_STATE_H
#define CMF_WATCH_STATE_H
#include <stdbool.h>
#include <stdint.h>

enum watch_screen {
	WATCH_SCREEN_HOME,
	WATCH_SCREEN_NOTIFICATIONS,
	WATCH_SCREEN_MUSIC,
	WATCH_SCREEN_ASSISTANT,
	WATCH_SCREEN_SETTINGS,
	WATCH_SCREEN_COUNT,
};

enum watch_connection_state {
	WATCH_CONNECTION_OFFLINE,
	WATCH_CONNECTION_CONNECTING,
	WATCH_CONNECTION_ONLINE,
};

#define WATCH_NOTIFICATION_CAPACITY 4U

struct watch_notification {
	char title[48];
	char body[96];
};

struct watch_notification_list {
	uint16_t total_count;
	uint8_t stored_count;
	uint8_t active_index;
	struct watch_notification entries[WATCH_NOTIFICATION_CAPACITY];
};

struct watch_state {
	enum watch_screen active_screen;
	enum watch_connection_state connection;
	uint8_t battery_percent;
	uint32_t steps;
	int16_t temperature_c;
	const char *time_text;
	const char *day_text;
	const char *date_text;
	char weather_condition[32];
	char music_title[64];
	bool music_playing;
	struct watch_notification_list notifications;
	bool host_data_stale;
	uint64_t host_data_updated_at_ms;
};

void watch_state_init(struct watch_state *state);
void watch_state_reset_host_owned(struct watch_state *state);
void watch_state_set_screen(struct watch_state *state, enum watch_screen screen);
bool watch_state_notification_next(struct watch_state *state);
bool watch_state_notification_previous(struct watch_state *state);
const char *watch_screen_name(enum watch_screen screen);
#endif
