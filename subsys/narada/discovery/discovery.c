#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(narada_discovery, CONFIG_NARADA_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/data/json.h>
#include <zephyr/devicetree.h>
#include <narada/discovery/discovery.h>

#include <narada/narada.h>
#ifdef CONFIG_NARADA_STATE
#include <narada/narada_state.h>
#endif

#define JSON_BUFFER_SIZE 1024

/* TODO: use number of child instead of static number */
#define MAX_ENTITIES 10

struct discovery_version_response {
	uint32_t major;
	uint32_t minor;
};

struct discovery_register_response {
	const char *status;
	const char *device_id;
	uint32_t heartbeat_interval;
	const char *heartbeat_endpoint;
	const char *note;
};

struct discovery_heartbeat_request {
	const char *device_id;
};

struct discovery_heartbeat_response {
	const char *status;
};

/* JSON descriptors */
static const struct json_obj_descr version_response_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct discovery_version_response, major, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct discovery_version_response, minor, JSON_TOK_NUMBER),
};

static const struct json_obj_descr register_request_entity_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct narada_entity, entity_id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct narada_entity, type, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct narada_entity, capabilities, JSON_TOK_NUMBER),
};

static const struct json_obj_descr register_request_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct narada_device_info, device_id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct narada_device_info, firmware_version, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct narada_device_info, hardware_version, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct narada_device_info, manufacturer, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct narada_device_info, heartbeat_interval, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct narada_device_info, entities, MAX_ENTITIES, entities_count,
				 register_request_entity_descr,
				 ARRAY_SIZE(register_request_entity_descr)),
};

static const struct json_obj_descr register_response_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct discovery_register_response, status, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct discovery_register_response, device_id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct discovery_register_response, heartbeat_interval,
			    JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct discovery_register_response, heartbeat_endpoint,
			    JSON_TOK_STRING),
};

static const struct json_obj_descr heartbeat_request_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct discovery_heartbeat_request, device_id, JSON_TOK_STRING),
};

static const struct json_obj_descr heartbeat_response_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct discovery_heartbeat_response, status, JSON_TOK_STRING),
};

static void heartbeat_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct narada_discovery_client *client =
		CONTAINER_OF(dwork, struct narada_discovery_client, heartbeat_work);

	int ret = narada_discovery_send_heartbeat(client);
	if (ret) {
		LOG_WRN("Failed to send heartbeat: %d", ret);
	}

	if (client->device->info.heartbeat_interval > 0) {
		k_work_schedule(&client->heartbeat_work,
				K_SECONDS(client->device->info.heartbeat_interval));
	}
}

static int server_discovery_handler(void *ctx, uint8_t *payload, int len,
				    const struct sockaddr *server_addr)
{
	struct narada_discovery_client *client = (struct narada_discovery_client *)ctx;
	char *core_links = (char *)payload;

	LOG_DBG("Received .well-known/core response (%d bytes): %.*s", len, len, core_links);

	/* Parse the CoRE Link Format to find discovery endpoints */
	if (strnstr(core_links, "</discovery/register>", len) &&
	    strnstr(core_links, "</discovery/version>", len)) {

		/* Extract server IP from the response source address */
		if (server_addr->sa_family == AF_INET) {
			const struct sockaddr_in *addr4 = (const struct sockaddr_in *)server_addr;
			char ip_str[INET_ADDRSTRLEN];
			uint16_t port = ntohs(addr4->sin_port);

			net_addr_ntop(AF_INET, &addr4->sin_addr, ip_str, INET_ADDRSTRLEN);

			strncpy(client->server_ip, ip_str, sizeof(client->server_ip) - 1);
			client->server_ip[sizeof(client->server_ip) - 1] = '\0';
			client->server_port = port;

			LOG_INF("Discovery server found at %s:%d", client->server_ip,
				client->server_port);

			int ret = narada_client_set_ipv4(client->narada_client, client->server_ip,
							 client->server_port);
			if (ret) {
				LOG_ERR("Failed to set server address: %d", ret);
				return ret;
			}

			client->server_discovered = true;
			LOG_INF("Discovery server validated and IP automatically captured");
			return 0;
		} else {
			LOG_WRN("IPv6 server discovery not yet supported");
			return -ENOTSUP;
		}
	}

	LOG_WRN("Server does not support discovery protocol");
	return -ENOTSUP;
}

struct version_response_context {
	uint32_t *major;
	uint32_t *minor;
};

static int version_response_handler(void *ctx, uint8_t *payload, int len)
{
	struct version_response_context *version_ctx = (struct version_response_context *)ctx;
	struct discovery_version_response version_resp;
	int ret;

	ret = json_obj_parse((char *)payload, len, version_response_descr,
			     ARRAY_SIZE(version_response_descr), &version_resp);
	if (ret < 0) {
		LOG_ERR("Failed to parse version response: %d", ret);
		LOG_ERR("%s", payload);
		return ret;
	}

	*version_ctx->major = version_resp.major;
	*version_ctx->minor = version_resp.minor;

	return 0;
}

static int register_response_handler(void *ctx, uint8_t *payload, int len)
{
	struct narada_discovery_client *client = (struct narada_discovery_client *)ctx;
	struct discovery_register_response reg_resp;
	int ret;

	memset(&reg_resp, 0, sizeof(reg_resp));

	ret = json_obj_parse((char *)payload, len, register_response_descr,
			     ARRAY_SIZE(register_response_descr), &reg_resp);
	if (ret < 0) {
		LOG_ERR("Failed to parse registration response: %d", ret);
		return ret;
	}

	if (reg_resp.status && strcmp(reg_resp.status, "registered") == 0) {
		client->registered = true;
		LOG_INF("Device successfully registered: %s",
			reg_resp.device_id ? reg_resp.device_id : "unknown");

		if (reg_resp.note && strlen(reg_resp.note) > 0) {
			LOG_INF("Server note: %s", reg_resp.note);
		}
	} else {
		LOG_ERR("Registration failed with status: %s",
			reg_resp.status ? reg_resp.status : "unknown");
		return -EIO;
	}

	return 0;
}

static int heartbeat_response_handler(void *ctx, uint8_t *payload, int len)
{
	struct discovery_heartbeat_response hb_resp;
	int ret;

	ret = json_obj_parse((char *)payload, len, heartbeat_response_descr,
			     ARRAY_SIZE(heartbeat_response_descr), &hb_resp);
	if (ret < 0) {
		LOG_ERR("Failed to parse heartbeat response: %d", ret);
		return ret;
	}

	if (strcmp(hb_resp.status, "heartbeat_received") == 0) {
		LOG_DBG("Heartbeat acknowledged by server");
	} else {
		LOG_WRN("Unexpected heartbeat response: %s", hb_resp.status);
	}

	return 0;
}

int narada_discovery_client_init(struct narada_discovery_client *client,
				 struct narada_client *narada_client)
{
	if (!client || !narada_client) {
		return -EINVAL;
	}

	memset(client, 0, sizeof(*client));
	client->narada_client = narada_client;
	client->server_port = NARADA_PORT;

	k_work_init_delayable(&client->heartbeat_work, heartbeat_work_handler);

	LOG_INF("Discovery client initialized - server discovery required");
	return 0;
}

int narada_discovery_discover_server(struct narada_discovery_client *client, k_timeout_t timeout)
{
	if (!client) {
		return -EINVAL;
	}

	uint8_t request_buf[JSON_BUFFER_SIZE];
	struct narada_request request;
	int ret;

	LOG_INF("Starting server discovery using .well-known/core multicast");

	client->server_discovered = false;

	ret = narada_multicast_req_init(&request, NARADA_WELL_KNOWN_CORE_PATH, request_buf, 0,
					server_discovery_handler, client);
	if (ret) {
		LOG_ERR("Failed to init discovery request: %d", ret);
		return ret;
	}

	ret = narada_multicast_get_req(client->narada_client, &request, timeout);
	if (ret) {
		LOG_ERR("Failed to send multicast discovery request: %d", ret);
		return ret;
	}

	if (client->server_discovered) {
		LOG_INF("Server discovery completed successfully");
		return 0;
	} else {
		LOG_WRN("No discovery server found within timeout");
		return -ETIMEDOUT;
	}
}

int narada_discovery_set_server_address(struct narada_discovery_client *client,
					const char *server_ip, uint16_t server_port)
{
	if (!client || !server_ip) {
		return -EINVAL;
	}

	strncpy(client->server_ip, server_ip, sizeof(client->server_ip) - 1);
	client->server_ip[sizeof(client->server_ip) - 1] = '\0';
	client->server_port = server_port;

	int ret = narada_client_set_ipv4(client->narada_client, server_ip, server_port);
	if (ret) {
		LOG_ERR("Failed to set server address: %d", ret);
		return ret;
	}

	LOG_INF("Discovery server address set to %s:%d", server_ip, server_port);
	return 0;
}

int narada_discovery_get_server_version(struct narada_discovery_client *client, uint32_t *major,
					uint32_t *minor)
{
	if (!client || !major || !minor) {
		return -EINVAL;
	}

	if (!client->server_discovered) {
		LOG_ERR("Server not discovered yet - call narada_discovery_discover_server first");
		return -ENOTCONN;
	}

	uint8_t request_buf[JSON_BUFFER_SIZE];
	struct narada_request request;
	struct version_response_context version_ctx = {.major = major, .minor = minor};
	int ret;

	ret = narada_req_init(&request, NARADA_DISCOVERY_VERSION_PATH, request_buf, 0,
			      version_response_handler, &version_ctx);
	if (ret) {
		LOG_ERR("Failed to init version request: %d", ret);
		return ret;
	}

	ret = narada_get_req_send(client->narada_client, &request);
	if (ret) {
		LOG_ERR("Failed to send version request: %d", ret);
		return ret;
	}

	LOG_DBG("Server version: %u.%u", *major, *minor);
	return 0;
}

int narada_discovery_register_device(struct narada_discovery_client *client)
{
	if (!client || !client->device->info.device_id) {
		return -EINVAL;
	}

	if (!client->server_discovered) {
		LOG_ERR("Server not discovered yet - call narada_discovery_discover_server first");
		return -ENOTCONN;
	}

	uint8_t request_buf[JSON_BUFFER_SIZE];
	char json_payload[JSON_BUFFER_SIZE];
	struct narada_request request;
	int ret;

	struct narada_device *device = client->device;

	ret = json_obj_encode_buf(register_request_descr, ARRAY_SIZE(register_request_descr),
				  &client->device->info, json_payload, sizeof(json_payload));
	if (ret) {
		LOG_ERR("Failed to encode registration request: %d", ret);
		return ret;
	}

	size_t payload_len = strlen(json_payload);
	if (payload_len >= JSON_BUFFER_SIZE) {
		LOG_ERR("Registration payload too large: %zu", payload_len);
		return -ENOMEM;
	}
	memcpy(request_buf, json_payload, payload_len);

	ret = narada_req_init(&request, NARADA_DISCOVERY_REGISTER_PATH, request_buf, payload_len,
			      register_response_handler, client);
	if (ret) {
		LOG_ERR("Failed to init registration request: %d", ret);
		return ret;
	}

	ret = narada_put_req_send(client->narada_client, &request);
	if (ret) {
		LOG_ERR("Failed to send registration request: %d", ret);
		return ret;
	}

	if (client->registered) {
		LOG_INF("Device registered successfully: %s", client->device->info.device_id);
	}

	return 0;
}

int narada_discovery_send_heartbeat(struct narada_discovery_client *client)
{
	if (!client || !client->registered) {
		return -EINVAL;
	}

	if (!client->server_discovered) {
		LOG_ERR("Server not discovered - cannot send heartbeat");
		return -ENOTCONN;
	}

	uint8_t request_buf[JSON_BUFFER_SIZE];
	char json_payload[JSON_BUFFER_SIZE];
	struct narada_request request;
	struct discovery_heartbeat_request hb_req;
	int ret;

	hb_req.device_id = client->device->info.device_id;

	/* Encode to JSON */
	ret = json_obj_encode_buf(heartbeat_request_descr, ARRAY_SIZE(heartbeat_request_descr),
				  &hb_req, json_payload, sizeof(json_payload));
	if (ret) {
		LOG_ERR("Failed to encode heartbeat request: %d", ret);
		return ret;
	}

	/* Copy JSON payload to request buffer */
	size_t payload_len = strlen(json_payload);
	if (payload_len >= JSON_BUFFER_SIZE) {
		LOG_ERR("Heartbeat payload too large: %zu", payload_len);
		return -ENOMEM;
	}
	memcpy(request_buf, json_payload, payload_len);

	ret = narada_req_init(&request, NARADA_DISCOVERY_HEARTBEAT_PATH, request_buf, payload_len,
			      heartbeat_response_handler, NULL);
	if (ret) {
		LOG_ERR("Failed to init heartbeat request: %d", ret);
		return ret;
	}

	ret = narada_put_req_send(client->narada_client, &request);
	if (ret) {
		LOG_ERR("Failed to send heartbeat request: %d", ret);
		return ret;
	}

	LOG_DBG("Heartbeat sent for device: %s", client->device->info.device_id);
	return 0;
}

int narada_discovery_start_heartbeat(struct narada_discovery_client *client)
{
	if (!client || !client->registered || client->device->info.heartbeat_interval == 0) {
		return -EINVAL;
	}

	/* Initialize heartbeat work */
	k_work_init_delayable(&client->heartbeat_work, heartbeat_work_handler);

	/* Schedule first heartbeat */
	k_work_schedule(&client->heartbeat_work,
			K_SECONDS(client->device->info.heartbeat_interval));

	LOG_INF("Heartbeat started with interval %d seconds",
		client->device->info.heartbeat_interval);
	return 0;
}

int narada_discovery_stop_heartbeat(struct narada_discovery_client *client)
{
	if (!client) {
		return -EINVAL;
	}

	k_work_cancel_delayable(&client->heartbeat_work);
	LOG_INF("Heartbeat stopped for device: %s", client->device->info.device_id);
	return 0;
}

/* Device tree integration */
#if DT_HAS_COMPAT_STATUS_OKAY(narada_discovery)

#define NARADA_DISCOVERY_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(narada_discovery)
#define NARADA_DEVICE_NODE    DT_PHANDLE(NARADA_DISCOVERY_NODE, device)

/* Static discovery client instance */
static struct narada_discovery_client dt_discovery_client = {
	.narada_client = &(*NARADA_GET_DEVICE(NARADA_DEVICE_NODE)).client,
	.device = NARADA_GET_DEVICE(NARADA_DEVICE_NODE),
};

struct narada_discovery_client *narada_discovery_dt_get_client(void)
{
	return &dt_discovery_client;
}

static void discovery_work_handler(struct k_work *work);

/* Discovery worker thread */
K_WORK_DELAYABLE_DEFINE(discovery_work, discovery_work_handler);
// static struct k_work_delayable discovery_work;
static bool discovery_active = false;

static void discovery_work_handler(struct k_work *work)
{
	struct narada_discovery_client *client = &dt_discovery_client;
	int ret;

	LOG_INF("Starting discovery process...");
	discovery_active = true;

	/* Step 1: Discover server using multicast .well-known/core */
	ret = narada_discovery_discover_server(client, K_SECONDS(5));
	if (ret) {
		LOG_ERR("Server discovery failed: %d", ret);
		discovery_active = false;
#ifdef CONFIG_NARADA_STATE
		narada_state_discovery_failed();
#endif
		return;
	}

	/* If server was discovered successfully, complete the discovery flow */
	if (client->server_discovered) {
		uint32_t major, minor;

		/* Get server version */
		ret = narada_discovery_get_server_version(client, &major, &minor);
		if (ret) {
			LOG_WRN("Failed to get server version: %d", ret);
			/* Continue anyway - version check is not critical */
		} else {
			LOG_INF("Discovery server version: %u.%u", major, minor);
		}

		/* Register device */
		ret = narada_discovery_register_device(client);
		if (ret) {
			LOG_ERR("Failed to register device: %d", ret);
			discovery_active = false;
#ifdef CONFIG_NARADA_STATE
			narada_state_discovery_failed();
#endif
			return;
		}

		/* Start heartbeat if configured */
		if (client->device->info.heartbeat_interval > 0) {
			ret = narada_discovery_start_heartbeat(client);
			if (ret) {
				LOG_WRN("Failed to start heartbeat: %d", ret);
				/* Continue anyway - heartbeat failure is not critical */
			}
		}

		discovery_active = false;
		LOG_INF("Discovery completed successfully with auto-captured server IP");

#ifdef CONFIG_NARADA_STATE
		narada_state_discovery_completed();
#endif
	} else {
		/* Discovery failed */
		discovery_active = false;
#ifdef CONFIG_NARADA_STATE
		narada_state_discovery_failed();
#endif
	}
}

int narada_discovery_start(void)
{
	if (discovery_active) {
		LOG_WRN("Discovery already in progress");
		return -EALREADY;
	}

	/* Schedule discovery work to run */
	k_work_schedule(&discovery_work, K_NO_WAIT);

	LOG_INF("Discovery process scheduled");
	return 0;
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(narada_discovery) */
