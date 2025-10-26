/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nabucasa_esphome_button_template

#include <stdlib.h>

#include <zephyr/devicetree.h>

#include <esphome/components/api.h>
#include <esphome/components/button.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ESPHome, CONFIG_ESPHOME_LOG_LEVEL);

#define DEFINE_ESPHOME_BUTTON(_num)                                                                \
                                                                                                   \
	extern int DT_STRING_TOKEN(DT_DRV_INST(_num), on_press)(const struct device *dev);         \
	static const struct esphome_button_config esphome_button_config_##_num = {                 \
		.on_press = DT_STRING_TOKEN(DT_DRV_INST(_num), on_press),                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_num, NULL, NULL, NULL, &esphome_button_config_##_num, POST_KERNEL,  \
			      CONFIG_ESPHOME_INIT_PRIORITY, NULL);                                 \
	DEFINE_ESPHOME_ENTITY(_num, esphome_button_template_##_num, "button",                      \
			      esphome_button_list_entity);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_ESPHOME_BUTTON);
