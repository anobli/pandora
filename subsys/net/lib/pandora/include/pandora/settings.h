/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#ifndef PANDORA_SETTINGS_H
#define PANDORA_SETTINGS_H

#include <zephyr/device.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef CONFIG_SETTINGS
struct pandora_setting {
	const struct device *dev;
	int (*load_cb)(const struct device *dev);
	int (*save_cb)(const struct device *dev);
	int (*erase_cb)(const struct device *dev);
};

#define PANDORA_SETTINGS_BUILD(_domain, _dev, _load, _save, _erase)                                \
	STRUCT_SECTION_ITERABLE(pandora_setting, DT_CAT(_domain, _settings)) = {                   \
		.dev = _dev,                                                                       \
		.load_cb = _load,                                                                  \
		.save_cb = _save,                                                                  \
		.erase_cb = _erase,                                                                \
	}

#define PANDORA_SETTINGS(_domain, _dev, _load, _save, _erase)                                      \
	PANDORA_SETTINGS_BUILD(_domain, _dev, _load, _save, _erase)

#define DT_PANDORA_SETTINGS(node_id, _load, _save, _erase)                                         \
	PANDORA_SETTINGS(DT_NODE_FULL_NAME_TOKEN(node_id), DEVICE_DT_GET(node_id), _load, _save,   \
			 _erase)

int pandora_settings_save_one(const char *domain, const char *prop, void *value, size_t val_len);
int pandora_settings_load_one(const char *domain, const char *prop, void *value, size_t val_len);
int pandora_settings_erase_one(const char *domain, const char *prop);

int pandora_settings_load_all(void);
int pandora_settings_save_all(void);
int pandora_settings_erase_all(void);
#else

#define PANDORA_SETTINGS(_domain, _dev, _load, _save, _erase)
#define DT_PANDORA_SETTINGS(node_id, _load, _save, _erase)

int pandora_settings_save_one(const char *domain, const char *prop, void *value, size_t val_len)
{
	return -ENOSYS;
}

int pandora_settings_load_one(const char *domain, const char *prop, void *value, size_t val_len)
{
	return -ENOSYS;
}

int pandora_settings_erase_one(const char *domain, const char *prop)
{
	return -ENOSYS;
}

static inline int pandora_settings_load_all(void)
{
	return -ENOSYS;
}

static inline int pandora_settings_save_all(void)
{
	return -ENOSYS;
}

static inline int pandora_settings_erase_all(void)
{
	return -ENOSYS;
}
#endif

#endif /* PANDORA_SETTINGS_H */
