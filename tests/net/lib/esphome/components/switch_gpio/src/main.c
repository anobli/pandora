/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

#include <esphome/esphome.h>
#include <esphome/components/switch.h>

struct esphome_switch_gpio_tests_fixture {
	const struct device *dev;
	const struct gpio_dt_spec gpio;
};

static void *switch_gpio_setup(void)
{
	static struct esphome_switch_gpio_tests_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_PATH(gpio_switch)),
		.gpio = GPIO_DT_SPEC_GET(DT_PATH(gpio_switch), gpios),
	};
	return &fixture;
}

ZTEST_SUITE(esphome_switch_gpio_tests, NULL, switch_gpio_setup, NULL, NULL, NULL);

ZTEST_F(esphome_switch_gpio_tests, test_esphome_switch_switch_gpio_set_state_on)
{
	int ret;

	ret = esphome_switch_set_state(fixture->dev, 1);
	zassert_equal(ret, 0);
	zassert_equal(gpio_emul_output_get(fixture->gpio.port, 0), 1);
}

ZTEST_F(esphome_switch_gpio_tests, test_esphome_switch_gpio_set_state_off)
{
	int ret;

	ret = esphome_switch_set_state(fixture->dev, 0);
	zassert_equal(ret, 0);
	zassert_equal(gpio_emul_output_get(fixture->gpio.port, 0), 0);
}

ZTEST_F(esphome_switch_gpio_tests, test_esphome_switch_gpio_get_state)
{
	int state;
	int ret;

	ret = esphome_switch_set_state(fixture->dev, 1);
	zassert_equal(ret, 0);

	ret = esphome_switch_get_state(fixture->dev, &state);
	zassert_equal(ret, 0);
	zassert_equal(state, 1);

	ret = esphome_switch_set_state(fixture->dev, 0);
	zassert_equal(ret, 0);

	ret = esphome_switch_get_state(fixture->dev, &state);
	zassert_equal(ret, 0);
	zassert_equal(state, 0);
}
