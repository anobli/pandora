/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#ifndef PANDORA_WIFI_H
#define PANDORA_WIFI_H

#define SSID_LEN_MAX 64
#define PSK_LEN_MAX  256

struct wifi_credentials {
	char ssid[SSID_LEN_MAX];
	char password[PSK_LEN_MAX];
};

int pandora_wifi_init();
int pandora_wifi_try_connect();
void pandora_wifi_set_credentials(struct wifi_credentials *credentials);
int pandora_wifi_settings_save(const struct device *dev);

#endif /* PANDORA_WIFI_H */
