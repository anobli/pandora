#ifndef ESPHOME_API_COMPONENT_H
#define ESPHOME_API_COMPONENT_H

#ifdef CONFIG_ESPHOME_COMPONENT_API
#include <zephyr/device.h>

#include <rpc/esphome_rpc.h>
#include <rpc/api.pb-c.h>

#define ESPHOME_API_NODE   DT_COMPAT_GET_ANY_STATUS_OKAY(nabucasa_esphome_api)
#define ESPHOME_API_DEVICE DEVICE_DT_GET(ESPHOME_API_NODE)

struct esphome_api_config {
	uint32_t api_version_major;
	uint32_t api_version_minor;
	const char *server_info;

	const char *password;
	int port;
};

#define MAC_ADDRESS_LEN 24
char *get_mac_address_string(char *buffer, int size);
char *get_unique_device_name(const char *name, char *buffer, int size);
const struct esphome_api_config *esphome_get_api_config(void);

#endif

#endif /* ESPHOME_API_COMPONENT_H */
