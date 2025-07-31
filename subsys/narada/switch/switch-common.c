#include <narada/switch/common.h>

const struct json_obj_descr json_switch_state_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_switch_state, state, JSON_TOK_NUMBER),
};
const int json_switch_state_descr_size = ARRAY_SIZE(json_switch_state_descr);
