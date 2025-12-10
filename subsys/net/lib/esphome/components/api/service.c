/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <esphome/components/entity.h>
#include <esphome/esphome.h>
#include <rpc/esphome_rpc.h>

#ifdef CONFIG_ESPHOME_COMPONENT_OPENTHREAD
#include "../openthread/service.h"
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ESPHome, CONFIG_ESPHOME_LOG_LEVEL);

#define ESPHOME_STACK_SIZE (4096)
#define ESPHOME_PRIORITY   (5)

#define ESPHOME_SENSOR_STACK_SIZE (2048)

static int esphome_init(const struct device *dev)
{
#ifdef CONFIG_ESPHOME_COMPONENT_OPENTHREAD
	esphome_ot_init(dev);
#endif
	esphome_entity_init(dev);
	return 0;
}

static const struct esphome_api_config esphome_config = {
	.password = DT_INST_PROP_OR(ESPHOME_API_NODE, password, NULL),
	.port = DT_PROP(ESPHOME_API_NODE, port),
	.api_version_major = 1,
	.api_version_minor = 10,
	.server_info = "",
};

const struct esphome_api_config *esphome_get_api_config(void)
{
	return &esphome_config;
}

static struct esphome_data esphome_data;
DEVICE_DT_DEFINE(ESPHOME_API_NODE, esphome_init, NULL, &esphome_data, &esphome_config, POST_KERNEL,
		 CONFIG_ESPHOME_INIT_PRIORITY, NULL);

K_THREAD_DEFINE(esphome_api_tid, ESPHOME_STACK_SIZE, esphome_rpc_service,
		DEVICE_DT_GET(ESPHOME_API_NODE), DT_PROP(ESPHOME_API_NODE, port), NULL,
		0 /* todo: set priority */, 0, 0);
