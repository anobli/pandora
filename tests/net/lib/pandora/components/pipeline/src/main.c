/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

#include <pandora/components/binary_sensor.h>
#include <pandora/components/switch.h>

struct pandora_pipeline_tests_fixture {
	const struct device *binary_sensor_dev;
	const struct device *switch_dev;
	const struct gpio_dt_spec gpio;
};

static struct pandora_pipeline_tests_fixture *g_fixture = NULL;
static void *pipeline_setup(void)
{
	static struct pandora_pipeline_tests_fixture fixture = {
		.binary_sensor_dev = DEVICE_DT_GET(DT_PATH(binary_sensor_0)),
		.switch_dev = DEVICE_DT_GET(DT_PATH(gpio_switch_0)),
		.gpio = GPIO_DT_SPEC_GET(DT_PATH(binary_sensor_0), gpios),
	};

	g_fixture = &fixture;

	return &fixture;
}

static void pipeline_before(void *f)
{
	struct pandora_pipeline_tests_fixture *fixture = f;

	gpio_emul_input_set_dt(&fixture->gpio, 0);
}

ZTEST_SUITE(pandora_pipeline_tests, NULL, pipeline_setup, pipeline_before, NULL, NULL);

ZTEST_F(pandora_pipeline_tests, test_pipeline_binary_sensor_on_state_to_switch_toggle)
{
	int ret;
	int state;
	int debounce_ms = DT_PROP(DT_PATH(binary_sensor_0), debounce_interval_ms) + 5;

	/* Test initial switch state */
	ret = pandora_switch_get_state(fixture->switch_dev, &state);
	zassert_equal(ret, 0);
	zassert_equal(state, 0);

	/* Set gpio and test if the pipeline handle it and toggle switch state */
	gpio_emul_input_set_dt(&fixture->gpio, 1);
	k_sleep(K_MSEC(debounce_ms)); // Give time to driver for debouncing

	/* Check binary_sensor state */
	ret = pandora_binary_sensor_get_state(fixture->binary_sensor_dev, &state);
	zassert_equal(ret, 0);
	zassert_equal(state, 1);

	/* And check that switch state has changed */
	ret = pandora_switch_get_state(fixture->switch_dev, &state);
	zassert_equal(ret, 0);
	zassert_equal(state, 1);

	/* Unset gpio and validate again pipeline is working */
	gpio_emul_input_set_dt(&fixture->gpio, 0);
	k_sleep(K_MSEC(debounce_ms)); // Give time to driver for debouncing

	/* Check that switch state has changed */
	ret = pandora_switch_get_state(fixture->switch_dev, &state);
	zassert_equal(ret, 0);
	zassert_equal(state, 0);
}

ZTEST_F(pandora_pipeline_tests, test_pipeline_binary_sensor_on_turn_on_to_switch_toggle)
{
	const struct device *switch_dev = DEVICE_DT_GET(DT_PATH(gpio_switch_1));
	const struct gpio_dt_spec gpio = GPIO_DT_SPEC_GET(DT_PATH(binary_sensor_1), gpios);
	int debounce_ms = DT_PROP(DT_PATH(binary_sensor_1), debounce_interval_ms) + 5;
	int state;

	/* Test initial switch state */
	gpio_emul_input_set_dt(&gpio, 0);
	pandora_switch_get_state(switch_dev, &state);
	zassert_equal(state, 0);

	/* Set gpio and test if the pipeline handle it and toggle switch state */
	gpio_emul_input_set_dt(&gpio, 1);
	k_sleep(K_MSEC(debounce_ms)); // Give time to driver for debouncing

	/* And check that switch state has changed */
	pandora_switch_get_state(switch_dev, &state);
	zassert_equal(state, 1);

	/* Unset gpio and validate that state have not changed */
	gpio_emul_input_set_dt(&gpio, 0);
	k_sleep(K_MSEC(debounce_ms)); // Give time to driver for debouncing
	pandora_switch_get_state(switch_dev, &state);
	zassert_equal(state, 1);

	/* Set gpio and test if the pipeline handle it and toggle switch state */
	gpio_emul_input_set_dt(&gpio, 1);
	k_sleep(K_MSEC(debounce_ms)); // Give time to driver for debouncing

	/* And check that switch state has changed */
	pandora_switch_get_state(switch_dev, &state);
	zassert_equal(state, 0);
}
