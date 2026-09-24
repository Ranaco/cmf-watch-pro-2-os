/* SPDX-License-Identifier: Apache-2.0 */
#include "renderer/renderer.h"
#include <stdint.h>
#include <stdio.h>
#include <lvgl.h>
#include "animation/watch_animation.h"

static renderer_action_handler_t handler;
static const lv_color_t BG = LV_COLOR_MAKE(8, 10, 13);
static const lv_color_t SURFACE = LV_COLOR_MAKE(25, 29, 34);
static const lv_color_t ACCENT = LV_COLOR_MAKE(255, 111, 64);
static const lv_color_t MUTED = LV_COLOR_MAKE(154, 163, 173);

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
	lv_obj_set_style_border_color(face, lv_color_make(45, 51, 58), 0);
	lv_obj_set_style_pad_all(face, 0, 0);
	return face;
}

static void action_event(lv_event_t *event)
{
	if (handler != NULL) {
		handler((enum renderer_action)(uintptr_t)lv_event_get_user_data(event));
	}
}

static void button_create(lv_obj_t *parent, const char *text, lv_align_t align,
			  int32_t x, int32_t y, int32_t width,
			  enum renderer_action action)
{
	lv_obj_t *button = lv_button_create(parent);
	lv_obj_set_size(button, width, 48);
	lv_obj_align(button, align, x, y);
	lv_obj_set_style_radius(button, 24, 0);
	lv_obj_set_style_bg_color(button, ACCENT, 0);
	lv_obj_add_event_cb(button, action_event, LV_EVENT_CLICKED,
			    (void *)(uintptr_t)action);
	lv_obj_t *button_label = label(button, text, &lv_font_montserrat_14,
					       lv_color_white());
	lv_obj_center(button_label);
}

static void render_home(lv_obj_t *face, const struct watch_state *state)
{
	char battery[8];
	char temperature[12];
	char steps[24];
	snprintf(battery, sizeof(battery), "%u%%", state->battery_percent);
	snprintf(temperature, sizeof(temperature), "%d°C", state->temperature_c);
	snprintf(steps, sizeof(steps), "%u steps", state->steps);

	lv_obj_t *object = label(face, state->time_text, &lv_font_montserrat_20,
				 lv_color_white());
	lv_obj_align(object, LV_ALIGN_TOP_LEFT, 72, 34);
	object = label(face, battery, &lv_font_montserrat_14, MUTED);
	lv_obj_align(object, LV_ALIGN_TOP_RIGHT, -72, 39);
	object = label(face, state->day_text, &lv_font_montserrat_14, ACCENT);
	lv_obj_align(object, LV_ALIGN_TOP_MID, 0, 104);
	lv_obj_set_style_text_letter_space(object, 2, 0);
	object = label(face, state->date_text, &lv_font_montserrat_28,
		       lv_color_white());
	lv_obj_align(object, LV_ALIGN_TOP_MID, 0, 132);

	lv_obj_t *weather = lv_button_create(face);
	lv_obj_set_size(weather, 190, 92);
	lv_obj_align(weather, LV_ALIGN_CENTER, 0, 24);
	lv_obj_set_style_radius(weather, 28, 0);
	lv_obj_set_style_bg_color(weather, SURFACE, 0);
	lv_obj_set_style_border_width(weather, 1, 0);
	lv_obj_set_style_border_color(weather, lv_color_make(48, 54, 61), 0);
	lv_obj_set_style_shadow_width(weather, 18, 0);
	lv_obj_set_style_shadow_opa(weather, LV_OPA_20, 0);
	lv_obj_add_event_cb(weather, action_event, LV_EVENT_CLICKED,
			    (void *)(uintptr_t)RENDERER_ACTION_OPEN_NOTIFICATIONS);
	object = label(weather, temperature, &lv_font_montserrat_28,
		       lv_color_white());
	lv_obj_align(object, LV_ALIGN_TOP_MID, 0, 10);
	object = label(weather, "Clear / feels like 29°", &lv_font_montserrat_14,
		       MUTED);
	lv_obj_align(object, LV_ALIGN_BOTTOM_MID, 0, -11);
	object = label(face, steps, &lv_font_montserrat_20, lv_color_white());
	lv_obj_align(object, LV_ALIGN_BOTTOM_MID, 0, -76);
	object = label(face, "Tap card or press R", &lv_font_montserrat_14, MUTED);
	lv_obj_align(object, LV_ALIGN_BOTTOM_MID, 0, -48);
}

static void render_app(lv_obj_t *face, const struct watch_state *state)
{
	bool notifications = state->active_screen == WATCH_SCREEN_NOTIFICATIONS;
	const char *title = notifications ? "Notifications" : "Music";
	const char *headline = notifications ? "You're all caught up"
					     : state->music_title;
	const char *detail = notifications ? "Opened locally. No host required."
					   : "Cached controls available offline.";
	lv_obj_t *object = label(face, "OFFLINE READY", &lv_font_montserrat_14,
				 ACCENT);
	lv_obj_align(object, LV_ALIGN_TOP_LEFT, 76, 48);
	lv_obj_set_style_text_letter_space(object, 2, 0);
	object = label(face, title, &lv_font_montserrat_28, lv_color_white());
	lv_obj_align(object, LV_ALIGN_TOP_LEFT, 76, 76);

	lv_obj_t *card = lv_obj_create(face);
	lv_obj_set_size(card, 360, 166);
	lv_obj_align(card, LV_ALIGN_CENTER, 0, 5);
	lv_obj_set_style_radius(card, 28, 0);
	lv_obj_set_style_bg_color(card, SURFACE, 0);
	lv_obj_set_style_border_width(card, 0, 0);
	lv_obj_set_style_pad_all(card, 22, 0);
	object = label(card, headline, &lv_font_montserrat_20, lv_color_white());
	lv_obj_align(object, LV_ALIGN_TOP_LEFT, 0, 0);
	object = label(card, detail, &lv_font_montserrat_14, MUTED);
	lv_obj_align(object, LV_ALIGN_TOP_LEFT, 0, 42);

	button_create(face, "Back", LV_ALIGN_BOTTOM_LEFT, 70, -48, 130,
		      RENDERER_ACTION_BACK);
	if (notifications) {
		button_create(face, "Music", LV_ALIGN_BOTTOM_RIGHT, -70, -48, 130,
			      RENDERER_ACTION_OPEN_MUSIC);
	}
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
	watch_animation_load(screen, backwards);
}
