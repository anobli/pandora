/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESPHOME_COMPONENT_BUTTON_H
#define ESPHOME_COMPONENT_BUTTON_H

#include <zephyr/device.h>

#include <esphome/components/api.h>
#include <esphome/components/entity.h>

typedef int (*esphome_on_press)(const struct device *dev);

struct esphome_button_config {
	esphome_on_press on_press;
};

static inline int esphome_button_on_press(const struct device *dev)
{
	const struct esphome_button_config *config = dev->config;

	return config->on_press(dev);
}

#ifdef CONFIG_ESPHOME_COMPONENT_API
static inline int esphome_button_list_entity(const struct device *api_dev,
					     struct esphome_entity *entity)
{
	const struct esphome_entity_config *config = entity->config;
	struct esphome_entity_data *data = entity->data;
	ListEntitiesButtonResponse response = LIST_ENTITIES_BUTTON_RESPONSE__INIT;

	DT_ENTITY_CONFIG_TO_RESPONSE(&response, config);
	response.key = data->key;
	ListEntitiesButtonResponseWrite(api_dev, &response);

	return 0;
}
#endif /* CONFIG_ESPHOME_COMPONENT_API */

#endif /* ESPHOME_COMPONENT_BUTTON_H */