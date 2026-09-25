#include <assert.h>
#include <stdio.h>
#include "platform/watch_platform.h"

int main(void)
{
	const struct watch_platform_info *info;

	assert(watch_platform_init() == 0);
	info = watch_platform_get_info();
	assert(info != NULL);
	assert(info->display_width == 466U);
	assert(info->display_height == 466U);
	assert(info->round_display);
	assert(watch_platform_has(WATCH_PLATFORM_DISPLAY));
	assert(watch_platform_has(WATCH_PLATFORM_TOUCH));
	assert(watch_platform_has(WATCH_PLATFORM_STORAGE));
	assert(!watch_platform_has(WATCH_PLATFORM_BLE));
	assert(!watch_platform_has(WATCH_PLATFORM_CROWN));
	puts("platform contract tests passed");
	return 0;
}
