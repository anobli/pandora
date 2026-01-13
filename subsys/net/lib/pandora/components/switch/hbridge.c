/*
 * Copyright (c) 2026 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pandora_switch_hbridge

#include <stdlib.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <pandora/components/switch.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(Pandora, CONFIG_PANDORA_LOG_LEVEL);

struct pandora_switch_hbridge_config {
	const struct gpio_dt_spec on_pin;
	const struct gpio_dt_spec off_pin;
	int wait_time;
};

struct pandora_switch_hbridge_data {
	int state;
};

static int pandora_switch_hbridge_init(const struct device *dev)
{
	const struct pandora_switch_hbridge_config *config = dev->config;
	struct pandora_switch_hbridge_data *data = dev->data;
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

static int pandora_switch_hbridge_gpios(const struct device *dev, int on_pin, int off_pin)
{
	const struct pandora_switch_hbridge_config *config = dev->config;
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

static int pandora_switch_hbridge_set_state(const struct device *dev, int state)
{
	const struct pandora_switch_hbridge_config *config = dev->config;
	struct pandora_switch_hbridge_data *data = dev->data;
	int ret;

	ret = pandora_switch_hbridge_gpios(dev, state, !state);
	if (ret < 0) {
		LOG_ERR("Failed to set hbridge state");
		return ret;
	}

	k_sleep(K_MSEC(config->wait_time));

	ret = pandora_switch_hbridge_gpios(dev, 0, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set hbridge state");
		return ret;
	}

	data->state = state;

	return 0;
}

static int pandora_switch_hbridge_get_state(const struct device *dev)
{
	const struct pandora_switch_hbridge_data *data = dev->data;

	return data->state;
}

static struct pandora_switch_component_api hbridge_switch = {
	.set_state = pandora_switch_hbridge_set_state,
	.get_state = pandora_switch_hbridge_get_state,
};

#define DEFINE_PANDORA_SWITCH_HBRIDGE(_num)                                                        \
                                                                                                   \
	static const struct pandora_switch_hbridge_config pandora_switch_hbridge_config_##_num = { \
		.on_pin = GPIO_DT_SPEC_GET(DT_DRV_INST(_num), on_gpios),                           \
		.off_pin = GPIO_DT_SPEC_GET(DT_DRV_INST(_num), off_gpios),                         \
		.wait_time = DT_INST_PROP(_num, wait_time),                                        \
	};                                                                                         \
	static struct pandora_switch_hbridge_data pandora_switch_hbridge_data_##_num;              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_num, pandora_switch_hbridge_init, NULL,                             \
			      &pandora_switch_hbridge_data_##_num,                                 \
			      &pandora_switch_hbridge_config_##_num, POST_KERNEL,                  \
			      CONFIG_PANDORA_COMPONENT_INIT_PRIORITY, &hbridge_switch);            \
	DT_INST_DEFINE_PANDORA_SWITCH_COMPONENT(_num);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_PANDORA_SWITCH_HBRIDGE);
