/*
 * Copyright (c) 2025 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESPHOME_WIFI_H
#define ESPHOME_WIFI_H

#include <zephyr/device.h>
#include <zephyr/net/wifi_mgmt.h>

typedef void (*esphome_wifi_on_connect)(void);
typedef void (*esphome_wifi_on_disconnect)(void);
typedef void (*esphome_wifi_on_error)(void);

struct esphome_wifi_network_config {
	const char *ssid;
	const char *password;
	bool save;
	uint32_t timeout;

	esphome_wifi_on_connect on_connect;
	esphome_wifi_on_error on_error;
};

struct esphome_wifi_config {
	esphome_wifi_on_connect on_connect;
	esphome_wifi_on_disconnect on_disconnect;
	esphome_wifi_on_error on_error;
};

struct esphome_wifi_data {
	const struct device *dev;
	struct net_mgmt_event_callback event_cb;
	int current_network;

	struct k_work_delayable work;
};

int esphome_wifi_enable(const struct device *dev);
int esphome_wifi_disable(const struct device *dev);
int esphome_wifi_configure(const struct device *dev, struct esphome_wifi_network_config *cfg);

bool esphome_wifi_is_enabled(const struct device *dev);
bool esphome_wifi_is_connected(const struct device *dev);

int esphome_wifi_init(const struct device *dev);

#endif /* ESPHOME_WIFI_H */
