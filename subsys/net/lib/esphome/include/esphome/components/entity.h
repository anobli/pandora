/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESPHOME_COMPONENT_ENTITY_H
#define ESPHOME_COMPONENT_ENTITY_H

#ifdef CONFIG_ESPHOME_COMPONENT_API

#include <stdlib.h>
#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>
#include <esphome/components/api.h>

#ifndef CONFIG_PROTOBUF_C
#define DT_ENTITY_STRCPY_SAFE(_resp, _cfg, _name)                                                  \
	_string_copy_safe((_resp)->_name, (_cfg)->_name, ARRAY_SIZE((_resp)->_name))
#else
#define DT_ENTITY_STRCPY_SAFE(_resp, _cfg, _name) (_resp)->_name = (char *)(_cfg)->_name
#endif

#define DT_ENTITY_CONFIG_TO_RESPONSE(_resp, _cfg)                                                  \
	char uid_buf[MAX_UNIQUE_ID_LEN];                                                           \
	DT_ENTITY_STRCPY_SAFE(_resp, _cfg, name);                                                  \
	DT_ENTITY_STRCPY_SAFE(_resp, _cfg, object_id);                                             \
	(_resp)->unique_id= esphome_build_unique_id((_cfg)->unique_id, uid_buf, MAX_UNIQUE_ID_LEN);\
	DT_ENTITY_STRCPY_SAFE(_resp, _cfg, icon);                                                  \
	(_resp)->disabled_by_default = (_cfg)->disabled_by_default;                                \
	(_resp)->entity_category = (_cfg)->entity_category;                                        \
	DT_ENTITY_STRCPY_SAFE(_resp, _cfg, device_class);

#define DT_ESPHOME_NAME DT_PROP(DT_PATH(esphome), entity_id)
#define DT_ESPHOME_UNIQUE_NAME(_num, _device_class)                                                \
	DT_ESPHOME_NAME "_" _device_class "_" DT_INST_PROP(_num, device_name)

#define DT_ESPHOME_ENTITY(_num, _device_class)                                                     \
	{                                                                                          \
		.name = DT_INST_PROP(_num, device_name),                                           \
		.object_id = STRINGIFY(DT_STRING_TOKEN(DT_DRV_INST(_num), device_name)),            \
				       .unique_id = DT_ESPHOME_UNIQUE_NAME(_num, _device_class),   \
				       .icon = NULL, .disabled_by_default = 0,                     \
				       .entity_category = 0, .device_class = _device_class,        \
		}

#define ESPHOME_UNIT_PERCENT "%"
#define ESPHOME_UNIT_CELSUIS "°C"
#define ESPHOME_UNIT_FAHRENHEIT "°F"

struct esphome_entity_config {
	const char *object_id;
	const char *name;
	const char *unique_id;
	const char *icon;
	bool disabled_by_default;
	EntityCategory entity_category;
	const char *device_class;
};

struct esphome_entity_data {
	uint32_t key;
	/* Find a way to get it using DT */
	const struct device *api_dev;
};

struct esphome_entity {
	const struct device *dev;
	const struct esphome_entity_config *config;
	const void *private_config;
	struct esphome_entity_data *data;
	int (*list_entity)(const struct device *api_dev, struct esphome_entity *entity);
};

#define DEFINE_ESPHOME_ENTITY_WITH_CONF(_num, name, _device_class, _list_entity, _priv_conf)       \
	static struct esphome_entity_config name##_entity_config =                                 \
		DT_ESPHOME_ENTITY(_num, _device_class);                                            \
	static struct esphome_entity_data name##_entity_data;                                      \
	STRUCT_SECTION_ITERABLE(esphome_entity, name) = {                                          \
		.dev = DEVICE_DT_GET(DT_DRV_INST(_num)),                                           \
		.config = &name##_entity_config,                                                   \
		.private_config = _priv_conf,                                                      \
		.data = &name##_entity_data,                                                       \
		.list_entity = _list_entity,                                                       \
	}

#define DEFINE_ESPHOME_ENTITY(_num, name, _device_class, _list_entity)                             \
	DEFINE_ESPHOME_ENTITY_WITH_CONF(_num, name, _device_class, _list_entity, NULL)

#define MAX_UNIQUE_ID_LEN 64

int _string_copy_safe(char *dest, const char *src, size_t len);
#define strcpy_safe(_dest, _src) _string_copy_safe(_dest, _src, ARRAY_SIZE(_dest))

uint32_t fnv1_hash(const char *str);
const struct device *find_device_entity_by_key(uint32_t key);
int esphome_entity_init(const struct device *api_dev);
char *esphome_build_unique_id(const char *base_name, char *buffer, int len);
#else

#define DEFINE_ESPHOME_ENTITY(_num, name, _device_class, _list_entity)

#endif /* CONFIG_ESPHOME_COMPONENT_API */

#endif /* ESPHOME_COMPONENT_ENTITY_H */
