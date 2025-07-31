#include <narada/narada.h>

#define NARADA_BUILD_DEVICE_TYPE(type) DT_CAT3(NARADA_, type, _DEVICE_TYPE)

#define NARADA_DEVICE_TYPE(node) NARADA_BUILD_DEVICE_TYPE(DT_STRING_UPPER_TOKEN(node, device_type))

#define NARADA_INIT_ENTITY(node)                                                                   \
	[DT_NODE_CHILD_IDX(node)] {                                                                \
		.entity_id = STRINGIFY(DT_NODE_FULL_NAME_TOKEN(node)),                             \
				       .type = NARADA_DEVICE_TYPE(node),                           \
	},

#define NARADA_INIT_ENTITIES(node) DT_FOREACH_CHILD_STATUS_OKAY(node, NARADA_INIT_ENTITY)

#define NARADA_DEVICE(node)                                                                        \
	struct narada_device NARADA_DEVICE_NAME(node) = {                                          \
		.info = {                                                                          \
			.device_id = DT_PROP(node, device_id),                                     \
			.manufacturer = DT_PROP_OR(node, manufacturer, ""),                        \
			.firmware_version = DT_PROP_OR(node, firmware_version, ""),                \
			.hardware_version = DT_PROP_OR(node, hardware_version, ""),                \
			.heartbeat_interval = DT_PROP(node, heartbeat_interval),                   \
			.entities = {NARADA_INIT_ENTITIES(node)},                                  \
			.entities_count = DT_CHILD_NUM(node),                                      \
		}};

DT_FOREACH_NARADA_DEVICE(NARADA_DEVICE)
