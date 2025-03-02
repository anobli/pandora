/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_fake.h>

#include <esphome/esphome.h>
#include <esphome/components/switch.h>

#include <zephyr/fff.h>
DEFINE_FFF_GLOBALS;

struct esphome_hbridge_tests_fixture {
	const struct device *dev;
	const struct gpio_dt_spec gpio_on;
	const struct gpio_dt_spec gpio_off;
};

static void *switch_hbridge_setup(void)
{
	static struct esphome_hbridge_tests_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_PATH(hbridge_switch)),
		.gpio_on = GPIO_DT_SPEC_GET(DT_PATH(hbridge_switch), on_gpios),
		.gpio_off = GPIO_DT_SPEC_GET(DT_PATH(hbridge_switch), off_gpios),
	};
	return &fixture;
}

ZTEST_SUITE(esphome_hbridge_tests, NULL, switch_hbridge_setup, NULL, NULL, NULL);

ZTEST_F(esphome_hbridge_tests, test_esphome_switch_hbridge_set_state_off)
{

	int ret;

	ret = esphome_switch_set_state(fixture->dev, 0);
	zassert_equal(ret, 0);

	/* First we clear gpio on*/
	zassert_equal(fixture->gpio_on.port, gpio_fake_port_clear_bits_raw_fake.arg0_history[0]);
	zassert_equal(1, gpio_fake_port_clear_bits_raw_fake.arg1_history[0]);

	/* Then we set gpio off */
	zassert_equal(fixture->gpio_off.port, gpio_fake_port_set_bits_raw_fake.arg0_history[0]);
	zassert_equal(2, gpio_fake_port_set_bits_raw_fake.arg1_history[0]);

	/* To finish we clear both gpios */
	zassert_equal(fixture->gpio_on.port, gpio_fake_port_clear_bits_raw_fake.arg0_history[1]);
	zassert_equal(1, gpio_fake_port_clear_bits_raw_fake.arg1_history[1]);
	zassert_equal(fixture->gpio_off.port, gpio_fake_port_clear_bits_raw_fake.arg0_history[2]);
	zassert_equal(2, gpio_fake_port_clear_bits_raw_fake.arg1_history[2]);
}

ZTEST_F(esphome_hbridge_tests, test_esphome_switch_hbridge_set_state_on)
{

	int ret;

	ret = esphome_switch_set_state(fixture->dev, 1);
	zassert_equal(ret, 0);

	/* First we set gpio on*/
	zassert_equal(fixture->gpio_off.port, gpio_fake_port_set_bits_raw_fake.arg0_history[0]);
	zassert_equal(1, gpio_fake_port_set_bits_raw_fake.arg1_history[0]);

	/* Then we clear gpio off */
	zassert_equal(fixture->gpio_on.port, gpio_fake_port_clear_bits_raw_fake.arg0_history[0]);
	zassert_equal(2, gpio_fake_port_clear_bits_raw_fake.arg1_history[0]);

	/* To finish we clear both gpios */
	zassert_equal(fixture->gpio_on.port, gpio_fake_port_clear_bits_raw_fake.arg0_history[1]);
	zassert_equal(1, gpio_fake_port_clear_bits_raw_fake.arg1_history[1]);
	zassert_equal(fixture->gpio_off.port, gpio_fake_port_clear_bits_raw_fake.arg0_history[2]);
	zassert_equal(2, gpio_fake_port_clear_bits_raw_fake.arg1_history[2]);
}

ZTEST_F(esphome_hbridge_tests, test_esphome_switch_hbridge_get_state)
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
