#ifndef NARADA_LIGHT_SERVER_H
#define NARADA_LIGHT_SERVER_H

#include <narada/narada.h>
#include <narada/light/common.h>

struct narada_light_api;

/*** Server ***/
struct narada_light_resource {
	struct narada_resource rsc;
	struct narada_light_api *api;
};

struct narada_light_api {
	int (*init)(struct narada_light_resource *sw);
	int (*set_state)(struct narada_light_resource *sw, int state);
	int (*get_state)(struct narada_light_resource *sw);
	int (*set_brightness)(struct narada_light_resource *sw, uint8_t brightness);
	int (*set_color)(struct narada_light_resource *sw, uint8_t r, uint8_t g, uint8_t b);
};

int narada_light_init(struct narada_resource *rsc);
void narada_light_handler_get_state(struct narada_resource *sw, void *data, uint16_t *len);
void narada_light_handler_put_state(struct narada_resource *sw, void *data, uint16_t len);
void narada_light_handler_get_brightness(struct narada_resource *sw, void *data, uint16_t *len);
void narada_light_handler_put_brightness(struct narada_resource *sw, void *data, uint16_t len);

#define DEFINE_NARADA_LIGHT_CMD(_inst, _api, _cmd, _init, _data)                                   \
	static struct narada_light_resource narada_rsc_##_cmd##_inst = {                          \
		.rsc = NARADA_RESOURCE_INIT(DT_INST_NARADA_RESOURCE_NAME(_inst),                   \
					    narada_light_init, narada_light_handler_get_##_cmd,    \
					    narada_light_handler_put_##_cmd, NULL, _data),         \
		.api = _api,                                                                       \
	};                                                                                         \
	NARADA_RESOURCE_DEFINE(light_resource_##_cmd##_inst, DT_INST_NARADA_RESOURCE_NAME(_inst), \
			       _cmd, narada_rsc_##_cmd##_inst);

#define DEFINE_NARADA_LIGHT(_inst, _api, _data)                                                    \
	DEFINE_NARADA_LIGHT_CMD(_inst, _api, state, narada_light_init, _data);                     \
	DEFINE_NARADA_LIGHT_CMD(_inst, _api, brightness, NULL, _data);

#endif /* NARADA_LIGHT_SERVER_H */
