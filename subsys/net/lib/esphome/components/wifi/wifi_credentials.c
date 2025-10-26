/*
 * Copyright (c) 2025 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nabucasa_wifi_credentials

#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util_internal.h>

#include "wifi_credentials.h"

struct wifi_credentials_config {
	const struct wifi_connect_req_params *credentials;
	uint8_t count;
};

struct wifi_credentials_data {
	uint8_t selected;
};

#define WIFI_SECURITY_TYPE(node_id)                                                                \
	CONCAT(WIFI_SECURITY_TYPE_, UTIL_EXPAND(DT_STRING_UPPER_TOKEN(node_id, security)))

#define WIFI_CREDENTIALS(node_id)                                                                  \
	{                                                                                          \
		.ssid = DT_PROP(node_id, ssid),                                                    \
		.ssid_length = sizeof(DT_PROP(node_id, ssid)),                                     \
		.psk = DT_PROP(node_id, password),                                                 \
		.psk_length = sizeof(DT_PROP(node_id, password)),                                  \
		.security = WIFI_SECURITY_TYPE(node_id),                                           \
		.channel = WIFI_CHANNEL_ANY,                                                       \
		.band = WIFI_FREQ_BAND_2_4_GHZ,                                                    \
	},

const static struct wifi_connect_req_params wifi_credentials[] = {
	DT_FOREACH_CHILD(DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT), WIFI_CREDENTIALS)};
static struct wifi_credentials_config wifi_cfg = {
	.credentials = wifi_credentials,
	.count = ARRAY_SIZE(wifi_credentials),
};
static struct wifi_credentials_data wifi_data;

void wifi_credentials_get_current(struct wifi_connect_req_params *params)
{
	memcpy(params, &wifi_cfg.credentials[wifi_data.selected], sizeof(*params));
}

void wifi_credentials_get_next(struct wifi_connect_req_params *params)
{
	int credential_id;

	credential_id = wifi_data.selected++;
	if (wifi_data.selected >= wifi_cfg.count) {
		wifi_data.selected = 0;
	}

	memcpy(params, &wifi_cfg.credentials[credential_id], sizeof(*params));
}
