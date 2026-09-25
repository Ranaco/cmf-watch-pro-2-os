/* SPDX-License-Identifier: Apache-2.0 */
#include "input/watch_input.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/atomic.h>

static const struct gpio_dt_spec hardware_button =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static const struct gpio_dt_spec activate_button =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0});
static struct gpio_callback hardware_button_callback;
static struct gpio_callback activate_button_callback;
static atomic_t pending_button;
static atomic_t pending_activate;

static void button_isr(const struct device *port, struct gpio_callback *callback,
		       uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(callback);
	ARG_UNUSED(pins);
	atomic_set(&pending_button, 1);
}

static void activate_isr(const struct device *port, struct gpio_callback *callback,
			 uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(callback);
	ARG_UNUSED(pins);
	atomic_set(&pending_activate, 1);
}

int watch_input_init(void)
{
	int error;
	if (!gpio_is_ready_dt(&hardware_button)) {
		return -ENODEV;
	}
	error = gpio_pin_configure_dt(&hardware_button, GPIO_INPUT);
	if (error != 0) {
		return error;
	}
	gpio_init_callback(&hardware_button_callback, button_isr,
			   BIT(hardware_button.pin));
	error = gpio_add_callback(hardware_button.port, &hardware_button_callback);
	if (error != 0) {
		return error;
	}
	error = gpio_pin_interrupt_configure_dt(&hardware_button, GPIO_INT_EDGE_TO_ACTIVE);
	if (error != 0) return error;
	if (!gpio_is_ready_dt(&activate_button)) return -ENODEV;
	error = gpio_pin_configure_dt(&activate_button, GPIO_INPUT);
	if (error != 0) return error;
	gpio_init_callback(&activate_button_callback, activate_isr, BIT(activate_button.pin));
	error = gpio_add_callback(activate_button.port, &activate_button_callback);
	return error != 0 ? error : gpio_pin_interrupt_configure_dt(&activate_button,
							    GPIO_INT_EDGE_TO_ACTIVE);
}

enum watch_input_event watch_input_poll(void)
{
	if (atomic_cas(&pending_activate, 1, 0)) return WATCH_INPUT_ACTIVATE;
	return atomic_cas(&pending_button, 1, 0)
		? WATCH_INPUT_HARDWARE_BUTTON : WATCH_INPUT_NONE;
}
