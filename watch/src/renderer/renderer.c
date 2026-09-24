/* SPDX-License-Identifier: Apache-2.0 */
#include "renderer/renderer.h"
#include <stdint.h>
#include <stdio.h>
#include <lvgl.h>
#include "animation/watch_animation.h"
#include "apps/app_registry.h"
#include "input/watch_gesture.h"

static renderer_action_handler_t handler;
static const lv_color_t BG = LV_COLOR_MAKE(4, 7, 7);
static const lv_color_t SURFACE = LV_COLOR_MAKE(8, 14, 13);
static const lv_color_t ACCENT = LV_COLOR_MAKE(105, 240, 166);
static const lv_color_t MUTED = LV_COLOR_MAKE(126, 146, 138);
static const lv_color_t LINE = LV_COLOR_MAKE(34, 67, 55);
static lv_point_t swipe_start;
static lv_point_t swipe_last;
static bool swipe_tracking;

static lv_obj_t *label(lv_obj_t *parent, const char *text,
		       const lv_font_t *font, lv_color_t color)
{
	lv_obj_t *object = lv_label_create(parent);
	lv_label_set_text(object, text);
	lv_obj_set_style_text_font(object, font, 0);
	lv_obj_set_style_text_color(object, color, 0);
	return object;
}

static lv_obj_t *face_create(lv_obj_t *screen)
{
	lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_all(screen, 0, 0);
	lv_obj_t *face = lv_obj_create(screen);
	lv_obj_set_size(face, 438, 438);
	lv_obj_center(face);
	lv_obj_set_scrollable(face, false);
	lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(face, BG, 0);
	lv_obj_set_style_bg_opa(face, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(face, 2, 0);
	lv_obj_set_style_border_color(face, LINE, 0);
	lv_obj_set_style_pad_all(face, 0, 0);
	return face;
}

static void gesture_event(lv_event_t *event)
{
	lv_indev_t *input = lv_indev_active();
	if (input == NULL) return;
	lv_event_code_t code = lv_event_get_code(event);
	if (code == LV_EVENT_PRESSED) {
		lv_indev_get_point(input, &swipe_start);
		swipe_last = swipe_start;
		swipe_tracking = true;
	} else if (code == LV_EVENT_PRESSING && swipe_tracking) {
		lv_indev_get_point(input, &swipe_last);
	} else if ((code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) && swipe_tracking) {
		lv_indev_get_point(input, &swipe_last);
		int32_t dx = swipe_last.x - swipe_start.x;
		int32_t dy = swipe_last.y - swipe_start.y;
		swipe_tracking = false;
		enum watch_swipe swipe = watch_swipe_classify(dx, dy);
		if (handler != NULL && swipe != WATCH_SWIPE_NONE) {
			handler(swipe == WATCH_SWIPE_NEXT
				? RENDERER_ACTION_NEXT : RENDERER_ACTION_PREVIOUS);
		}
	}
}

static void gesture_layer_create(lv_obj_t *face)
{
	lv_obj_t *layer = lv_obj_create(face);
	lv_obj_remove_style_all(layer);
	lv_obj_set_size(layer, 438, 438);
	lv_obj_center(layer);
	lv_obj_set_clickable(layer, true);
	lv_obj_set_scrollable(layer, false);
	lv_obj_add_event_cb(layer, gesture_event, LV_EVENT_PRESSED, NULL);
	lv_obj_add_event_cb(layer, gesture_event, LV_EVENT_PRESSING, NULL);
	lv_obj_add_event_cb(layer, gesture_event, LV_EVENT_RELEASED, NULL);
	lv_obj_add_event_cb(layer, gesture_event, LV_EVENT_PRESS_LOST, NULL);
}

static void page_indicator(lv_obj_t *face, enum watch_screen screen)
{
	char text[16];
	snprintf(text, sizeof(text), "< %02u/%02u >", (unsigned int)screen + 1U,
		 (unsigned int)WATCH_SCREEN_COUNT);
	lv_obj_t *object = label(face, text, &lv_font_unscii_16, MUTED);
	lv_obj_align(object, LV_ALIGN_BOTTOM_MID, 0, -43);
}

static void render_home(lv_obj_t *face, const struct watch_state *state)
{
	char battery[8];
	char temperature[12];
	char steps[24];
	char condition[48];
	snprintf(battery, sizeof(battery), "%u%%", state->battery_percent);
	snprintf(temperature, sizeof(temperature), "%d°C", state->temperature_c);
	snprintf(steps, sizeof(steps), "%u STEPS", state->steps);
	snprintf(condition, sizeof(condition), "%s", state->weather_condition);

	lv_obj_t *object = label(face, state->time_text, &lv_font_unscii_16, lv_color_white());
	lv_obj_align(object, LV_ALIGN_TOP_LEFT, 82, 42);
	object = label(face, battery, &lv_font_unscii_16, MUTED);
	lv_obj_align(object, LV_ALIGN_TOP_RIGHT, -82, 42);
	object = label(face, state->date_text, &lv_font_unscii_16, MUTED);
	lv_obj_align(object, LV_ALIGN_TOP_MID, 0, 96);

	lv_obj_t *weather = lv_obj_create(face);
	lv_obj_remove_style_all(weather);
	lv_obj_set_size(weather, 250, 112);
	lv_obj_align(weather, LV_ALIGN_CENTER, 0, -8);
	lv_obj_set_style_bg_opa(weather, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(weather, 0, 0);
	lv_obj_set_style_shadow_width(weather, 0, 0);
	object = label(weather, temperature, &lv_font_montserrat_28, lv_color_white());
	lv_obj_align(object, LV_ALIGN_CENTER, 0, -16);
	object = label(weather, condition, &lv_font_unscii_16, MUTED);
	lv_obj_align(object, LV_ALIGN_CENTER, 0, 20);
	object = label(face, steps, &lv_font_unscii_16, lv_color_white());
	lv_obj_align(object, LV_ALIGN_BOTTOM_MID, 0, -112);
	page_indicator(face, state->active_screen);
}

static void render_app(lv_obj_t *face, const struct watch_state *state)
{
	bool notifications = state->active_screen == WATCH_SCREEN_NOTIFICATIONS;
	bool music = state->active_screen == WATCH_SCREEN_MUSIC;
	const struct watch_app_descriptor *app =
		app_registry_find(state->active_screen);
	const char *title = app != NULL ? app->name : "Unknown";
	const char *headline = notifications && state->notification_count > 0
		? state->notification_title
		: (music ? state->music_title : (app != NULL ? app->headline : "Unavailable"));
	const char *detail = notifications && state->notification_count > 0
		? state->notification_body
		: (music ? (state->music_playing ? "Playing from your connected host"
						 : "Playback is paused")
			 : (app != NULL ? app->offline_message : "Application metadata is missing."));
	const char *status = state->connection == WATCH_CONNECTION_ONLINE ? "HOST SYNCED"
								      : "OFFLINE READY";
	lv_obj_t *object = label(face, status, &lv_font_unscii_16,
				 ACCENT);
	lv_obj_align(object, LV_ALIGN_TOP_LEFT, 76, 48);
	object = label(face, title, &lv_font_unscii_16, lv_color_white());
	lv_obj_align(object, LV_ALIGN_TOP_LEFT, 76, 78);

	lv_obj_t *card = lv_obj_create(face);
	lv_obj_set_size(card, 350, 210);
	lv_obj_align(card, LV_ALIGN_CENTER, 0, 20);
	lv_obj_set_style_radius(card, 6, 0);
	lv_obj_set_style_bg_color(card, SURFACE, 0);
	lv_obj_set_style_border_width(card, 1, 0);
	lv_obj_set_style_border_color(card, LINE, 0);
	lv_obj_set_style_pad_all(card, 18, 0);
	object = label(card, headline, &lv_font_unscii_16, lv_color_white());
	lv_obj_align(object, LV_ALIGN_TOP_LEFT, 0, 0);
	lv_obj_set_width(object, 310);
	lv_label_set_long_mode(object, LV_LABEL_LONG_WRAP);
	object = label(card, detail, &lv_font_unscii_16, MUTED);
	lv_obj_align(object, LV_ALIGN_TOP_LEFT, 0, 36);
	lv_obj_set_width(object, 310);
	lv_label_set_long_mode(object, LV_LABEL_LONG_WRAP);

	page_indicator(face, state->active_screen);
}

void renderer_init(renderer_action_handler_t action_handler)
{
	handler = action_handler;
}

void renderer_render(const struct watch_state *state, bool backwards)
{
	lv_obj_t *screen = lv_obj_create(NULL);
	lv_obj_t *face = face_create(screen);
	if (state->active_screen == WATCH_SCREEN_HOME) {
		render_home(face, state);
	} else {
		render_app(face, state);
	}
	gesture_layer_create(face);
	watch_animation_load(screen, backwards);
}

void renderer_refresh(const struct watch_state *state)
{
	lv_obj_t *screen = lv_obj_create(NULL);
	lv_obj_t *face = face_create(screen);
	if (state->active_screen == WATCH_SCREEN_HOME) {
		render_home(face, state);
	} else {
		render_app(face, state);
	}
	gesture_layer_create(face);
	lv_screen_load_anim(screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
}
