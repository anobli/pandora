/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esphome/components/api.h>
#include <esphome/components/entity.h>
#include <esphome/components/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ESPHome, CONFIG_ESPHOME_LOG_LEVEL);

static int esphome_sensor_service(void *arg1, void *arg2, void *arg3)
{
	while (1) {
		STRUCT_SECTION_FOREACH(esphome_sensor_entity, sensor) {
			const struct esphome_entity *entity = sensor->entity;
			esphome_sensor_send_state(entity->data->api_dev, entity);
		}
		k_sleep(K_MSEC(1000));
	}

	return 0;
}

#define ESPHOME_SENSOR_STACK_SIZE 2048
K_THREAD_DEFINE(esphome_sensor_tid, ESPHOME_SENSOR_STACK_SIZE, esphome_sensor_service, NULL, NULL,
		NULL, 0, 0, 0);
