/*
 * Copyright (c) 2025 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nabucasa_esphome_sensor_timestamp

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <esphome/components/api.h>
#include <esphome/components/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ESPHome, CONFIG_ESPHOME_LOG_LEVEL);

int device_read_timestamp(const struct device *dev, float *state)
{
	uint64_t ts;

	ARG_UNUSED(dev);

	ts = k_ticks_to_ns_floor64(k_uptime_ticks());
	*state = (float)ts / NSEC_PER_SEC;

	return 0;
}

struct esphome_sensor_api esphome_timestamp_sensor = {
	.read = device_read_timestamp,
};

#define DEFINE_ESPHOME_SENSOR_TIMESTAMP(_num)                                                      \
                                                                                                   \
	static struct esphome_sensor_data esphome_sensor_data_##_num;                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_num, esphome_sensor_init, NULL, &esphome_sensor_data_##_num, NULL,  \
			      POST_KERNEL, CONFIG_ESPHOME_INIT_PRIORITY,                           \
			      &esphome_timestamp_sensor);                                          \
	DEFINE_ESPHOME_SENSOR_ENTITY(_num, esphome_timestamp_sensor_##_num);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_ESPHOME_SENSOR_TIMESTAMP);
