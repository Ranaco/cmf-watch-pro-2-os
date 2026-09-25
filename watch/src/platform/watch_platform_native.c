/* SPDX-License-Identifier: Apache-2.0 */
#include "platform/watch_platform.h"

static const struct watch_platform_info native_info = {
	.name = "native_sim",
	.display_width = 466,
	.display_height = 466,
	.capabilities = WATCH_PLATFORM_DISPLAY | WATCH_PLATFORM_TOUCH |
			WATCH_PLATFORM_BUTTON | WATCH_PLATFORM_RTC |
			WATCH_PLATFORM_STORAGE | WATCH_PLATFORM_POWER,
	.round_display = true,
};

int watch_platform_init(void)
{
	return 0;
}

const struct watch_platform_info *watch_platform_get_info(void)
{
	return &native_info;
}

bool watch_platform_has(enum watch_platform_capability capability)
{
	return (native_info.capabilities & (uint32_t)capability) != 0U;
}
