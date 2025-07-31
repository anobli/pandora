#ifndef NARADA_LIGHT_COMMON_H
#define NARADA_LIGHT_COMMON_H

#include <zephyr/data/json.h>

#include <narada/narada.h>

struct json_light_state {
	bool state;
};

struct json_light_brightness {
	uint8_t brightness;
};

extern const struct json_obj_descr json_light_state_descr[];
extern const int json_light_state_descr_size;
extern const struct json_obj_descr json_light_brightness_descr[];
extern const int json_light_brightness_descr_size;

#endif /* NARADA_LIGHT_COMMON_H */
