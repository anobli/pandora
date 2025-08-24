/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#include <hermes/hermes.h>

#define HERMES_BUILD_DEVICE_TYPE(type) DT_CAT3(HERMES_, type, _DEVICE_TYPE)

#define HERMES_DEVICE_TYPE(node) HERMES_BUILD_DEVICE_TYPE(DT_STRING_UPPER_TOKEN(node, device_type))

#define HERMES_INIT_ENTITY(node)                                                                   \
	[DT_NODE_CHILD_IDX(node)] {                                                                \
		.entity_id = STRINGIFY(DT_NODE_FULL_NAME_TOKEN(node)),                             \
				       .type = HERMES_DEVICE_TYPE(node),                           \
	},

#define HERMES_INIT_ENTITIES(node) DT_FOREACH_CHILD_STATUS_OKAY(node, HERMES_INIT_ENTITY)

#define HERMES_DEVICE(node)                                                                        \
	struct hermes_device HERMES_DEVICE_NAME(node) = {                                          \
		.info = {                                                                          \
			.device_id = DT_PROP(node, device_id),                                     \
			.manufacturer = DT_PROP_OR(node, manufacturer, ""),                        \
			.firmware_version = DT_PROP_OR(node, firmware_version, ""),                \
			.hardware_version = DT_PROP_OR(node, hardware_version, ""),                \
			.heartbeat_interval = DT_PROP(node, heartbeat_interval),                   \
			.entities = {HERMES_INIT_ENTITIES(node)},                                  \
			.entities_count = DT_CHILD_NUM(node),                                      \
		}};

DT_FOREACH_HERMES_DEVICE(HERMES_DEVICE)
