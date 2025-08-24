/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#ifndef HERMES_DEVICE_H
#define HERMES_DEVICE_H

#include <zephyr/devicetree.h>

struct hermes_entity {
	const char *entity_id;
	uint32_t type;
	uint32_t capabilities;
};

struct hermes_device_info {
	const char *device_id;
	const char *manufacturer;
	const char *firmware_version;
	const char *hardware_version;
	uint32_t heartbeat_interval;
	struct hermes_entity entities[10];
	int entities_count;
};

struct hermes_device {
	struct hermes_client client;
	struct hermes_device_info info;
};

#define HERMES_DEVICE_NAME_CAT(prefix, name) DT_CAT(prefix, name)

#define HERMES_DEVICE_NAME(node)                                                                   \
	HERMES_DEVICE_NAME_CAT(hermes_device_, DT_NODE_FULL_NAME_TOKEN(node))

#define HERMES_DECLARE_DEVICE(node) extern struct hermes_device HERMES_DEVICE_NAME(node);

#define DT_FOREACH_HERMES_DEVICE(fn) DT_FOREACH_STATUS_OKAY(pandora_device, fn)

DT_FOREACH_HERMES_DEVICE(HERMES_DECLARE_DEVICE)

#define HERMES_GET_DEVICE(node) &HERMES_DEVICE_NAME(node)

#if DT_HAS_CHOSEN(hermes_device)
#define HERMES_CHOSEN_DEVICE HERMES_GET_DEVICE(DT_CHOSEN(hermes_device))
#endif

#define HERMES_INIT_CLIENT(node)                                                                   \
	ret = hermes_client_init(&(HERMES_GET_DEVICE(node))->client);                              \
	if (ret) {                                                                                 \
		return ret;                                                                        \
	}

static inline int hermes_devices_init(void)
{
	int ret = 0;

	DT_FOREACH_HERMES_DEVICE(HERMES_INIT_CLIENT)

	return ret;
}

#endif /* HERMES_DEVICE_H */
