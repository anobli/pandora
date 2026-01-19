/*
 * Copyright (c) 2026 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PANDORA_COMPONENT_BINARY_SENSOR_H
#define PANDORA_COMPONENT_BINARY_SENSOR_H

#include <zephyr/device.h>

#include <pandora/components/component.h>
#include <pandora/components/component_callback.h>

#define DT_INST_DEFINE_PANDORA_COMPONENT_BINARY_SENSOR(_inst)                                      \
	DT_INST_DEFINE_PANDORA_COMPONENT(_inst, PANDORA_COMPONENT_BINARY_SENSOR)

#define binary_sensor_on_state_params int, state
DECLARE_PANDORA_CB(binary_sensor, on_state);

struct pandora_component_binary_sensor_api {
	int (*get_state)(const struct device *dev);
};

int pandora_binary_sensor_get_state(const struct device *dev, int *state);

#endif /* PANDORA_COMPONENT_BINARY_SENSOR_H */
