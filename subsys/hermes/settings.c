/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(hermes, CONFIG_HERMES_LOG_LEVEL);

#include <hermes/hermes.h>
#include <hermes/settings.h>

#define KEY_LEN_MAX 64

int hermes_settings_save_one(const char *domain, const char *prop, void *value, size_t val_len)
{
	char key[KEY_LEN_MAX];
	int ret;

	snprintf(key, KEY_LEN_MAX, "%s.%s", domain, prop);
	ret = settings_save_one(key, value, val_len);
	if (ret) {
		LOG_ERR("Failed to save %s from settings: %d", key, ret);
		return ret;
	}

	return 0;
}

int hermes_settings_load_one(const char *domain, const char *prop, void *value, size_t val_len)
{
	char key[KEY_LEN_MAX];
	int ret;

	snprintf(key, KEY_LEN_MAX, "%s.%s", domain, prop);
	ret = settings_load_one(key, value, val_len);
	if (ret < 0) {
		LOG_ERR("Failed to load %s from settings", key);
	}

	return ret;
}

int hermes_settings_erase_one(const char *domain, const char *prop)
{
	char key[KEY_LEN_MAX];
	int ret;

	snprintf(key, KEY_LEN_MAX, "%s.%s", domain, prop);
	ret = settings_delete(key);
	if (ret < 0) {
		LOG_ERR("Failed to delete %s from settings", key);
	}

	return ret;
}

int hermes_settings_load_all(void)
{
	int ret;

	STRUCT_SECTION_FOREACH(hermes_setting, setting) {
		ret = setting->load_cb(setting->dev);
		if (ret) {
			if (setting->dev) {
				LOG_ERR("Failed to load %s settings", setting->dev->name);
			} else {
				LOG_ERR("Failed to load settings");
			}
			return ret;
		}
	}

	return 0;
}

int hermes_settings_save_all(void)
{
	int ret;

	STRUCT_SECTION_FOREACH(hermes_setting, setting) {
		ret = setting->save_cb(setting->dev);
		if (ret) {
			if (setting->dev) {
				LOG_ERR("Failed to save %s settings", setting->dev->name);
			} else {
				LOG_ERR("Failed to savloade settings");
			}
			return ret;
		}
	}

	return 0;
}

int hermes_settings_erase_all(void)
{
	int ret;

	STRUCT_SECTION_FOREACH(hermes_setting, setting) {
		if (!setting->erase_cb) {
			continue;
		}

		ret = setting->erase_cb(setting->dev);
		if (ret) {
			if (setting->dev) {
				LOG_ERR("Failed to erase %s settings", setting->dev->name);
			} else {
				LOG_ERR("Failed to erase settings");
			}
			return ret;
		}
	}

	return 0;
}

static int hermes_settings_init(void)
{
	int ret;

	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Failed to initialize settings");
		return ret;
	}
	hermes_settings_load_all();

	return 0;
}

SYS_INIT(hermes_settings_init, APPLICATION, 0);
