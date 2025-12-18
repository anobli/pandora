/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>

#include <pandora/events.h>
#include <pandora/settings.h>
#include <pandora/wifi.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(Pandora, CONFIG_PANDORA_LOG_LEVEL);

#define NET_EVENT_WIFI_MASK                                                                        \
	(NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT |                        \
	 NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT)

/* AP Mode Configuration */
#define WIFI_AP_SSID       "ESP32-AP"
#define WIFI_AP_PSK        ""
#define WIFI_AP_IP_ADDRESS "192.168.4.1"
#define WIFI_AP_NETMASK    "255.255.255.0"

static struct net_if *ap_iface;
static struct net_if *sta_iface;

static struct wifi_connect_req_params ap_config;
static struct wifi_connect_req_params sta_config;

static struct net_mgmt_event_callback cb;
static struct wifi_credentials credentials;

static void start_dhcpv4_client(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	LOG_INF("Start on %s: index=%d", net_if_get_device(iface)->name,
		net_if_get_by_iface(iface));
	net_dhcpv4_start(iface);
}

static void stop_dhcpv4_client(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	LOG_INF("Stop on %s: index=%d", net_if_get_device(iface)->name, net_if_get_by_iface(iface));
	net_dhcpv4_stop(iface);
}

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			       struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT: {
		LOG_INF("Connected to %s", credentials.ssid);
		net_if_foreach(start_dhcpv4_client, NULL);
		break;
	}
	case NET_EVENT_WIFI_DISCONNECT_RESULT: {
		net_if_foreach(stop_dhcpv4_client, NULL);
		LOG_INF("Disconnected from %s", credentials.ssid);
		pandora_event_post_disconnected();
		break;
	}
	case NET_EVENT_WIFI_AP_ENABLE_RESULT: {
		LOG_INF("AP Mode is enabled. Waiting for station to connect");
		break;
	}
	case NET_EVENT_WIFI_AP_DISABLE_RESULT: {
		LOG_INF("AP Mode is disabled.");
		break;
	}
	default:
		break;
	}
}

static void enable_dhcpv4_server(void)
{
	static struct in_addr addr;
	static struct in_addr netmaskAddr;

	if (net_addr_pton(AF_INET, WIFI_AP_IP_ADDRESS, &addr)) {
		LOG_ERR("Invalid address: %s", WIFI_AP_IP_ADDRESS);
		return;
	}

	if (net_addr_pton(AF_INET, WIFI_AP_NETMASK, &netmaskAddr)) {
		LOG_ERR("Invalid netmask: %s", WIFI_AP_NETMASK);
		return;
	}

	net_if_ipv4_set_gw(ap_iface, &addr);

	if (net_if_ipv4_addr_add(ap_iface, &addr, NET_ADDR_MANUAL, 0) == NULL) {
		LOG_ERR("unable to set IP address for AP interface");
	}

	if (!net_if_ipv4_set_netmask_by_addr(ap_iface, &addr, &netmaskAddr)) {
		LOG_ERR("Unable to set netmask for AP interface: %s", WIFI_AP_NETMASK);
	}

	addr.s4_addr[3] += 10; /* Starting IPv4 address for DHCPv4 address pool. */

	if (net_dhcpv4_server_start(ap_iface, &addr) != 0) {
		LOG_ERR("DHCP server is not started for desired IP");
		return;
	}

	LOG_INF("DHCPv4 server started...\n");
}

static void disable_dhcpv4_server(void)
{
	if (net_dhcpv4_server_stop(ap_iface) != 0) {
		return;
	}

	LOG_INF("DHCPv4 server stopped...\n");
}

static int enable_ap_mode(void)
{
	if (!ap_iface) {
		LOG_INF("AP: is not initialized");
		return -EIO;
	}

	LOG_INF("Turning on AP Mode");
	ap_config.ssid = (const uint8_t *)WIFI_AP_SSID;
	ap_config.ssid_length = strlen(WIFI_AP_SSID);
	ap_config.psk = (const uint8_t *)WIFI_AP_PSK;
	ap_config.psk_length = strlen(WIFI_AP_PSK);
	ap_config.channel = WIFI_CHANNEL_ANY;
	ap_config.band = WIFI_FREQ_BAND_2_4_GHZ;

	if (strlen(WIFI_AP_PSK) == 0) {
		ap_config.security = WIFI_SECURITY_TYPE_NONE;
	} else {

		ap_config.security = WIFI_SECURITY_TYPE_PSK;
	}

	enable_dhcpv4_server();

	int ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, ap_iface, &ap_config,
			   sizeof(struct wifi_connect_req_params));
	if (ret) {
		LOG_ERR("NET_REQUEST_WIFI_AP_ENABLE failed, err: %d", ret);
	}

	return ret;
}

static int disable_ap_mode(void)
{
	if (!ap_iface) {
		LOG_INF("AP: is not initialized");
		return -EIO;
	}

	LOG_INF("Turning off AP Mode");

	disable_dhcpv4_server();

	int ret = net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, ap_iface, NULL, 0);
	if (ret) {
		LOG_ERR("NET_REQUEST_WIFI_AP_DISABLE failed, err: %d", ret);
	}

	return ret;
}

static int connect_to_wifi(void)
{
	if (!sta_iface) {
		LOG_INF("STA: interface no initialized");
		return -EIO;
	}

	sta_config.ssid = (const uint8_t *)credentials.ssid;
	sta_config.ssid_length = strlen(credentials.ssid);
	sta_config.psk = (const uint8_t *)credentials.password;
	sta_config.psk_length = strlen(credentials.password);
	/* TODO: make it configurable */
	sta_config.security = WIFI_SECURITY_TYPE_PSK;
	sta_config.channel = WIFI_CHANNEL_ANY;
	sta_config.band = WIFI_FREQ_BAND_2_4_GHZ;

	LOG_INF("Connecting to SSID: %s, %s\n", sta_config.ssid, sta_config.psk);

	int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, sta_iface, &sta_config,
			   sizeof(struct wifi_connect_req_params));
	if (ret) {
		LOG_ERR("Unable to Connect to (%s)", sta_config.ssid);
	}

	return ret;
}

static void wifi_work_handler(struct k_work *work);

/* Discovery worker thread */
K_WORK_DELAYABLE_DEFINE(wifi_work, wifi_work_handler);

static void wifi_work_handler(struct k_work *work)
{
	disable_ap_mode();
	connect_to_wifi();
}

static int pandora_wifi_settings_load(const struct device *dev)
{

	pandora_settings_load_one("wifi", "ssid", credentials.ssid, SSID_LEN_MAX);
	pandora_settings_load_one("wifi", "password", credentials.password, PSK_LEN_MAX);

	return 0;
}

int pandora_wifi_settings_save(const struct device *dev)
{
	pandora_settings_save_one("wifi", "ssid", credentials.ssid, SSID_LEN_MAX);
	pandora_settings_save_one("wifi", "password", credentials.password, PSK_LEN_MAX);

	return 0;
}

static int pandora_wifi_settings_erase(const struct device *dev)
{
	pandora_settings_erase_one("wifi", "ssid");
	pandora_settings_erase_one("wifi", "password");

	return 0;
}

int pandora_wifi_init()
{
	net_mgmt_init_event_callback(&cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
	net_mgmt_add_event_callback(&cb);

	/* Get AP interface in AP-STA mode. */
	ap_iface = net_if_get_wifi_sap();

	/* Get STA interface in AP-STA mode. */
	sta_iface = net_if_get_wifi_sta();

	return 0;
}

int pandora_wifi_try_connect()
{
	if (strlen(credentials.ssid) && strlen(credentials.password)) {
		k_work_schedule(&wifi_work, K_NO_WAIT);
	} else {
		enable_ap_mode();
	}

	return 0;
}

void pandora_wifi_set_credentials(struct wifi_credentials *new_credentials)
{
	memcpy(&credentials, new_credentials, sizeof(*new_credentials));
	k_work_schedule(&wifi_work, K_MSEC(1000));
}

PANDORA_SETTINGS(wifi, NULL, pandora_wifi_settings_load, pandora_wifi_settings_save,
		 pandora_wifi_settings_erase);
