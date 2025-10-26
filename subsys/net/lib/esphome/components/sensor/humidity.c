/*
 * Copyright (c) 2025 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nabucasa_esphome_sensor_humidity

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <esphome/components/api.h>
#include <esphome/components/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ESPHome, CONFIG_ESPHOME_LOG_LEVEL);

struct esphome_humidity_sensor_config {
	const struct device *sensor;
};

int device_read_humidity(const struct device *dev, float *state)
{
	const struct esphome_humidity_sensor_config *config = dev->config;
	struct sensor_value sensor_val;
	int ret;

	ret = sensor_sample_fetch(config->sensor);
	if (ret) {
		LOG_ERR("Failed to fetch sensor sample [%d]", ret);
		return ret;
	}

	ret = sensor_channel_get(config->sensor, SENSOR_CHAN_HUMIDITY, &sensor_val);
	if (ret) {
		LOG_ERR("Failed to get sensor channel [%d]", ret);
		return ret;
	}

	*state = sensor_value_to_float(&sensor_val);

	return ret;
}

void sensor_humidity_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	struct esphome_sensor_data *data = CONTAINER_OF(trigger, struct esphome_sensor_data, trig);

	ARG_UNUSED(dev);

	data->data_rdy = 1;
}

int device_init_humidity(const struct device *dev)
{
	const struct esphome_humidity_sensor_config *config = dev->config;
	struct esphome_sensor_data *data = dev->data;

	if (!device_is_ready(config->sensor)) {
		return -ENODEV;
	}

	/* TODO: use the delta trigger with configurable threshold */
	data->trig.type = SENSOR_TRIG_DATA_READY;
	data->trig.chan = SENSOR_CHAN_HUMIDITY;
	sensor_trigger_set(config->sensor, &data->trig, sensor_humidity_handler);

	return 0;
}

struct esphome_sensor_api esphome_humidity_sensor = {
	.init = device_init_humidity,
	.read = device_read_humidity,
};

#define DEFINE_ESPHOME_SENSOR_HUMIDITY(_num)                                                    \
                                                                                                   \
	struct esphome_humidity_sensor_config esphome_humidity_sensor_config##_num = {       \
		.sensor = DEVICE_DT_GET(DT_PHANDLE_BY_IDX(DT_DRV_INST(_num), sensor, 0)),          \
	};                                                                                         \
	static struct esphome_sensor_data esphome_sensor_data_##_num;                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_num, esphome_sensor_init, NULL, &esphome_sensor_data_##_num,        \
			      &esphome_humidity_sensor_config##_num, POST_KERNEL,               \
			      CONFIG_ESPHOME_INIT_PRIORITY, &esphome_humidity_sensor);          \
	DEFINE_ESPHOME_SENSOR_ENTITY(_num, esphome_humidity_sensor_##_num);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_ESPHOME_SENSOR_HUMIDITY);
