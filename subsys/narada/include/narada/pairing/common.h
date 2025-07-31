#ifndef NARADA_PAIRING_COMMON_H
#define NARADA_PAIRING_COMMON_H

#include <zephyr/data/json.h>

#include <narada/narada.h>

#define PAIRING_BASE_URI "pairing"

struct pairing_request {
	int device_type;
};

struct pairing_request_done {
	const char *ip;
};

extern const struct json_obj_descr json_pairing_request_descr[];
extern const int json_pairing_request_descr_size;

extern const struct json_obj_descr json_pairing_done_descr[];
extern const int json_pairing_done_descr_size;

#endif /* NARADA_PAIRING_COMMON_H */
