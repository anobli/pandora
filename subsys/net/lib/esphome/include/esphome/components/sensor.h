#ifndef ESPHOME_SENSOR_COMPONENT_H
#define ESPHOME_SENSOR_COMPONENT_H

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include <esphome/components/api.h>
#include <esphome/components/entity.h>

struct esphome_sensor_data {
	struct sensor_trigger trig;
	bool data_rdy;
};

struct esphome_sensor_api {
	int (*init)(const struct device *dev);
	int (*read)(const struct device *dev, float *state);
};

static inline int esphome_sensor_init(const struct device *dev)
{
	const struct esphome_sensor_api *api = dev->api;

	if (api->init) {
		return api->init(dev);
	}

	return 0;
}

static inline int esphome_sensor_read(const struct device *dev, float *state)
{
	const struct esphome_sensor_api *api = dev->api;

	return api->read(dev, state);
}

#ifdef CONFIG_ESPHOME_COMPONENT_API
struct esphome_sensor_entity {
	const struct esphome_entity *entity;
};

struct esphome_sensor_config {
	const char *unit_of_measurement;
};

#define DEFINE_ESPHOME_SENSOR_ENTITY(_num, name)                                                   \
	static struct esphome_sensor_config esphome_sensor_config##_num = {                        \
		.unit_of_measurement = DT_STRING_UPPER_TOKEN_OR(DT_DRV_INST(_num), unit, NULL),    \
	};                                                                                         \
	DEFINE_ESPHOME_ENTITY_WITH_CONF(_num, name, "sensor", esphome_sensor_list_entity,          \
					&esphome_sensor_config##_num);                             \
	STRUCT_SECTION_ITERABLE(esphome_sensor_entity, name##sensor_entity) = {                    \
		.entity = &name,                                                                   \
	}

static inline void esphome_sensor_send_state(const struct device *api_dev,
					     const struct esphome_entity *entity)
{
	struct esphome_entity_data *data = entity->data;
	const struct device *dev = entity->dev;
	SensorStateResponse response = SENSOR_STATE_RESPONSE__INIT;

	/* TODO: Protect me */
	response.key = data->key;
	esphome_sensor_read(dev, &response.state);
	SensorStateResponseWrite(api_dev, &response);
}

static inline int esphome_sensor_list_entity(const struct device *api_dev,
					     struct esphome_entity *entity)
{
	const struct esphome_entity_config *config = entity->config;
	const struct esphome_sensor_config *sensor_config = entity->private_config;
	struct esphome_entity_data *data = entity->data;
	ListEntitiesSensorResponse response = LIST_ENTITIES_SENSOR_RESPONSE__INIT;

	DT_ENTITY_CONFIG_TO_RESPONSE(&response, config);
	DT_ENTITY_STRCPY_SAFE(&response, sensor_config, unit_of_measurement);
	response.accuracy_decimals = 2;
	response.key = data->key;
	ListEntitiesSensorResponseWrite(api_dev, &response);

	return 0;
}
#else /* CONFIG_ESPHOME_COMPONENT_API */
#define DEFINE_ESPHOME_SENSOR_ENTITY(_num, name)
#endif /* CONFIG_ESPHOME_COMPONENT_API */

#endif /* ESPHOME_SENSOR_COMPONENT_H */
