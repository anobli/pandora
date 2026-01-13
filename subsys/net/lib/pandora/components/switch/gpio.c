/*
 * Copyright (c) 2026 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pandora_switch_gpio

#include <stdlib.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <pandora/components/switch.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(Pandora, CONFIG_PANDORA_LOG_LEVEL);

struct pandora_gpio_switch_config {
	const struct gpio_dt_spec gpio;
};

struct pandora_gpio_switch_data {
	int state;
};

static int pandora_gpio_switch_init(const struct device *dev)
{
	const struct pandora_gpio_switch_config *config = dev->config;
	struct pandora_gpio_switch_data *data = dev->data;
	int ret;

	ret = gpio_pin_configure_dt(&config->gpio, GPIO_OUTPUT_LOW);
	if (ret) {
		return ret;
	}

	data->state = -EINVAL;

	return 0;
}

static int pandora_gpio_switch_set_state(const struct device *dev, int state)
{
	const struct pandora_gpio_switch_config *config = dev->config;
	struct pandora_gpio_switch_data *data = dev->data;
	int ret;

	ret = gpio_pin_set_dt(&config->gpio, state);
	if (ret < 0) {
		LOG_ERR("Failed to set gpio state");
		return ret;
	}

	data->state = state;

	return 0;
}

static int pandora_gpio_switch_get_state(const struct device *dev)
{
	struct pandora_gpio_switch_data *data = dev->data;

	return data->state;
}

static struct pandora_switch_component_api gpio_switch = {
	.set_state = pandora_gpio_switch_set_state,
	.get_state = pandora_gpio_switch_get_state,
};

#define DEFINE_PANDORA_SWITCH_GPIO(_num)                                                           \
                                                                                                   \
	static const struct pandora_gpio_switch_config pandora_gpio_switch_config_##_num = {       \
		.gpio = GPIO_DT_SPEC_GET(DT_DRV_INST(_num), gpios),                                \
	};                                                                                         \
	static struct pandora_gpio_switch_data pandora_gpio_switch_data_##_num;                    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_num, pandora_gpio_switch_init, NULL,                                \
			      &pandora_gpio_switch_data_##_num,                                    \
			      &pandora_gpio_switch_config_##_num, POST_KERNEL,                     \
			      CONFIG_PANDORA_COMPONENT_INIT_PRIORITY, &gpio_switch);               \
	DT_INST_DEFINE_PANDORA_SWITCH_COMPONENT(_num);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_PANDORA_SWITCH_GPIO);
