#ifndef NARADA_DISCOVERY_H
#define NARADA_DISCOVERY_H

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <narada/narada.h>

enum narada_discovery_version {
	NARADA_DISCOVERY_1_0,
	NARADA_DISCOVERY_LATEST,
};

/* Discovery server endpoints */
#define NARADA_DISCOVERY_VERSION_PATH   "discovery/version"
#define NARADA_DISCOVERY_REGISTER_PATH  "discovery/register"
#define NARADA_DISCOVERY_HEARTBEAT_PATH "discovery/heartbeat"
#define NARADA_WELL_KNOWN_CORE_PATH     ".well-known/core"

/* Forward declaration */
struct narada_device;

///* Discovery-specific device info */
// struct narada_discovery_device_info {
//	struct narada_device *device; /* Pointer to narada device */
//	const char **resources;       /* CoAP resources (discovery-specific) */
//	size_t resources_count;
//	uint32_t heartbeat_interval; /* seconds, 0 = no heartbeat */
// };

/* Discovery client structure */
struct narada_discovery_client {
	struct narada_client *narada_client;
	struct narada_device *device;
	//	struct narada_discovery_device_info device_info;
	bool registered;
	bool server_discovered;
	struct k_work_delayable heartbeat_work;
	char server_ip[INET_ADDRSTRLEN];
	uint16_t server_port;
};

/* Discovery client functions */
int narada_discovery_client_init(struct narada_discovery_client *client,
				 struct narada_client *narada_client);

// int narada_discovery_client_set_device_info(struct narada_discovery_client *client,
//					    const struct narada_discovery_device_info *info);

int narada_discovery_discover_server(struct narada_discovery_client *client, k_timeout_t timeout);

int narada_discovery_set_server_address(struct narada_discovery_client *client,
					const char *server_ip, uint16_t server_port);

int narada_discovery_get_server_version(struct narada_discovery_client *client, uint32_t *major,
					uint32_t *minor);

int narada_discovery_register_device(struct narada_discovery_client *client);

int narada_discovery_send_heartbeat(struct narada_discovery_client *client);

int narada_discovery_start_heartbeat(struct narada_discovery_client *client);

int narada_discovery_stop_heartbeat(struct narada_discovery_client *client);

/* Device tree helper - get the DT-configured discovery client instance */
#if DT_HAS_COMPAT_STATUS_OKAY(narada_discovery)
struct narada_discovery_client *narada_discovery_dt_get_client(void);

/* Simple discovery start function for state machine */
int narada_discovery_start(void);

/* Complete discovery with server IP (when IP capture is available) */
int narada_discovery_complete_with_server_ip(const char *server_ip);
#endif

#endif /* NARADA_DISCOVERY_H */
