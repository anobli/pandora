/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nabucasa_esphome_switch_gpio

#include <stdlib.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <esphome/components/api.h>
#include <esphome/components/switch.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ESPHome, CONFIG_ESPHOME_LOG_LEVEL);

struct esphome_gpio_switch_config {
	const struct gpio_dt_spec gpio;
};

struct esphome_gpio_switch_data {
	int state;
};

static int esphome_gpio_switch_init(const struct device *dev)
{
	const struct esphome_gpio_switch_config *config = dev->config;
	struct esphome_gpio_switch_data *data = dev->data;
	int ret;

	ret = gpio_pin_configure_dt(&config->gpio, GPIO_OUTPUT_LOW);
	if (ret) {
		return ret;
	}

	data->state = -EINVAL;

	return 0;
}

static int esphome_gpio_switch_set_state(const struct device *dev, int state)
{
	const struct esphome_gpio_switch_config *config = dev->config;
	struct esphome_gpio_switch_data *data = dev->data;
	int ret;

	ret = gpio_pin_set_dt(&config->gpio, state);
	if (ret < 0) {
		LOG_ERR("Failed to set gpio state");
		return ret;
	}

	data->state = state;

	return 0;
}

static int esphome_gpio_switch_get_state(const struct device *dev)
{
	struct esphome_gpio_switch_data *data = dev->data;

	return data->state;
}

static struct esphome_switch_component_api gpio_switch = {
	.set_state = esphome_gpio_switch_set_state,
	.get_state = esphome_gpio_switch_get_state,
};

#define DEFINE_ESPHOME_SWITCH_GPIO(_num)                                                           \
                                                                                                   \
	static const struct esphome_gpio_switch_config esphome_gpio_switch_config_##_num = {       \
		.gpio = GPIO_DT_SPEC_GET(DT_DRV_INST(_num), gpios),                                \
	};                                                                                         \
	static struct esphome_gpio_switch_data esphome_gpio_switch_data_##_num;                    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_num, esphome_gpio_switch_init, NULL,                                \
			      &esphome_gpio_switch_data_##_num,                                    \
			      &esphome_gpio_switch_config_##_num, POST_KERNEL,                     \
			      CONFIG_ESPHOME_INIT_PRIORITY, &gpio_switch);                         \
	DEFINE_ESPHOME_ENTITY(_num, esphome_gpio_switch_##_num, "switch.gpio",                     \
			      esphome_switch_list_entity);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_ESPHOME_SWITCH_GPIO);
