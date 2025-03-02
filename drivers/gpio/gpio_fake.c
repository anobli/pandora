/*
 * Copyright (c) 2025 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_gpio_fake

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_fake.h>

#include <zephyr/fff.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif /* CONFIG_ZTEST */

struct gpio_fake_config {
	gpio_port_pins_t port_pin_mask;
};

DEFINE_FAKE_VALUE_FUNC(int, gpio_fake_pin_configure, const struct device *, gpio_pin_t,
		       gpio_flags_t);
#ifdef CONFIG_GPIO_GET_CONFIG
DEFINE_FAKE_VALUE_FUNC(int, gpio_fake_pin_get_config, const struct device *, gpio_pin_t,
		       gpio_flags_t *);
#endif
DEFINE_FAKE_VALUE_FUNC(int, gpio_fake_port_get_raw, const struct device *, gpio_port_value_t *);
DEFINE_FAKE_VALUE_FUNC(int, gpio_fake_port_set_masked_raw, const struct device *, gpio_port_pins_t,
		       gpio_port_value_t);
DEFINE_FAKE_VALUE_FUNC(int, gpio_fake_port_set_bits_raw, const struct device *, gpio_port_pins_t);
DEFINE_FAKE_VALUE_FUNC(int, gpio_fake_port_clear_bits_raw, const struct device *, gpio_port_pins_t);
DEFINE_FAKE_VALUE_FUNC(int, gpio_fake_port_toggle_bits, const struct device *, gpio_port_pins_t);
DEFINE_FAKE_VALUE_FUNC(int, gpio_fake_pin_interrupt_configure, const struct device *, gpio_pin_t,
		       enum gpio_int_mode, enum gpio_int_trig);
DEFINE_FAKE_VALUE_FUNC(int, gpio_fake_manage_callback, const struct device *,
		       struct gpio_callback *, bool);
DEFINE_FAKE_VALUE_FUNC(uint32_t, gpio_fake_get_pending_int, const struct device *);
#ifdef CONFIG_GPIO_GET_DIRECTION
DEFINE_FAKE_VALUE_FUNC(int, gpio_fake_port_get_direction, gpio_port_pins_t, gpio_port_pins_t *,
		       gpio_port_pins_t *);
#endif

#ifdef CONFIG_ZTEST
static void gpio_fake_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(gpio_fake_pin_configure);
#ifdef CONFIG_GPIO_GET_CONFIG
	RESET_FAKE(gpio_fake_pin_get_config);
#endif
	RESET_FAKE(gpio_fake_port_get_raw);
	RESET_FAKE(gpio_fake_port_set_masked_raw);
	RESET_FAKE(gpio_fake_port_set_bits_raw);
	RESET_FAKE(gpio_fake_port_clear_bits_raw);
	RESET_FAKE(gpio_fake_port_toggle_bits);
	RESET_FAKE(gpio_fake_pin_interrupt_configure);
	RESET_FAKE(gpio_fake_manage_callback);
	RESET_FAKE(gpio_fake_get_pending_int);
#ifdef CONFIG_GPIO_GET_DIRECTION
	RESET_FAKE(gpio_fake_port_get_direction);
#endif
}

ZTEST_RULE(gpio_fake_reset_rule, gpio_fake_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

static DEVICE_API(gpio, gpio_fake_driver) = {
	.pin_configure = gpio_fake_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_fake_pin_get_config,
#endif
	.port_get_raw = gpio_fake_port_get_raw,
	.port_set_masked_raw = gpio_fake_port_set_masked_raw,
	.port_set_bits_raw = gpio_fake_port_set_bits_raw,
	.port_clear_bits_raw = gpio_fake_port_clear_bits_raw,
	.port_toggle_bits = gpio_fake_port_toggle_bits,
	.pin_interrupt_configure = gpio_fake_pin_interrupt_configure,
	.manage_callback = gpio_fake_manage_callback,
	.get_pending_int = gpio_fake_get_pending_int,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_fake_port_get_direction,
#endif /* CONFIG_GPIO_GET_DIRECTION */
};

#define DEFINE_GPIO_FAKE(_num)                                                                     \
                                                                                                   \
	static struct gpio_fake_config gpio_fake_config##_num = {                                  \
		.port_pin_mask = 0xffffffff,                                                       \
	};                                                                                         \
	static struct gpio_driver_data gpio_fake_data##_num;                                       \
	DEVICE_DT_INST_DEFINE(_num, NULL, PM_DEVICE_DT_INST_GET(_num), &gpio_fake_data##_num,      \
			      &gpio_fake_config##_num, POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,     \
			      &gpio_fake_driver);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_GPIO_FAKE)