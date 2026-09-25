/* SPDX-License-Identifier: Apache-2.0 */
#ifndef CMF_WATCH_PLATFORM_H
#define CMF_WATCH_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

enum watch_platform_capability {
	WATCH_PLATFORM_DISPLAY = 1U << 0,
	WATCH_PLATFORM_TOUCH = 1U << 1,
	WATCH_PLATFORM_BUTTON = 1U << 2,
	WATCH_PLATFORM_CROWN = 1U << 3,
	WATCH_PLATFORM_BATTERY = 1U << 4,
	WATCH_PLATFORM_RTC = 1U << 5,
	WATCH_PLATFORM_STORAGE = 1U << 6,
	WATCH_PLATFORM_BLE = 1U << 7,
	WATCH_PLATFORM_SENSORS = 1U << 8,
	WATCH_PLATFORM_POWER = 1U << 9,
};

struct watch_platform_info {
	const char *name;
	uint16_t display_width;
	uint16_t display_height;
	uint32_t capabilities;
	bool round_display;
};

/*
 * The application depends on this small platform contract, while concrete
 * devices continue to use Zephyr's display/input/storage/BLE driver APIs.
 * A physical CMF backend must not advertise a capability until its mapping is
 * independently verified on the target hardware.
 */
int watch_platform_init(void);
const struct watch_platform_info *watch_platform_get_info(void);
bool watch_platform_has(enum watch_platform_capability capability);

#endif
