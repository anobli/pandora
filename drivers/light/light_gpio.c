/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/drivers/light.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(light_gpio, CONFIG_LIGHT_LOG_LEVEL);

#define DT_DRV_COMPAT pandora_light_gpio

struct gpio_light_config {
	struct gpio_dt_spec gpio;
};

struct gpio_light_data {
	struct pandora_light_data light;
};

static int gpio_light_update(const struct device *dev)
{
	struct gpio_light_data *data = dev->data;
	struct pandora_light_data *light_data = &data->light;
	const struct gpio_light_config *config = dev->config;

	/* TODO: return error if we set anything else than state */
	return gpio_pin_set_dt(&config->gpio, light_data->state);
}

struct pandora_light_data *gpio_light_get_data(const struct device *dev)
{
	struct gpio_light_data *data = dev->data;
	struct pandora_light_data *light_data = &data->light;

	return light_data;
}

static int gpio_light_init(const struct device *dev)
{
	const struct gpio_light_config *config = dev->config;
	int err;

	if (device_is_ready(config->gpio.port)) {
		err = gpio_pin_configure_dt(&config->gpio, GPIO_OUTPUT_INACTIVE);

		if (err) {
			LOG_ERR("Cannot configure GPIO (err %d)", err);
		}
	} else {
		LOG_ERR("%s: GPIO device not ready", dev->name);
		err = -ENODEV;
	}

	return 0;
}

DEVICE_API(pandora_light, gpio_light) = {
	.update = gpio_light_update,
	.get_light_data = gpio_light_get_data,
};

#define GPIO_LIGHT(inst)                                                                           \
	static struct gpio_light_data gpio_light_data_##inst;                                      \
	static const struct gpio_light_config gpio_light_config_##inst = {                         \
		.gpio = GPIO_DT_SPEC_INST_GET(inst, gpios),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, gpio_light_init, NULL, &gpio_light_data_##inst,                \
			      &gpio_light_config_##inst, POST_KERNEL,                              \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_light);

DT_INST_FOREACH_STATUS_OKAY(GPIO_LIGHT)
