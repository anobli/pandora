#include <narada/light/common.h>

const struct json_obj_descr json_light_state_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_light_state, state, JSON_TOK_TRUE),
};
const int json_light_state_descr_size = ARRAY_SIZE(json_light_state_descr);

const struct json_obj_descr json_light_brightness_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_light_brightness, brightness, JSON_TOK_TRUE),
};
const int json_light_brightness_descr_size = ARRAY_SIZE(json_light_brightness_descr);
