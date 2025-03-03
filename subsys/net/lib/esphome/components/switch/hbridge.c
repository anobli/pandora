/*
 * Copyright (c) 2025 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nabucasa_esphome_switch_hbridge

#include <stdlib.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <esphome/components/api.h>
#include <esphome/components/entity.h>
#include <esphome/components/switch.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ESPHome, CONFIG_ESPHOME_LOG_LEVEL);

struct esphome_switch_hbridge_config {
	const struct gpio_dt_spec on_pin;
	const struct gpio_dt_spec off_pin;
	int wait_time;
};

struct esphome_switch_hbridge_data {
	int state;
};

static int esphome_switch_hbridge_init(const struct device *dev)
{
	const struct esphome_switch_hbridge_config *config = dev->config;
	struct esphome_switch_hbridge_data *data = dev->data;
	int ret;

	ret = gpio_pin_configure_dt(&config->on_pin, GPIO_OUTPUT_LOW);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->off_pin, GPIO_OUTPUT_LOW);
	if (ret) {
		return ret;
	}

	data->state = -EINVAL;

	return 0;
}

static int esphome_switch_hbridge_gpios(const struct device *dev, int on_pin, int off_pin)
{
	const struct esphome_switch_hbridge_config *config = dev->config;
	int ret;

	ret = gpio_pin_set_dt(&config->on_pin, on_pin);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_set_dt(&config->off_pin, off_pin);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int esphome_switch_hbridge_set_state(const struct device *dev, int state)
{
	const struct esphome_switch_hbridge_config *config = dev->config;
	struct esphome_switch_hbridge_data *data = dev->data;
	int ret;

	ret = esphome_switch_hbridge_gpios(dev, state, !state);
	if (ret < 0) {
		LOG_ERR("Failed to set hbridge state");
		return ret;
	}

	k_sleep(K_MSEC(config->wait_time));

	ret = esphome_switch_hbridge_gpios(dev, 0, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set hbridge state");
		return ret;
	}

	data->state = state;

	return 0;
}

static int esphome_switch_hbridge_get_state(const struct device *dev)
{
	const struct esphome_switch_hbridge_data *data = dev->data;

	return data->state;
}

static struct esphome_switch_component_api hbridge_switch = {
	.set_state = esphome_switch_hbridge_set_state,
	.get_state = esphome_switch_hbridge_get_state,
};

#define DEFINE_ESPHOME_SWITCH_HBRIDGE(_num)                                                        \
                                                                                                   \
	static const struct esphome_switch_hbridge_config esphome_switch_hbridge_config_##_num = { \
		.on_pin = GPIO_DT_SPEC_GET(DT_DRV_INST(_num), on_gpios),                           \
		.off_pin = GPIO_DT_SPEC_GET(DT_DRV_INST(_num), off_gpios),                         \
		.wait_time = DT_INST_PROP(_num, wait_time),                                        \
	};                                                                                         \
	static struct esphome_switch_hbridge_data esphome_switch_hbridge_data_##_num;              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_num, esphome_switch_hbridge_init, NULL,                             \
			      &esphome_switch_hbridge_data_##_num,                                 \
			      &esphome_switch_hbridge_config_##_num, POST_KERNEL,                  \
			      CONFIG_ESPHOME_INIT_PRIORITY, &hbridge_switch);                      \
	DEFINE_ESPHOME_ENTITY(_num, esphome_switch_hbridge_##_num, "switch.hbridge",               \
			      esphome_switch_list_entity);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_ESPHOME_SWITCH_HBRIDGE);
