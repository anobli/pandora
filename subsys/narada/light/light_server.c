#include <narada/narada.h>
#include <narada/light/server.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(narada, CONFIG_NARADA_LOG_LEVEL);

int narada_light_init(struct narada_resource *rsc)
{
	struct narada_light_resource *swtich_rsc =
		CONTAINER_OF(rsc, struct narada_light_resource, rsc);
	int ret;

	if (!swtich_rsc->api || !swtich_rsc->api->init || !swtich_rsc->api->set_state ||
	    !swtich_rsc->api->get_state) {
		return -ENOSYS;
	}

	ret = swtich_rsc->api->init(swtich_rsc);
	if (ret) {
		return ret;
	}

	return 0;
}

void narada_light_handler_get_state(struct narada_resource *rsc, void *data, uint16_t *len)
{
	struct narada_light_resource *sw = CONTAINER_OF(rsc, struct narada_light_resource, rsc);
	struct json_light_state light_data;

	light_data.state = sw->api->get_state(sw);

	json_obj_encode_buf(json_light_state_descr, json_light_state_descr_size, &light_data, data,
			    *len);
	*len = strlen(data);
}

void narada_light_handler_put_state(struct narada_resource *rsc, void *data, uint16_t len)
{

	struct json_light_state light_data;
	struct narada_light_resource *sw = CONTAINER_OF(rsc, struct narada_light_resource, rsc);
	int ret;

	json_obj_parse((void *)data, len, json_light_state_descr, json_light_state_descr_size,
		       &light_data);

	if(light_data.state) {
		ret = sw->api->set_state(sw, 1);
	} else {
		ret = sw->api->set_state(sw, 0);
	}

	if (ret) {
		LOG_ERR("Failed to set light state: %d", ret);
	}
}

void narada_light_handler_get_brightness(struct narada_resource *sw, void *data, uint16_t *len)
{

}
void narada_light_handler_put_brightness(struct narada_resource *sw, void *data, uint16_t len)
{

}
