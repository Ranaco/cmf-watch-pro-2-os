/* SPDX-License-Identifier: Apache-2.0 */
#include "runtime/watch_runtime.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <lvgl.h>
#include "cache/watch_cache.h"
#include "cache/watch_cache_storage.h"
#include "input/watch_input.h"
#include "navigation/navigation.h"
#include "renderer/renderer.h"
#include "state/watch_state.h"
#include "state/watch_sync.h"
#include "transport/transport.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(watch_runtime);

static struct watch_state state;
static struct watch_navigation navigation;
static struct watch_sync sync_state;
static struct watch_cache_record cache;
static bool runtime_ready;

static void render_current(bool backwards)
{
	watch_state_set_screen(&state, navigation_current(&navigation));
	LOG_INF("Navigation: %s", watch_screen_name(state.active_screen));
	renderer_render(&state, backwards);
}

static int handle_protocol_message(const struct watch_protocol_message *message, void *context)
{
	ARG_UNUSED(context);
	enum watch_sync_result result = watch_sync_handle(&sync_state, &state, message);
	if (result == WATCH_SYNC_ERROR) {
		LOG_WRN("Rejected semantic message: %s", watch_message_type_name(message->type));
		return -EINVAL;
	}
	if (result == WATCH_SYNC_NEEDS_SNAPSHOT) {
		LOG_WRN("Revision gap after %llu; requesting snapshot",
			(unsigned long long)watch_sync_revision(&sync_state));
		return transport_request_sync(watch_sync_revision(&sync_state));
	}
	if (result == WATCH_SYNC_CHANGED) {
		state.connection = WATCH_CONNECTION_ONLINE;
		state.host_data_stale = false;
		state.host_data_updated_at_ms = watch_cache_storage_now_ms();
		watch_cache_capture_sync(&cache, &state, watch_sync_revision(&sync_state),
					 state.host_data_updated_at_ms);
		(void)watch_cache_storage_save(&cache);
		LOG_INF("State synchronized revision=%llu temp=%d steps=%u notifications=%u",
			(unsigned long long)watch_sync_revision(&sync_state), state.temperature_c,
			state.steps, state.notifications.total_count);
		if (runtime_ready) renderer_refresh(&state);
	}
	return 0;
}

static void handle_transport_event(enum transport_event event, void *context)
{
	ARG_UNUSED(context);
	if (event == TRANSPORT_EVENT_CONNECTED) {
		state.connection = WATCH_CONNECTION_CONNECTING;
		watch_sync_connected(&sync_state);
	} else {
		state.connection = WATCH_CONNECTION_OFFLINE;
		watch_sync_disconnected(&sync_state);
		if (cache.weather_meta.version > 0U) {
			state.host_data_stale = true;
			watch_cache_mark_stale(&cache, watch_cache_storage_now_ms());
			(void)watch_cache_storage_save(&cache);
		}
	}
	if (runtime_ready) renderer_refresh(&state);
}

static void handle_action(enum renderer_action action)
{
	bool changed = false;
	bool backwards = false;
	switch (action) {
	case RENDERER_ACTION_NEXT:
		changed = navigation_next(&navigation);
		break;
	case RENDERER_ACTION_PREVIOUS:
		changed = navigation_previous(&navigation);
		backwards = true;
		break;
	case RENDERER_ACTION_NEXT_ITEM:
		if (state.active_screen == WATCH_SCREEN_NOTIFICATIONS) {
			changed = watch_state_notification_next(&state);
		}
		break;
	case RENDERER_ACTION_PREVIOUS_ITEM:
		if (state.active_screen == WATCH_SCREEN_NOTIFICATIONS) {
			changed = watch_state_notification_previous(&state);
		}
		break;
	default:
		break;
	}
	if (changed) {
		if (action == RENDERER_ACTION_NEXT_ITEM ||
		    action == RENDERER_ACTION_PREVIOUS_ITEM) renderer_refresh(&state);
		else render_current(backwards);
		watch_cache_capture_application(&cache, &state,
						watch_cache_storage_now_ms());
		(void)watch_cache_storage_save(&cache);
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
	watch_sync_init(&sync_state);
	navigation_init(&navigation);
	watch_cache_init(&cache);
	int cache_result = watch_cache_storage_load(&cache);
	if (cache_result == 0 && watch_cache_restore(&cache, &state)) {
		navigation_replace(&navigation, state.active_screen);
		LOG_INF("Restored cache revision=%llu temp=%d steps=%u notifications=%u stale=%d",
			(unsigned long long)cache.synchronized_revision,
			state.temperature_c, state.steps, state.notifications.total_count,
			state.host_data_stale);
	} else if (cache_result == -EBADMSG) {
		LOG_WRN("Ignoring invalid cache record");
		watch_cache_init(&cache);
	}
	renderer_init(handle_action);
	transport_init(handle_protocol_message, handle_transport_event, NULL);
	if (watch_input_init() != 0) {
		LOG_WRN("Hardware shortcut unavailable; pointer input remains active");
	}
	render_current(false);
	runtime_ready = true;
	lv_timer_handler();
	if (display_blanking_off(display) < 0) {
		LOG_WRN("Display blanking control is unavailable");
	}
	LOG_INF("CMF simulator ready at round 466x466 (offline=%d)",
		!transport_connected());
	while (true) {
		transport_poll();
		if (watch_input_poll() == WATCH_INPUT_HARDWARE_BUTTON) {
			handle_action(RENDERER_ACTION_NEXT);
		}
		lv_timer_handler();
		k_sleep(K_MSEC(10));
	}
	return 0;
}
