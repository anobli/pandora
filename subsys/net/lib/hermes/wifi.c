/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/data/json.h>

#include <pandora/events.h>
#include <pandora/settings.h>
#include <pandora/wifi.h>

#include <hermes/hermes.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(hermes, CONFIG_HERMES_LOG_LEVEL);

#define NET_EVENT_WIFI_MASK (NET_EVENT_WIFI_AP_STA_CONNECTED | NET_EVENT_WIFI_AP_STA_DISCONNECTED)

static struct net_mgmt_event_callback cb;

const struct json_obj_descr json_wifi_credentials_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct wifi_credentials, ssid, JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM(struct wifi_credentials, password, JSON_TOK_STRING_BUF),
};
const int json_wifi_credentials_descr_size = ARRAY_SIZE(json_wifi_credentials_descr);

static void hermes_wifi_handler_put_credentials(struct hermes_resource *rsc, const void *data,
						uint16_t len)
{
	struct wifi_credentials credentials;

	json_obj_parse((void *)data, len, json_wifi_credentials_descr,
		       json_wifi_credentials_descr_size, &credentials);
	pandora_wifi_set_credentials(&credentials);
}

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			       struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_AP_STA_CONNECTED: {
		/* Save settings once we know they are really working */
		pandora_wifi_settings_save(NULL);

		hermes_server_start();
		break;
	}
	case NET_EVENT_WIFI_AP_STA_DISCONNECTED: {
		hermes_server_stop();
		break;
	}
	default:
		break;
	}
}

static int hermes_wifi_init(void)
{
	net_mgmt_init_event_callback(&cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
	net_mgmt_add_event_callback(&cb);

	return 0;
}

#define DEFINE_HERMES_WIFI_EP(_ep)                                                                 \
	HERMES_RESOURCE_DEFINE_DOMAIN(NULL, wifi, _ep, NULL, hermes_wifi_handler_put_##_ep, NULL);

#define DEFINE_HERMES_WIFI() DEFINE_HERMES_WIFI_EP(credentials);

DEFINE_HERMES_WIFI();
SYS_INIT(hermes_wifi_init, APPLICATION, 0);
