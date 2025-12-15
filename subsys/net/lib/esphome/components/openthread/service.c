/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ESPHomeOT);

#include <zephyr/net/openthread.h>
#include <zephyr/version.h>

#include <openthread/thread.h>

#include "service.h"
#include "srp.h"

#define OPENTHREAD_JOINER_STACK_SIZE 2048
#define OPENTHREAD_JOINER_PRIORITY   5

#ifdef CONFIG_OPENTHREAD_JOINER_AUTOSTART

K_THREAD_STACK_DEFINE(openthread_joiner_stack_area, OPENTHREAD_JOINER_STACK_SIZE);
struct k_thread openthread_joiner_data;

static void ot_joiner_thread(void *arg0, void *arg1, void *arg2)
{
	struct otInstance *ot_instance = openthread_get_default_instance();
	int ret;

	/*
	 * Restart OpenThread until a network is commissioned.
	 * When CONFIG_OPENTHREAD_JOINER_AUTOSTART is enabled,
	 * openthread_run automatically tries to join a network.
	 * But, if the commissioner is not ready, network is not found
	 * and openthread doesn't start until we reboot device.
	 * This gets devices state and re-run openthread_run if the device
	 * is not commissioned.
	 *
	 * Note:
	 * I tried to otJoinserStart without success.
	 * So far, this is the most reliable way I found to join automatically a network.
	 */
	do {
		if (otJoinerGetState(ot_instance) != OT_JOINER_STATE_IDLE) {
			k_sleep(K_SECONDS(1));
			continue;
		}

		if (otDatasetIsCommissioned(ot_instance)) {
			return;
		}

		ret = openthread_run();
		if (ret != OT_ERROR_NONE) {
			k_sleep(K_SECONDS(1));
		}
	} while (1);
}
#endif

static void ot_state_changed(otChangedFlags flags, void *data)
{
	struct otInstance *ot_instance = openthread_get_default_instance();

	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(ot_instance)) {
		case OT_DEVICE_ROLE_CHILD:
		case OT_DEVICE_ROLE_ROUTER:
		case OT_DEVICE_ROLE_LEADER:
			ot_srp_init();
			break;
		default:
			break;
		}
	}
}

static struct openthread_state_changed_callback ot_state_changed_cb = {
	.otCallback = ot_state_changed,
};

int esphome_ot_init()
{
	struct openthread_context *ctx;
	int ret;

	static const struct device *const radio_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));

	ctx = openthread_get_default_context();
	if (!ctx || !device_is_ready(radio_dev)) {
		LOG_ERR("Failed to find OpenThread device");
		return -ENODEV;
	}

	openthread_state_changed_callback_register(&ot_state_changed_cb);

#ifdef CONFIG_OPENTHREAD_JOINER_AUTOSTART
	k_thread_create(&openthread_joiner_data, openthread_joiner_stack_area,
			K_THREAD_STACK_SIZEOF(openthread_joiner_stack_area), ot_joiner_thread, NULL,
			NULL, NULL, OPENTHREAD_JOINER_PRIORITY, 0, K_SECONDS(1));
#endif

	return ret;
}
