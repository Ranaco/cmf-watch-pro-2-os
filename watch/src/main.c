/*
 * CMF Watch Pro 2 desktop simulator
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <lvgl.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cmf_watch_simulator);

static const struct gpio_dt_spec hardware_button =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_callback hardware_button_callback;
static atomic_t navigation_requested;

static const lv_color_t COLOR_BACKGROUND = LV_COLOR_MAKE(8, 10, 13);
static const lv_color_t COLOR_SURFACE = LV_COLOR_MAKE(25, 29, 34);
static const lv_color_t COLOR_ACCENT = LV_COLOR_MAKE(255, 111, 64);
static const lv_color_t COLOR_MUTED = LV_COLOR_MAKE(154, 163, 173);

static void configure_screen(lv_obj_t *screen)
{
	lv_obj_set_style_bg_color(screen, COLOR_BACKGROUND, 0);
	lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
	lv_obj_set_style_text_color(screen, lv_color_white(), 0);
	lv_obj_set_style_pad_all(screen, 0, 0);
}

static lv_obj_t *make_round_face(lv_obj_t *screen)
{
	lv_obj_t *face = lv_obj_create(screen);
	/* Keep the panel outline clear of SDL/window decoration and antialiasing. */
	lv_obj_set_size(face, 438, 438);
	lv_obj_center(face);
	lv_obj_set_scrollable(face, false);
	lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(face, COLOR_BACKGROUND, 0);
	lv_obj_set_style_bg_opa(face, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(face, 2, 0);
	lv_obj_set_style_border_color(face, lv_color_make(45, 51, 58), 0);
	lv_obj_set_style_pad_all(face, 0, 0);
	return face;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text,
			    const lv_font_t *font, lv_color_t color)
{
	lv_obj_t *label = lv_label_create(parent);
	lv_label_set_text(label, text);
	lv_obj_set_style_text_font(label, font, 0);
	lv_obj_set_style_text_color(label, color, 0);
	return label;
}

static void show_home(void);
static void show_notifications(void);

static void open_notifications_event(lv_event_t *event)
{
	ARG_UNUSED(event);
	show_notifications();
}

static void back_home_event(lv_event_t *event)
{
	ARG_UNUSED(event);
	show_home();
}

static void show_home(void)
{
	LOG_INF("Navigation: home");
	lv_obj_t *screen = lv_obj_create(NULL);
	configure_screen(screen);
	lv_obj_t *face = make_round_face(screen);

	lv_obj_t *time = make_label(face, "21:45", &lv_font_montserrat_20,
				    lv_color_white());
	lv_obj_align(time, LV_ALIGN_TOP_LEFT, 72, 34);

	lv_obj_t *battery = make_label(face, "72%", &lv_font_montserrat_14,
				       COLOR_MUTED);
	lv_obj_align(battery, LV_ALIGN_TOP_RIGHT, -72, 39);

	lv_obj_t *day = make_label(face, "THURSDAY", &lv_font_montserrat_14,
				   COLOR_ACCENT);
	lv_obj_align(day, LV_ALIGN_TOP_MID, 0, 104);
	lv_obj_set_style_text_letter_space(day, 2, 0);

	lv_obj_t *date = make_label(face, "September 24", &lv_font_montserrat_28,
				    lv_color_white());
	lv_obj_align(date, LV_ALIGN_TOP_MID, 0, 132);

	lv_obj_t *weather = lv_button_create(face);
	lv_obj_set_size(weather, 190, 92);
	lv_obj_align(weather, LV_ALIGN_CENTER, 0, 24);
	lv_obj_set_style_radius(weather, 28, 0);
	lv_obj_set_style_bg_color(weather, COLOR_SURFACE, 0);
	lv_obj_set_style_bg_opa(weather, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(weather, 1, 0);
	lv_obj_set_style_border_color(weather, lv_color_make(48, 54, 61), 0);
	lv_obj_set_style_shadow_width(weather, 18, 0);
	lv_obj_set_style_shadow_opa(weather, LV_OPA_20, 0);
	lv_obj_add_event_cb(weather, open_notifications_event, LV_EVENT_CLICKED, NULL);

	lv_obj_t *temperature = make_label(weather, "28°C", &lv_font_montserrat_28,
					   lv_color_white());
	lv_obj_align(temperature, LV_ALIGN_TOP_MID, 0, 10);
	lv_obj_t *condition = make_label(weather, "Clear / feels like 29°",
					 &lv_font_montserrat_14, COLOR_MUTED);
	lv_obj_align(condition, LV_ALIGN_BOTTOM_MID, 0, -11);

	lv_obj_t *steps = make_label(face, "7,421 steps", &lv_font_montserrat_20,
				     lv_color_white());
	lv_obj_align(steps, LV_ALIGN_BOTTOM_MID, 0, -76);

	lv_obj_t *hint = make_label(face, "Tap card or press R",
				    &lv_font_montserrat_14, COLOR_MUTED);
	lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -48);

	lv_screen_load_anim(screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 260, 0, true);
}

static void show_notifications(void)
{
	LOG_INF("Navigation: notifications");
	lv_obj_t *screen = lv_obj_create(NULL);
	configure_screen(screen);
	lv_obj_t *face = make_round_face(screen);

	lv_obj_t *eyebrow = make_label(face, "OFFLINE READY",
				       &lv_font_montserrat_14, COLOR_ACCENT);
	lv_obj_align(eyebrow, LV_ALIGN_TOP_LEFT, 76, 48);
	lv_obj_set_style_text_letter_space(eyebrow, 2, 0);

	lv_obj_t *title = make_label(face, "Notifications",
				     &lv_font_montserrat_28, lv_color_white());
	lv_obj_align(title, LV_ALIGN_TOP_LEFT, 76, 76);

	lv_obj_t *card = lv_obj_create(face);
	lv_obj_set_size(card, 360, 166);
	lv_obj_align(card, LV_ALIGN_CENTER, 0, 5);
	lv_obj_set_style_radius(card, 28, 0);
	lv_obj_set_style_bg_color(card, COLOR_SURFACE, 0);
	lv_obj_set_style_border_width(card, 0, 0);
	lv_obj_set_style_pad_all(card, 22, 0);

	lv_obj_t *empty = make_label(card, "You're all caught up",
				     &lv_font_montserrat_20, lv_color_white());
	lv_obj_align(empty, LV_ALIGN_TOP_LEFT, 0, 0);
	lv_obj_t *detail = make_label(card,
		"This screen opened locally.\nNo host connection was required.",
		&lv_font_montserrat_14, COLOR_MUTED);
	lv_obj_align(detail, LV_ALIGN_TOP_LEFT, 0, 40);
	lv_obj_set_style_text_line_space(detail, 7, 0);

	lv_obj_t *back = lv_button_create(face);
	lv_obj_set_size(back, 154, 48);
	lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -48);
	lv_obj_set_style_radius(back, 24, 0);
	lv_obj_set_style_bg_color(back, COLOR_ACCENT, 0);
	lv_obj_add_event_cb(back, back_home_event, LV_EVENT_CLICKED, NULL);
	lv_obj_t *back_label = make_label(back, "Back home",
					  &lv_font_montserrat_14, lv_color_white());
	lv_obj_center(back_label);

	lv_screen_load_anim(screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 260, 0, true);
}

static void hardware_button_isr(const struct device *port,
				struct gpio_callback *callback, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(callback);
	ARG_UNUSED(pins);
	atomic_set(&navigation_requested, 1);
}

static int configure_hardware_button(void)
{
	int error;

	if (!gpio_is_ready_dt(&hardware_button)) {
		LOG_ERR("Simulator hardware button is not ready");
		return -ENODEV;
	}

	error = gpio_pin_configure_dt(&hardware_button, GPIO_INPUT);
	if (error != 0) {
		return error;
	}

	gpio_init_callback(&hardware_button_callback, hardware_button_isr,
			   BIT(hardware_button.pin));
	error = gpio_add_callback(hardware_button.port, &hardware_button_callback);
	if (error != 0) {
		return error;
	}

	return gpio_pin_interrupt_configure_dt(&hardware_button,
					       GPIO_INT_EDGE_TO_ACTIVE);
}

int main(void)
{
	const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(display)) {
		LOG_ERR("SDL display device is not ready");
		return 0;
	}

	if (configure_hardware_button() != 0) {
		LOG_WRN("Keyboard shortcut unavailable; pointer input still works");
	}

	show_home();
	lv_timer_handler();

	if (display_blanking_off(display) < 0) {
		LOG_WRN("Display blanking control is unavailable");
	}

	LOG_INF("CMF simulator ready at round 466x466");

	while (true) {
		if (atomic_cas(&navigation_requested, 1, 0)) {
			show_notifications();
		}

		lv_timer_handler();
		k_sleep(K_MSEC(10));
	}

	return 0;
}
