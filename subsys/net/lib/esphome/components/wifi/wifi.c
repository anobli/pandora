/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nabucasa_esphome_wifi

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/logging/log.h>
#include <errno.h>

LOG_MODULE_REGISTER(ESPHome);

#include <esphome/esphome.h>

#include "wifi.h"
#include "wifi_credentials.h"

// static K_SEM_DEFINE(wifi_connected, 0, 1);
// static K_SEM_DEFINE(ipv4_address_obtained, 0, 1);

static void wifi_connect(const struct device *dev);

static void wifi_status(void)
{
	struct net_if *iface = net_if_get_default();

	struct wifi_iface_status status = {0};

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
		     sizeof(struct wifi_iface_status))) {
		LOG_ERR("WiFi Status Request Failed");
	}

	if (status.state >= WIFI_STATE_ASSOCIATED) {
		LOG_INF("SSID: %-32s\n", status.ssid);
		LOG_INF("Band: %s\n", wifi_band_txt(status.band));
		LOG_INF("Channel: %d\n", status.channel);
		LOG_INF("Security: %s\n", wifi_security_txt(status.security));
		LOG_INF("RSSI: %d\n", status.rssi);
	}
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	struct esphome_wifi_data *wifi_data = CONTAINER_OF(cb, struct esphome_wifi_data, event_cb);
	const struct esphome_wifi_config *wifi_cfg = wifi_data->dev->config;
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_ERR("Connection request failed (%d)\n", status->status);
		k_work_schedule(&wifi_data->work, K_MSEC(15000));
	} else {
		LOG_INF("Connected\n");
		wifi_status();
		if (wifi_cfg->on_connect) {
			wifi_cfg->on_connect();
		}
	}
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	struct esphome_wifi_data *wifi_data = CONTAINER_OF(cb, struct esphome_wifi_data, event_cb);
	const struct esphome_wifi_config *wifi_cfg = wifi_data->dev->config;
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_ERR("Disconnection request (%d)\n", status->status);
		/* Not sure why we get here when we fail to connect */
		k_work_schedule(&wifi_data->work, K_MSEC(15000));
	} else {
		if (wifi_cfg->on_disconnect) {
			wifi_cfg->on_disconnect();
		}
		LOG_INF("Disconnected\n");
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				    struct net_if *iface)
{
	switch (mgmt_event) {

	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;

	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;

	default:
		break;
	}
}

static void wifi_connect(const struct device *dev)
{
	struct esphome_wifi_data *wifi_data = dev->data;
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params wifi_params = {0};

	wifi_credentials_get_next(&wifi_params);
	LOG_INF("Connecting to SSID: %s\n", wifi_params.ssid);
	LOG_INF("Security: %s\n", wifi_security_txt(wifi_params.security));

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params,
		     sizeof(struct wifi_connect_req_params))) {
		LOG_ERR("WiFi Connection Request Failed\n");
		k_work_schedule(&wifi_data->work, K_MSEC(15000));
	}
}

void wifi_disconnect(void)
{
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0)) {
		LOG_ERR("WiFi Disconnection Request Failed\n");
	}
}

void wifi_connect_work_cb(struct k_work *work)
{
	struct k_work_delayable *work_delayable = CONTAINER_OF(work, struct k_work_delayable, work);
	struct esphome_wifi_data *wifi_data =
		CONTAINER_OF(work_delayable, struct esphome_wifi_data, work);

	wifi_disconnect();
	wifi_connect(wifi_data->dev);
}

int esphome_wifi_init(const struct device *dev)
{
	struct esphome_wifi_data *wifi_data = dev->data;

	k_work_init_delayable(&wifi_data->work, wifi_connect_work_cb);
	net_mgmt_init_event_callback(&wifi_data->event_cb, wifi_mgmt_event_handler,
				     NET_EVENT_WIFI_CONNECT_RESULT |
					     NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_data->event_cb);

	k_work_schedule(&wifi_data->work, K_MSEC(1000));

	return (0);
}

#define ESPHOME_WIFI_NETWORK(node_id)                                                              \
	{                                                                                          \
		.ssid = DT_PROP(node_id, ssid),                                                    \
		.password = DT_PROP(node_id, password),                                            \
	},

#define DEFINE_ESPHOME_WIFI(_num)                                                                  \
	COND_CODE_1(DT_NODE_HAS_PROP(_num, on_connect),                                            \
		    (extern int DT_STRING_TOKEN(DT_DRV_INST(_num), on_connect)()), ())             \
	COND_CODE_1(DT_NODE_HAS_PROP(_num, on_disconnect),                                         \
		    (extern int DT_STRING_TOKEN(DT_DRV_INST(_num), on_disconnect)()), ())          \
	COND_CODE_1(DT_NODE_HAS_PROP(_num, on_error),                                              \
		    (extern int DT_STRING_TOKEN(DT_DRV_INST(_num), on_error)()), ())               \
	static const struct esphome_wifi_config esphome_wifi_config_##_num = {                     \
		.on_connect =                                                                      \
			COND_CODE_1(DT_NODE_HAS_PROP(_num, on_connect),                            \
				    (DT_STRING_TOKEN(DT_DRV_INST(_num), on_connect)), (NULL)),     \
		.on_disconnect =                                                                   \
			COND_CODE_1(DT_NODE_HAS_PROP(_num, on_disconnect),                         \
				    (DT_STRING_TOKEN(DT_DRV_INST(_num), on_disconnect)), (NULL)),  \
		.on_error = COND_CODE_1(DT_NODE_HAS_PROP(_num, on_errort),                         \
					(DT_STRING_TOKEN(DT_DRV_INST(_num), on_error)), (NULL)),   \
	};                                                                                         \
	static struct esphome_wifi_data esphome_wifi_data_##_num = {                               \
		.dev = DEVICE_DT_INST_GET(_num),                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_num, esphome_wifi_init, NULL, &esphome_wifi_data_##_num,            \
			      &esphome_wifi_config_##_num, POST_KERNEL,                            \
			      CONFIG_ESPHOME_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_ESPHOME_WIFI);
