/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#include <zephyr/device.h>
#include <zephyr/data/json.h>

#include <hermes/hermes.h>
#include <hermes/settings.h>
#include <zephyr/drivers/light.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(hermes, CONFIG_HERMES_LOG_LEVEL);

struct json_light_state {
	int state;
};

struct json_light_brightness {
	int brightness;
};

struct json_light_temperature {
	int temperature;
};

struct json_light_color {
	int color;
};

const struct json_obj_descr json_light_state_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_light_state, state, JSON_TOK_NUMBER),
};
const int json_light_state_descr_size = ARRAY_SIZE(json_light_state_descr);

const struct json_obj_descr json_light_brightness_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_light_brightness, brightness, JSON_TOK_NUMBER),
};
const int json_light_brightness_descr_size = ARRAY_SIZE(json_light_brightness_descr);

const struct json_obj_descr json_light_temperature_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_light_temperature, temperature, JSON_TOK_NUMBER),
};
const int json_light_temperature_descr_size = ARRAY_SIZE(json_light_temperature_descr);

const struct json_obj_descr json_light_color_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_light_color, color, JSON_TOK_NUMBER),
};
const int json_light_color_descr_size = ARRAY_SIZE(json_light_color_descr);

void hermes_light_handler_get_state(struct hermes_resource *rsc, void *data, uint16_t *len)
{
	struct json_light_state light_data;
	uint8_t state;

	pandora_light_get_state(rsc->dev, &state);
	light_data.state = state;
	json_obj_encode_buf(json_light_state_descr, json_light_state_descr_size, &light_data, data,
			    *len);
	*len = strlen(data);
}

void hermes_light_handler_put_state(struct hermes_resource *rsc, const void *data, uint16_t len)
{
	struct json_light_state light_data;
	int ret;

	json_obj_parse((void *)data, len, json_light_state_descr, json_light_state_descr_size,
		       &light_data);

	ret = pandora_light_set_state(rsc->dev, light_data.state > 0);
	if (ret) {
		LOG_ERR("Failed to set light state: %d", ret);
	} else {
		hermes_settings_save_all();
	}
}

void hermes_light_handler_get_brightness(struct hermes_resource *rsc, void *data, uint16_t *len)
{
	struct json_light_brightness light_data;
	uint8_t brightness;

	pandora_light_get_brightness(rsc->dev, &brightness);
	light_data.brightness = brightness;
	json_obj_encode_buf(json_light_brightness_descr, json_light_brightness_descr_size,
			    &light_data, data, *len);
	*len = strlen(data);
}

void hermes_light_handler_put_brightness(struct hermes_resource *rsc, const void *data,
					 uint16_t len)
{
	struct json_light_brightness light_data;
	int ret;

	json_obj_parse((void *)data, len, json_light_brightness_descr,
		       json_light_brightness_descr_size, &light_data);

	ret = pandora_light_set_brightness(rsc->dev, light_data.brightness);
	if (ret) {
		LOG_ERR("Failed to set light brightness: %d", ret);
	} else {
		hermes_settings_save_all();
	}
}

void hermes_light_handler_get_temperature(struct hermes_resource *rsc, void *data, uint16_t *len)
{
	struct json_light_temperature light_data;
	uint8_t temperature;

	pandora_light_get_temperature(rsc->dev, &temperature);
	light_data.temperature = temperature;
	json_obj_encode_buf(json_light_temperature_descr, json_light_temperature_descr_size,
			    &light_data, data, *len);
	*len = strlen(data);
}

void hermes_light_handler_put_temperature(struct hermes_resource *rsc, const void *data,
					  uint16_t len)
{
	struct json_light_temperature light_data;
	int ret;

	json_obj_parse((void *)data, len, json_light_temperature_descr,
		       json_light_temperature_descr_size, &light_data);

	ret = pandora_light_set_temperature(rsc->dev, light_data.temperature);
	if (ret) {
		LOG_ERR("Failed to set light temperature: %d", ret);
	} else {
		hermes_settings_save_all();
	}
}

void hermes_light_handler_get_color(struct hermes_resource *rsc, void *data, uint16_t *len)
{
	struct json_light_color light_data;
	uint32_t color;

	pandora_light_get_color(rsc->dev, &color);
	light_data.color = (int)color;
	json_obj_encode_buf(json_light_color_descr, json_light_color_descr_size, &light_data, data,
			    *len);
	*len = strlen(data);
}

void hermes_light_handler_put_color(struct hermes_resource *rsc, const void *data, uint16_t len)
{
	struct json_light_color light_data;
	int ret;

	json_obj_parse((void *)data, len, json_light_color_descr, json_light_color_descr_size,
		       &light_data);

	ret = pandora_light_set_color(rsc->dev, light_data.color);
	if (ret) {
		LOG_ERR("Failed to set light color: %d", ret);
	} else {
		hermes_settings_save_all();
	}
}

int hermes_light_settings_load(const struct device *dev)
{
	struct pandora_light_data *data = pandora_light_get_data(dev);

	hermes_settings_load_one(dev->name, "state", &data->state, sizeof(data->state));
	hermes_settings_load_one(dev->name, "brightness", &data->brightness,
				 sizeof(data->brightness));
	hermes_settings_load_one(dev->name, "temperature", &data->temperature,
				 sizeof(data->temperature));
	hermes_settings_load_one(dev->name, "color", &data->color, sizeof(data->color));

	pandora_light_update(dev);

	return 0;
}

int hermes_light_settings_save(const struct device *dev)
{
	struct pandora_light_data *data = pandora_light_get_data(dev);

	hermes_settings_save_one(dev->name, "state", &data->state, sizeof(data->state));
	hermes_settings_save_one(dev->name, "brightness", &data->brightness,
				 sizeof(data->brightness));
	hermes_settings_save_one(dev->name, "temperature", &data->temperature,
				 sizeof(data->temperature));
	hermes_settings_save_one(dev->name, "color", &data->color, sizeof(data->color));

	return 0;
}

#define DEFINE_HERMES_LIGHT_EP(node_id, _ep)                                                       \
	DT_HERMES_RESOURCE_DEFINE(node_id, _ep, hermes_light_handler_get_##_ep,                    \
				  hermes_light_handler_put_##_ep, NULL);

#define DEFINE_HERMES_LIGHT(node_id)                                                               \
	DT_HERMES_SETTINGS(node_id, hermes_light_settings_load, hermes_light_settings_save, NULL); \
	DEFINE_HERMES_LIGHT_EP(node_id, state);                                                    \
	DEFINE_HERMES_LIGHT_EP(node_id, brightness);                                               \
	DEFINE_HERMES_LIGHT_EP(node_id, temperature);                                              \
	DEFINE_HERMES_LIGHT_EP(node_id, color);

DT_FOREACH_STATUS_OKAY(pandora_light_rgbct, DEFINE_HERMES_LIGHT)
