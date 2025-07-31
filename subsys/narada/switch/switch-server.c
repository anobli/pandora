#include <narada/narada.h>
#include <narada/switch/server.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(narada, CONFIG_NARADA_LOG_LEVEL);

int narada_switch_init(struct narada_resource *rsc)
{
	struct narada_switch_resource *swtich_rsc =
		CONTAINER_OF(rsc, struct narada_switch_resource, rsc);
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

void narada_switch_handler_get(struct narada_resource *rsc, void *data, uint16_t *len)
{
	struct narada_switch_resource *sw = CONTAINER_OF(rsc, struct narada_switch_resource, rsc);
	struct json_switch_state switch_data;

	switch_data.state = sw->api->get_state(sw);

	json_obj_encode_buf(json_switch_state_descr, json_switch_state_descr_size, &switch_data,
			    data, *len);
	*len = strlen(data);
}

void narada_switch_handler_put(struct narada_resource *rsc, void *data, uint16_t len)
{

	struct json_switch_state switch_data;
	struct narada_switch_resource *sw = CONTAINER_OF(rsc, struct narada_switch_resource, rsc);
	int ret;

	json_obj_parse((void *)data, len, json_switch_state_descr, json_switch_state_descr_size,
		       &switch_data);

	switch (switch_data.state) {
	case SWITCH_MSG_STATE_ON:
		ret = sw->api->set_state(sw, 1);
		break;
	case SWITCH_MSG_STATE_OFF:
		ret = sw->api->set_state(sw, 0);
		break;
	default:
		LOG_ERR("Set an unsupported SWITCH state: %x", switch_data.state);
	}

	if (ret) {
		LOG_ERR("Failed to set switch state: %d", ret);
	}
}
