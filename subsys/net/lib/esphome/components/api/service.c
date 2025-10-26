/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nabucasa_esphome_api

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <esphome/components/entity.h>
#include <esphome/esphome.h>
#include <rpc/esphome_rpc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ESPHome, CONFIG_ESPHOME_LOG_LEVEL);

#define ESPHOME_STACK_SIZE (4096)
#define ESPHOME_PRIORITY   (5)

#define ESPHOME_SENSOR_STACK_SIZE (2048)

static int esphome_init(const struct device *dev)
{
	esphome_entity_init(dev);
	return 0;
}

#define DEFINE_ESPHOME(_num)                                                                       \
                                                                                                   \
	static const struct esphome_config esphome_config_##_num = {                               \
		.name = DT_PROP(DT_INST_PARENT(_num), entity_id),                                  \
		.friendly_name = DT_PROP_OR(DT_INST_PARENT(_num), friendly_name, ""),              \
		.password = DT_INST_PROP_OR(_num, password, ""),                                   \
		.port = DT_INST_PROP(_num, port),                                                  \
		.api_version_major = 1,                                                            \
		.api_version_minor = 10,                                                           \
		.compilation_time = __DATE__ " " __TIME__,                                         \
		.server_info = "",                                                                 \
	};                                                                                         \
	static struct esphome_data esphome_data_##_num;                                            \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_num, esphome_init, NULL, &esphome_data_##_num,                      \
			      &esphome_config_##_num, POST_KERNEL, CONFIG_ESPHOME_INIT_PRIORITY,   \
			      NULL);                                                               \
                                                                                                   \
	K_THREAD_DEFINE(esphome_tid_##_num, ESPHOME_STACK_SIZE, esphome_rpc_service,               \
			DEVICE_DT_INST_GET(_num), DT_INST_PROP(_num, port), NULL,                  \
			0 /* todo: set priority */, 0, 0);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_ESPHOME);
