#ifndef PANDORA_COMPONENT
#define PANDORA_COMPONENT

#include <zephyr/device.h>
#include <zephyr/sys/iterable_sections.h>

#define PANDORA_COMPONENT_UNDEFINED     0
#define PANDORA_COMPONENT_SWITCH        1
#define PANDORA_COMPONENT_BINARY_SENSOR 2

struct pandora_component_item {
	const struct device *dev;
	int type;
};

#define DEFINE_PANDORA_COMPONENT(name, _dev, _type)                                                \
	STRUCT_SECTION_ITERABLE(pandora_component_item, _CONCAT(pandora_component_, name)) = {     \
		.dev = _dev,                                                                       \
		.type = _type,                                                                     \
	}

#define DT_DEFINE_PANDORA_COMPONENT(_node, _type)                                                  \
	DEFINE_PANDORA_COMPONENT(DEVICE_DT_NAME_GET(_node), DEVICE_DT_GET(_node), _type)

#define DT_INST_DEFINE_PANDORA_COMPONENT(_inst, _type)                                             \
	DT_DEFINE_PANDORA_COMPONENT(DT_DRV_INST(_inst), _type)

#endif /* PANDORA_COMPONENT */
