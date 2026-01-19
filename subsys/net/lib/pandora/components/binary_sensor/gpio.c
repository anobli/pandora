/*
 * Copyright (c) 2026 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pandora_binary_sensor_gpio

#include <stdlib.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <pandora/components/binary_sensor.h>
#include "binary_sensor_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(Pandora, CONFIG_PANDORA_LOG_LEVEL);

struct pandora_binary_sensor_gpio_config {
	const struct gpio_dt_spec gpio;
	const int debounce_ms;
};

struct pandora_binary_sensor_gpio_data {
	const struct device *dev;
	struct gpio_callback callback;
	struct k_work_delayable dwork;
	int state;
};

static void debounce_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct pandora_binary_sensor_gpio_data *data =
		CONTAINER_OF(dwork, struct pandora_binary_sensor_gpio_data, dwork);
	const struct pandora_binary_sensor_gpio_config *config = data->dev->config;
	int state;

	state = gpio_pin_get_dt(&config->gpio);
	if (state != data->state) {
		data->state = state;
		pandora_binary_sensor_update_state(data->dev, state);
	}
}

static void pandora_binary_sensor_gpio_callback(const struct device *dev, struct gpio_callback *cb,
						uint32_t pins)
{
	struct pandora_binary_sensor_gpio_data *data =
		CONTAINER_OF(cb, struct pandora_binary_sensor_gpio_data, callback);
	const struct pandora_binary_sensor_gpio_config *config = data->dev->config;

	k_work_reschedule(&data->dwork, K_MSEC(config->debounce_ms));
}

static int pandora_binary_sensor_gpio_get_state(const struct device *dev)
{
	const struct pandora_binary_sensor_gpio_config *config = dev->config;
	struct pandora_binary_sensor_gpio_data *data = dev->data;

	data->state = gpio_pin_get_dt(&config->gpio);
	return data->state;
}

static int pandora_binary_sensor_gpio_init(const struct device *dev)
{
	const struct pandora_binary_sensor_gpio_config *config = dev->config;
	struct pandora_binary_sensor_gpio_data *data = dev->data;
	int ret;

	k_work_init_delayable(&data->dwork, debounce_handler);

	ret = gpio_pin_configure_dt(&config->gpio, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	data->state = gpio_pin_get_dt(&config->gpio);
	if (data->state < 0) {
		LOG_ERR("Failed to get initial state");
		return data->state;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->gpio, GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		LOG_ERR("Failed to configure interrupt");
		return 0;
	}

	gpio_init_callback(&data->callback, pandora_binary_sensor_gpio_callback,
			   BIT(config->gpio.pin));
	ret = gpio_add_callback_dt(&config->gpio, &data->callback);
	if (ret != 0) {
		LOG_ERR("Failed to set gpio callback!");
		return ret;
	}

	return 0;
}

static struct pandora_component_binary_sensor_api binary_sensor_gpio = {
	.get_state = pandora_binary_sensor_gpio_get_state,
};

#define DEFINE_PANDORA_BINARY_SENSOR_GPIO(_num)                                                    \
                                                                                                   \
	static const struct pandora_binary_sensor_gpio_config                                      \
		pandora_binary_sensor_gpio_config_##_num = {                                       \
			.gpio = GPIO_DT_SPEC_GET(DT_DRV_INST(_num), gpios),                        \
			.debounce_ms = DT_INST_PROP(_num, debounce_interval_ms),                   \
	};                                                                                         \
	static struct pandora_binary_sensor_gpio_data pandora_binary_sensor_gpio_data_##_num = {   \
		.dev = DEVICE_DT_GET(DT_DRV_INST(_num)),                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_num, pandora_binary_sensor_gpio_init, NULL,                         \
			      &pandora_binary_sensor_gpio_data_##_num,                             \
			      &pandora_binary_sensor_gpio_config_##_num, POST_KERNEL,              \
			      CONFIG_PANDORA_COMPONENT_INIT_PRIORITY, &binary_sensor_gpio);        \
	DT_INST_DEFINE_PANDORA_COMPONENT_BINARY_SENSOR(_num);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_PANDORA_BINARY_SENSOR_GPIO);
