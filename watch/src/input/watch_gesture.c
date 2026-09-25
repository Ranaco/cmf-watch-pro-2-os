/* SPDX-License-Identifier: Apache-2.0 */
#include "input/watch_gesture.h"

#define WATCH_SWIPE_MIN_DISTANCE 50

static int32_t magnitude(int32_t value)
{
	return value < 0 ? -value : value;
}

enum watch_swipe watch_swipe_classify(int32_t delta_x, int32_t delta_y)
{
	int32_t horizontal = magnitude(delta_x);
	int32_t vertical = magnitude(delta_y);

	if (horizontal >= WATCH_SWIPE_MIN_DISTANCE && horizontal > vertical * 2) {
		return delta_x < 0 ? WATCH_SWIPE_NEXT : WATCH_SWIPE_PREVIOUS;
	}
	if (vertical >= WATCH_SWIPE_MIN_DISTANCE && vertical > horizontal * 2) {
		return delta_y < 0 ? WATCH_SWIPE_NEXT_ITEM : WATCH_SWIPE_PREVIOUS_ITEM;
	}
	return WATCH_SWIPE_NONE;
}
