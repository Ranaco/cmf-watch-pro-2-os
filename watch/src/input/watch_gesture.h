#ifndef CMF_WATCH_GESTURE_H
#define CMF_WATCH_GESTURE_H

#include <stdint.h>

enum watch_swipe {
	WATCH_SWIPE_NONE,
	WATCH_SWIPE_NEXT,
	WATCH_SWIPE_PREVIOUS,
};

enum watch_swipe watch_swipe_classify(int32_t delta_x, int32_t delta_y);

#endif
