/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/drivers/light.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(hermes, CONFIG_HERMES_LOG_LEVEL);

#define DT_DRV_COMPAT pandora_light_rgbct

#define MAX_WHITE_BRIGHTNESS 100

#define RED_COLOR(color)   ((color >> 0) & 0xFF)
#define GREEN_COLOR(color) ((color >> 8) & 0xFF)
#define BLUE_COLOR(color)  ((color >> 16) & 0xFF)
#define RGB_WHITE_COLOR    0x00FFFFFF
#define RGB_MAX_PERIOD     255

struct rgbct_light_config {
	const struct pwm_dt_spec white_brightness;
	const struct pwm_dt_spec color_temperature;
	const struct pwm_dt_spec red;
	const struct pwm_dt_spec green;
	const struct pwm_dt_spec blue;
};

struct rgbct_light_data {
	struct pandora_light_data light;
};

static int rgbct_light_update(const struct device *dev);

static int rgbct_light_init(const struct device *dev)
{
	const struct rgbct_light_config *config = dev->config;

	if (!pwm_is_ready_dt(&config->white_brightness) ||
	    !pwm_is_ready_dt(&config->color_temperature)) {
		return -ENODEV;
	}

	if (!pwm_is_ready_dt(&config->red) || !pwm_is_ready_dt(&config->green) ||
	    !pwm_is_ready_dt(&config->blue)) {
		return -ENODEV;
	}

	return rgbct_light_update(dev);
}

static int rgbct_light_set(const struct rgbct_light_config *config, uint8_t white_brightness,
			   uint8_t color_temperature, int color)
{
	if (!color || color == RGB_WHITE_COLOR) {
		pwm_set_dt(&config->red, RGB_MAX_PERIOD, 0);
		pwm_set_dt(&config->green, RGB_MAX_PERIOD, 0);
		pwm_set_dt(&config->blue, RGB_MAX_PERIOD, 0);

		if (white_brightness > MAX_WHITE_BRIGHTNESS) {
			return -EINVAL;
		}

		pwm_set_dt(&config->white_brightness, MAX_WHITE_BRIGHTNESS, white_brightness);
		/* Not sure about that one */
		pwm_set_dt(&config->color_temperature, MAX_WHITE_BRIGHTNESS, color_temperature);

		return 0;
	} else {
		pwm_set_dt(&config->white_brightness, MAX_WHITE_BRIGHTNESS, 0);
		pwm_set_dt(&config->color_temperature, MAX_WHITE_BRIGHTNESS, 0);

		pwm_set_dt(&config->red, RGB_MAX_PERIOD, RED_COLOR(color));
		pwm_set_dt(&config->green, RGB_MAX_PERIOD, GREEN_COLOR(color));
		pwm_set_dt(&config->blue, RGB_MAX_PERIOD, BLUE_COLOR(color));

		return 0;
	}
}

static int rgbct_light_update(const struct device *dev)
{
	struct rgbct_light_data *data = dev->data;
	struct pandora_light_data *light_data = &data->light;
	const struct rgbct_light_config *config = dev->config;

	if (light_data->state) {
		return rgbct_light_set(config, light_data->brightness, light_data->temperature,
				       light_data->color);
	} else {
		return rgbct_light_set(config, 0, 0, 0);
	}
}

struct pandora_light_data *rgbct_light_get_data(const struct device *dev)
{
	struct rgbct_light_data *data = dev->data;
	struct pandora_light_data *light_data = &data->light;

	return light_data;
}

DEVICE_API(pandora_light, rgbct_light) = {
	.update = rgbct_light_update,
	.get_light_data = rgbct_light_get_data,
};

#define HERMES_RGBCT_LIGHT(inst)                                                                   \
	static struct rgbct_light_data rgbct_light_data_##inst;                                    \
	static const struct rgbct_light_config rgbct_light_config_##inst = {                       \
		.white_brightness = PWM_DT_SPEC_INST_GET_BY_NAME(inst, white_brightness),          \
		.color_temperature = PWM_DT_SPEC_INST_GET_BY_NAME(inst, color_temperature),        \
		.red = PWM_DT_SPEC_INST_GET_BY_NAME(inst, red),                                    \
		.green = PWM_DT_SPEC_INST_GET_BY_NAME(inst, green),                                \
		.blue = PWM_DT_SPEC_INST_GET_BY_NAME(inst, blue),                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, rgbct_light_init, NULL, &rgbct_light_data_##inst,              \
			      &rgbct_light_config_##inst, POST_KERNEL,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &rgbct_light);

DT_INST_FOREACH_STATUS_OKAY(HERMES_RGBCT_LIGHT)
