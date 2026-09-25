#ifndef CMF_WATCH_INPUT_H
#define CMF_WATCH_INPUT_H
enum watch_input_event {
	WATCH_INPUT_NONE,
	WATCH_INPUT_HARDWARE_BUTTON,
	WATCH_INPUT_ACTIVATE,
};
int watch_input_init(void);
enum watch_input_event watch_input_poll(void);
#endif
