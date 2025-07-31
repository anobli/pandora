#ifndef NARADA_DEVICE_H
#define NARADA_DEVICE_H

#include <zephyr/devicetree.h>

struct narada_entity {
	const char *entity_id;
	uint32_t type;
	uint32_t capabilities;
};

struct narada_device_info {
	const char *device_id;
	const char *manufacturer;
	const char *firmware_version;
	const char *hardware_version;
	uint32_t heartbeat_interval;
	struct narada_entity entities[10];
	int entities_count;
};

struct narada_device {
	struct narada_client client;
	struct narada_device_info info;
};

#define NARADA_DEVICE_NAME_CAT(prefix, name) DT_CAT(prefix, name)

#define NARADA_DEVICE_NAME(node)                                                                   \
	NARADA_DEVICE_NAME_CAT(narada_device_, DT_NODE_FULL_NAME_TOKEN(node))

#define NARADA_DECLARE_DEVICE(node) extern struct narada_device NARADA_DEVICE_NAME(node);

#define DT_FOREACH_NARADA_DEVICE(fn) DT_FOREACH_STATUS_OKAY(gordios_narada, fn)

DT_FOREACH_NARADA_DEVICE(NARADA_DECLARE_DEVICE)

#define NARADA_GET_DEVICE(node) &NARADA_DEVICE_NAME(node)

#if DT_HAS_CHOSEN(narada_device)
#define NARADA_CHOSEN_DEVICE NARADA_GET_DEVICE(DT_CHOSEN(narada_device))
#endif

#define NARADA_INIT_CLIENT(node)                                                                   \
	ret = narada_client_init(&(NARADA_GET_DEVICE(node))->client);                              \
	if (ret) {                                                                                 \
		return ret;                                                                        \
	}

static inline int narada_devices_init(void)
{
	int ret = 0;

	DT_FOREACH_NARADA_DEVICE(NARADA_INIT_CLIENT)

	return ret;
}

#endif /* NARADA_DEVICE_H */
