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
};

enum watch_connection_state {
	WATCH_CONNECTION_OFFLINE,
	WATCH_CONNECTION_CONNECTING,
	WATCH_CONNECTION_ONLINE,
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
	const char *music_title;
	bool music_playing;
};

void watch_state_init(struct watch_state *state);
void watch_state_set_screen(struct watch_state *state, enum watch_screen screen);
const char *watch_screen_name(enum watch_screen screen);
#endif
