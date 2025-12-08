/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ESPHomeOT);

#include <esphome/esphome.h>
#include <esphome/components/api.h>

#include <openthread/link.h>
#include "srp.h"

#define API_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nabucasa_esphome_api)

static const char *SRP_SERVICE_NAME = "_esphomelib._tcp";
static const char friendly_name[] = DT_PROP_OR(DT_PARENT(API_NODE), friendly_name, "");
static bool ot_srp_init_done = false;

void ot_srp_callback(otError aError, const otSrpClientHostInfo *aHostInfo,
		     const otSrpClientService *aServices,
		     const otSrpClientService *aRemovedServices, void *aContext)
{
	if (aError != OT_ERROR_NONE) {
		LOG_ERR("SRP update error: %s", otThreadErrorToString(aError));
	}
	LOG_INF("SRP update registered");
}

static char mac[MAC_ADDRESS_LEN];
otDnsTxtEntry txtEntries[] = {
	{.mKey = "version", .mValue = (const uint8_t *)"1.0", .mValueLength = 3},
	{.mKey = "board",
	 .mValue = (const uint8_t *)CONFIG_BOARD,
	 .mValueLength = sizeof(CONFIG_BOARD)},
	{.mKey = "mac", .mValue = (const uint8_t *)mac, .mValueLength = sizeof(MAC_ADDRESS_LEN)},
	{.mKey = "friendly_name",
	 .mValue = (const uint8_t *)friendly_name,
	 .mValueLength = sizeof(friendly_name)},
};

int ot_srp_init(const struct device *dev)
{
	otError error;
	otInstance *ot;
	otSrpClientBuffersServiceEntry *entry;
	char *host_name;
	char *instance_name;
	char *service_name;
	uint16_t size;

	const struct esphome_config *cfg = dev->config;

	if (ot_srp_init_done) {
		return 0;
	}

	LOG_INF("Initializing SRP client");

	ot = openthread_get_default_instance();
	if (!ot) {
		LOG_ERR("Failed to get an OpenThread instance");
		return -ENODEV;
	}

	otSrpClientSetCallback(ot, ot_srp_callback, NULL);
	host_name = otSrpClientBuffersGetHostNameString(ot, &size);
	get_unique_device_name(cfg->name, host_name, size);

	error = otSrpClientSetHostName(ot, host_name);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set SRP host name: %s", otThreadErrorToString(error));
		return -1;
	}

	error = otSrpClientEnableAutoHostAddress(ot);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set SRP host address: %s", otThreadErrorToString(error));
		return -1;
	}

	entry = otSrpClientBuffersAllocateService(ot);
	if (entry == NULL) {
		LOG_ERR("Failed to allocate SRP service: %s", otThreadErrorToString(error));
		return -1;
	}
	entry->mService.mPort = cfg->port;

	instance_name = otSrpClientBuffersGetServiceEntryInstanceNameString(entry, &size);
	size = MIN(size, strlen(cfg->name) + 1);
	memcpy(instance_name, cfg->name, size);

	service_name = otSrpClientBuffersGetServiceEntryServiceNameString(entry, &size);
	size = MIN(size, strlen(SRP_SERVICE_NAME) + 1);
	memcpy(service_name, SRP_SERVICE_NAME, size);

	get_mac_address_string(mac, MAC_ADDRESS_LEN);
	for (int i = 0; i < ARRAY_SIZE(txtEntries); i++) {
		txtEntries[i].mValueLength = strlen(txtEntries[i].mValue);
	}
	entry->mService.mNumTxtEntries = ARRAY_SIZE(txtEntries);
	entry->mService.mTxtEntries = txtEntries;

	error = otSrpClientAddService(ot, &entry->mService);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to register SRP service: %s", otThreadErrorToString(error));
		return -1;
	}

	otSrpClientEnableAutoStartMode(ot, NULL, NULL);

	ot_srp_init_done = true;

	LOG_INF("SRP client initialized");

	return 0;
}
