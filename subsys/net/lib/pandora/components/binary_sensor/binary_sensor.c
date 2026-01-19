/*
 * Copyright (c) 2026 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <pandora/components/binary_sensor.h>

DEFINE_PANDORA_CB(binary_sensor, on_state);

int pandora_binary_sensor_get_state(const struct device *dev, int *state)
{
	const struct pandora_component_binary_sensor_api *api = dev->api;

	*state = api->get_state(dev);
	return *state < 0 ? *state : 0;
}

int pandora_binary_sensor_update_state(const struct device *dev, int state)
{
	pandora_cb_call(binary_sensor, on_state, dev, state);
	return 0;
}
