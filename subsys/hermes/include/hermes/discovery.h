#ifndef HERMES_DISCOVERY_H
#define HERMES_DISCOVERY_H

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <hermes/hermes.h>

enum hermes_discovery_version {
	HERMES_DISCOVERY_1_0,
	HERMES_DISCOVERY_LATEST,
};

/* Discovery server endpoints */
#define HERMES_DISCOVERY_VERSION_PATH   "discovery/version"
#define HERMES_DISCOVERY_REGISTER_PATH  "discovery/register"
#define HERMES_DISCOVERY_HEARTBEAT_PATH "discovery/heartbeat"
#define HERMES_WELL_KNOWN_CORE_PATH     ".well-known/core"

/* Forward declaration */
struct hermes_device;

/* Discovery client structure */
struct hermes_discovery_client {
	struct hermes_client *hermes_client;
	struct hermes_device *device;
	//	struct hermes_discovery_device_info device_info;
	bool registered;
	bool server_discovered;
	struct k_work_delayable heartbeat_work;
	char server_ip[INET_ADDRSTRLEN];
	uint16_t server_port;
};

/* Discovery client functions */
int hermes_discovery_client_init(struct hermes_discovery_client *client,
				 struct hermes_client *hermes_client);

// int hermes_discovery_client_set_device_info(struct hermes_discovery_client *client,
//					    const struct hermes_discovery_device_info *info);

int hermes_discovery_discover_server(struct hermes_discovery_client *client, k_timeout_t timeout);

int hermes_discovery_set_server_address(struct hermes_discovery_client *client,
					const char *server_ip, uint16_t server_port);

int hermes_discovery_get_server_version(struct hermes_discovery_client *client, uint32_t *major,
					uint32_t *minor);

int hermes_discovery_register_device(struct hermes_discovery_client *client);

int hermes_discovery_send_heartbeat(struct hermes_discovery_client *client);

int hermes_discovery_start_heartbeat(struct hermes_discovery_client *client);

int hermes_discovery_stop_heartbeat(struct hermes_discovery_client *client);

/* Device tree helper - get the DT-configured discovery client instance */
struct hermes_discovery_client *hermes_discovery_dt_get_client(void);

/* Simple discovery start function for state machine */
int hermes_discovery_start(void);

/* Complete discovery with server IP (when IP capture is available) */
int hermes_discovery_complete_with_server_ip(const char *server_ip);

#endif /* HERMES_DISCOVERY_H */
