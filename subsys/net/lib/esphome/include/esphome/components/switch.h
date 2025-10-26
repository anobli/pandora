#ifndef ESPHOME_SWITCH_COMPONENT
#define ESPHOME_SWITCH_COMPONENT

#include <zephyr/device.h>

#include <esphome/components/entity.h>

struct esphome_switch_component_api {
	int (*set_state)(const struct device *dev, int state);
	int (*get_state)(const struct device *dev);
};

static inline int esphome_switch_get_state(const struct device *dev, int *state)
{
	const struct esphome_switch_component_api *api = dev->api;

	*state = api->get_state(dev);
	return *state < 0 ? *state : 0;
}

static inline int esphome_switch_set_state(const struct device *dev, int state)
{
	const struct esphome_switch_component_api *api = dev->api;

	return api->set_state(dev, state);
}

static inline int esphome_switch_turn_on(const struct device *dev)
{
	const struct esphome_switch_component_api *api = dev->api;

	return api->set_state(dev, true);
}

static inline int esphome_switch_turn_off(const struct device *dev)
{
	const struct esphome_switch_component_api *api = dev->api;

	return api->set_state(dev, false);
}

static inline int esphome_switch_toggle(const struct device *dev)
{
	int state;
	int ret;

	ret = esphome_switch_get_state(dev, &state);
	if (ret < 0) {
		return ret;
	}

	if (state) {
		return esphome_switch_turn_off(dev);
	}
	return esphome_switch_turn_on(dev);
}

#ifdef CONFIG_ESPHOME_COMPONENT_API
static inline int esphome_switch_list_entity(const struct device *api_dev,
					     struct esphome_entity *entity)
{
	const struct esphome_entity_config *config = entity->config;
	struct esphome_entity_data *data = entity->data;
	ListEntitiesSwitchResponse response = LIST_ENTITIES_SWITCH_RESPONSE__INIT;

	DT_ENTITY_CONFIG_TO_RESPONSE(&response, config);
	response.key = data->key;
	//         response.assumed_state = config->entity.assumed_state;
	ListEntitiesSwitchResponseWrite(api_dev, &response);

	return 0;
}
#endif

#endif /* ESPHOME_SWITCH_COMPONENT */
