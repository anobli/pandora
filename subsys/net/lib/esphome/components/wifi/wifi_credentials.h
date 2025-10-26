/*
 * Copyright (c) 2025 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESPHOME_WIFI_CREDENTIALS_H
#define ESPHOME_WIFI_CREDENTIALS_H

#include <zephyr/net/wifi_mgmt.h>

void wifi_credentials_get_current(struct wifi_connect_req_params *params);
void wifi_credentials_get_next(struct wifi_connect_req_params *params);

#endif /* ESPHOME_WIFI_CREDENTIALS_H */
