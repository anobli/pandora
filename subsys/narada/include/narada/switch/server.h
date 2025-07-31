#ifndef NARADA_SWITCH_SERVER_H
#define NARADA_SWITCH_SERVER_H

#include <narada/narada.h>
#include <narada/switch/common.h>

struct narada_switch_api;

/*** Server ***/
struct narada_switch_resource {
	struct narada_resource rsc;
	struct narada_switch_api *api;
};

struct narada_switch_api {
	int (*init)(struct narada_switch_resource *sw);
	int (*set_state)(struct narada_switch_resource *sw, int state);
	int (*get_state)(struct narada_switch_resource *sw);
};

int narada_switch_init(struct narada_resource *rsc);
void narada_switch_handler_get(struct narada_resource *sw, void *data, uint16_t *len);
void narada_switch_handler_put(struct narada_resource *sw, void *data, uint16_t len);

#define DEFINE_NARADA_SWITCH(_inst, _id, _api, _data)                                              \
	static struct narada_switch_resource narada_rsc_##_inst = {                                \
		.rsc = NARADA_RESOURCE_INIT(_id, narada_switch_init, narada_switch_handler_get,    \
					    narada_switch_handler_put, NULL, _data),               \
		.api = _api,                                                                       \
	};                                                                                         \
	NARADA_RESOURCE_DEFINE(switch_resource_##_inst, SWITCH_BASE_URI, _inst, narada_rsc_##_inst);
#endif /* NARADA_SWITCH_SERVER */
