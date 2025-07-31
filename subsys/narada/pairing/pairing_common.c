#include <narada/pairing/common.h>

const struct json_obj_descr json_pairing_request_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct pairing_request, device_type, JSON_TOK_NUMBER),
};
const int json_pairing_request_descr_size = ARRAY_SIZE(json_pairing_request_descr);

const struct json_obj_descr json_pairing_done_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct pairing_request_done, ip, JSON_TOK_STRING),
};
const int json_pairing_done_descr_size = ARRAY_SIZE(json_pairing_done_descr);
