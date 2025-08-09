#ifndef HERMES_SETTINGS_H
#define HERMES_SETTINGS_H

#include <zephyr/device.h>
#include <zephyr/sys/iterable_sections.h>

#include <hermes/hermes.h>

#ifdef CONFIG_SETTINGS
struct hermes_setting {
	const struct device *dev;
	int (*load_cb)(const struct device *dev);
	int (*save_cb)(const struct device *dev);
	int (*erase_cb)(const struct device *dev);
};

#define HERMES_SETTINGS_BUILD(_domain, _dev, _load, _save, _erase)                                 \
	STRUCT_SECTION_ITERABLE(hermes_setting, DT_CAT(_domain, _settings)) = {                    \
		.dev = _dev,                                                                       \
		.load_cb = _load,                                                                  \
		.save_cb = _save,                                                                  \
		.erase_cb = _erase,                                                                \
	}

#define HERMES_SETTINGS(_domain, _dev, _load, _save, _erase)                                       \
	HERMES_SETTINGS_BUILD(_domain, _dev, _load, _save, _erase)

#define DT_HERMES_SETTINGS(node_id, _load, _save, _erase)                                          \
	HERMES_SETTINGS(DT_NODE_FULL_NAME_TOKEN(node_id), DEVICE_DT_GET(node_id), _load, _save,    \
			_erase)

int hermes_settings_save_one(const char *domain, const char *prop, void *value, size_t val_len);
int hermes_settings_load_one(const char *domain, const char *prop, void *value, size_t val_len);
int hermes_settings_erase_one(const char *domain, const char *prop);

int hermes_settings_load_all(void);
int hermes_settings_save_all(void);
int hermes_settings_erase_all(void);
#else

#define HERMES_SETTINGS(_domain, _dev, _load, _save, _erase)
#define DT_HERMES_SETTINGS(node_id, _load, _save, _erase)

int hermes_settings_save_one(const char *domain, const char *prop, void *value, size_t val_len)
{
	return -ENOSYS;
}

int hermes_settings_load_one(const char *domain, const char *prop, void *value, size_t val_len)
{
	return -ENOSYS;
}

int hermes_settings_erase_one(const char *domain, const char *prop)
{
	return -ENOSYS;
}

static inline int hermes_settings_load_all(void)
{
	return -ENOSYS;
}

static inline int hermes_settings_save_all(void)
{
	return -ENOSYS;
}

static inline int hermes_settings_erase_all(void)
{
	return -ENOSYS;
}
#endif

#endif /* HERMES_SETTINGS_H */
