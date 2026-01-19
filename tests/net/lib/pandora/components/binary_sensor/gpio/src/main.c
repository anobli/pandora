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

struct pandora_binary_sensor_gpio_tests_fixture {
	const struct device *dev;
	const struct gpio_dt_spec gpio;

	int state;
	struct k_sem sem;
};

static struct pandora_binary_sensor_gpio_tests_fixture *g_fixture = NULL;
static void *binary_sensor_gpio_setup(void)
{
	static struct pandora_binary_sensor_gpio_tests_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_PATH(binary_sensor)),
		.gpio = GPIO_DT_SPEC_GET(DT_PATH(binary_sensor), gpios),
	};

	k_sem_init(&fixture.sem, 0, 1);
	g_fixture = &fixture;

	return &fixture;
}

static void binary_sensor_gpio_before(void *f)
{
	struct pandora_binary_sensor_gpio_tests_fixture *fixture = f;

	gpio_emul_input_set_dt(&fixture->gpio, 0);
	k_sem_reset(&g_fixture->sem);
}

ZTEST_SUITE(pandora_binary_sensor_gpio_tests, NULL, binary_sensor_gpio_setup,
	    binary_sensor_gpio_before, NULL, NULL);

PANDORA_CB(binary_sensor, on_state, on_state_test, DEVICE_DT_GET(DT_PATH(binary_sensor)))
{
	g_fixture->state = state;
	k_sem_give(&g_fixture->sem);
}

ZTEST_F(pandora_binary_sensor_gpio_tests, test_pandora_binary_sensor_gpio_get_state)
{
	int state;
	int ret;

	gpio_emul_input_set_dt(&fixture->gpio, 0);
	ret = pandora_binary_sensor_get_state(fixture->dev, &state);
	zassert_equal(ret, 0);
	zassert_equal(state, 0);

	gpio_emul_input_set_dt(&fixture->gpio, 1);
	ret = pandora_binary_sensor_get_state(fixture->dev, &state);
	zassert_equal(ret, 0);
	zassert_equal(state, 1);
}

ZTEST_F(pandora_binary_sensor_gpio_tests, test_pandora_binary_sensor_on_state_cb)
{
	int ret;

	/* Enable on_state callback */
	pandora_cb_enable(binary_sensor, on_state, &on_state_test);

	/* Test callback when state change: 0 -> 1 */
	gpio_emul_input_set_dt(&fixture->gpio, 1);
	ret = k_sem_take(&fixture->sem, K_MSEC(100));
	zassert_equal(ret, 0);
	zassert_equal(fixture->state, 1);

	/* Test callback when state change: 1 -> 0 */
	gpio_emul_input_set_dt(&fixture->gpio, 0);
	ret = k_sem_take(&fixture->sem, K_MSEC(100));
	zassert_equal(ret, 0);
	zassert_equal(fixture->state, 0);

	/* Disable on_state callback */
	pandora_cb_disable(binary_sensor, on_state, &on_state_test);

	/* Check that the callback is invoked anymore */
	gpio_emul_input_set_dt(&fixture->gpio, 1);
	ret = k_sem_take(&fixture->sem, K_MSEC(100));
	zassert_equal(ret, -EAGAIN);
}
