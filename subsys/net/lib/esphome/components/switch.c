#include <pandora/components/switch.h>
#include <esphome/components/entity.h>

static int esphome_switch_list_entity(const struct device *api_dev, struct esphome_entity *entity)
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

static int esphome_switch_publish_state(const struct device *api_dev, struct esphome_entity *entity)
{
	const struct device *switch_dev;
	SwitchStateResponse response = SWITCH_STATE_RESPONSE__INIT;

	switch_dev = find_device_entity_by_key(entity->data->key);
	if (!switch_dev) {
		return -ENODEV;
	}

	pandora_switch_get_state(switch_dev, &response.state);
	response.key = entity->data->key;

	return SwitchStateResponseWrite(api_dev, &response);
}

PANDORA_CB(switch, on_state, esphome_switch_on_state, NULL)
{
	struct esphome_entity *entity = find_entity_by_dev(dev);
	if (entity) {
		SwitchStateResponse response = SWITCH_STATE_RESPONSE__INIT;

		response.state = state;
		response.key = entity->data->key;

		SwitchStateResponseWrite(ESPHOME_API_DEVICE, &response);
	}
}

#define DEFINE_ESPHOME_SWITCH(_num, _component)                                                    \
	DEFINE_ESPHOME_ENTITY(_num, esphome_switch_##_component##_num, "switch." #_component,      \
			      esphome_switch_list_entity, esphome_switch_publish_state);

DT_FOREACH_STATUS_OKAY_VARGS(pandora_switch_gpio, DEFINE_ESPHOME_SWITCH, gpio);
DT_FOREACH_STATUS_OKAY_VARGS(pandora_switch_hbridge, DEFINE_ESPHOME_SWITCH, hbridge);

/* This exposes switch to enable or disable pandora pipelines */
DT_FOREACH_STATUS_OKAY_VARGS(pandora_pipeline, DEFINE_ESPHOME_SWITCH, pipeline);

static int esphome_switch_init(void)
{
	pandora_cb_enable(switch, on_state, &esphome_switch_on_state);

	return 0;
}

SYS_INIT(esphome_switch_init, APPLICATION, 99);
