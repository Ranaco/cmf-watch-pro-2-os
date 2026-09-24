/* SPDX-License-Identifier: Apache-2.0 */
#include "runtime/watch_runtime.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <lvgl.h>
#include "cache/watch_cache.h"
#include "input/watch_input.h"
#include "navigation/navigation.h"
#include "renderer/renderer.h"
#include "state/watch_state.h"
#include "transport/transport.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(watch_runtime);

static struct watch_state state;
static struct watch_navigation navigation;

static void render_current(bool backwards)
{
	watch_state_set_screen(&state, navigation_current(&navigation));
	LOG_INF("Navigation: %s", watch_screen_name(state.active_screen));
	renderer_render(&state, backwards);
}

static void handle_action(enum renderer_action action)
{
	bool changed = false;
	bool backwards = false;
	switch (action) {
	case RENDERER_ACTION_OPEN_NOTIFICATIONS:
		changed = navigation_push(&navigation, WATCH_SCREEN_NOTIFICATIONS);
		break;
	case RENDERER_ACTION_OPEN_MUSIC:
		changed = navigation_push(&navigation, WATCH_SCREEN_MUSIC);
		break;
	case RENDERER_ACTION_BACK:
		changed = navigation_pop(&navigation);
		backwards = true;
		break;
	default:
		break;
	}
	if (changed) {
		render_current(backwards);
	}
}

int watch_runtime_run(void)
{
	const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display)) {
		LOG_ERR("Display device is not ready");
		return -ENODEV;
	}
	watch_state_init(&state);
	navigation_init(&navigation);
	watch_cache_init();
	transport_init();
	renderer_init(handle_action);
	if (watch_input_init() != 0) {
		LOG_WRN("Hardware shortcut unavailable; pointer input remains active");
	}
	render_current(false);
	lv_timer_handler();
	if (display_blanking_off(display) < 0) {
		LOG_WRN("Display blanking control is unavailable");
	}
	LOG_INF("CMF simulator ready at round 466x466 (offline=%d)",
		!transport_connected());
	while (true) {
		if (watch_input_poll() == WATCH_INPUT_HARDWARE_BUTTON) {
			handle_action(RENDERER_ACTION_OPEN_NOTIFICATIONS);
		}
		lv_timer_handler();
		k_sleep(K_MSEC(10));
	}
	return 0;
}
