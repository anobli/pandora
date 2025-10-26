/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ESPHome);

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>

#include "service.h"
#include "srp.h"

static void ot_state_changed(otChangedFlags flags, void *data)
{
	struct otInstance *ot_instance = openthread_get_default_instance();
	const struct device *dev = data;

	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(ot_instance)) {
		case OT_DEVICE_ROLE_CHILD:
		case OT_DEVICE_ROLE_ROUTER:
		case OT_DEVICE_ROLE_LEADER:
			ot_srp_init(dev);
			break;
		default:
			break;
		}
	}
}

static struct openthread_state_changed_callback ot_state_changed_cb = {
	.otCallback = ot_state_changed,
};

int esphome_ot_init(const struct device *dev)
{
	struct openthread_context *ctx;
	int ret;

	static const struct device *const radio_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));

	ot_state_changed_cb.user_data = (void *)dev;
	ctx = openthread_get_default_context();
	if (!ctx || !device_is_ready(radio_dev)) {
		LOG_ERR("Failed to find OpenThread device");
		return -ENODEV;
	}

	openthread_state_changed_callback_register(&ot_state_changed_cb);

	return ret;
}
