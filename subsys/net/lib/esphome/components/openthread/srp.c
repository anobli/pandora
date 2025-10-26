/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ESPHome);

#include <esphome/esphome.h>

#include <openthread/link.h>
#include "srp.h"

static const char *SRP_INSTANCE_NAME = "ot-esphome";
static const char *SRP_SERVICE_NAME = "_esphomelib._tcp";

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
	size = MIN(size, strlen(cfg->name) + 1);
	memcpy(host_name, cfg->name, size);

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

	error = otSrpClientAddService(ot, &entry->mService);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to register SRP service: %s", otThreadErrorToString(error));
		return -1;
	}

	otSrpClientEnableAutoStartMode(ot, NULL, NULL);

	ot_srp_init_done = true;

	return 0;
}
