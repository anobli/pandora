/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#ifndef HERMES_LIGHT_COMMON_H
#define HERMES_LIGHT_COMMON_H

#include <zephyr/data/json.h>

#include <hermes/hermes.h>

struct json_light_state {
	bool state;
};

struct json_light_brightness {
	uint8_t brightness;
};

struct json_light_color {
	uint32_t color;
};

extern const struct json_obj_descr json_light_state_descr[];
extern const int json_light_state_descr_size;
extern const struct json_obj_descr json_light_brightness_descr[];
extern const int json_light_brightness_descr_size;
extern const struct json_obj_descr json_light_color_descr[];
extern const int json_light_color_descr_size;

#endif /* HERMES_LIGHT_COMMON_H */
