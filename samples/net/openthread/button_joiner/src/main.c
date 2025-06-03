/*
 * Copyright (c) 2025 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/led.h>
#include <zephyr/net/openthread.h>
#include <zephyr/version.h>

#include <openthread/thread.h>

#include <zephyr/sys/reboot.h>

#define OT_JOINER_PSKD "J01NME"
#if defined(CONFIG_OPENTHREAD_PLATFORM_INFO)
#define OT_PLATFORM_INFO CONFIG_OPENTHREAD_PLATFORM_INFO
#else
#define OT_PLATFORM_INFO ""
#endif
#define ZEPHYR_PACKAGE_NAME "Zephyr"
#define PACKAGE_VERSION     KERNEL_VERSION_STRING

static const struct device *leds = DEVICE_DT_GET_OR_NULL(DT_INST(0, gpio_leds));

#ifdef CONFIG_OPENTHREAD_COMMISSIONER

void ot_commissioner_state_changed(otCommissionerState aState, void *user_data)
{
	struct otInstance *ot_instance = openthread_get_default_instance();
	int ret;

	if (aState == OT_COMMISSIONER_STATE_ACTIVE) {
		ret = otCommissionerAddJoiner(ot_instance, NULL, OT_JOINER_PSKD, 120);
		if (ret != OT_ERROR_NONE) {
			printf("Failed to add a joiner: %d\n", ret);
			led_blink(leds, 0, 200, 200);
		} else {
			led_blink(leds, 0, 1000, 1000);
		}
	} else if (aState == OT_COMMISSIONER_STATE_PETITION) {
		led_blink(leds, 0, 500, 500);
	} else {
		led_off(leds, 0);
	}
}

void joinerCallback(otCommissionerJoinerEvent aEvent, const otJoinerInfo *aJoinerInfo,
		    const otExtAddress *aJoinerId, void *aContext)
{
	(void)aContext;
	struct otInstance *ot_instance = openthread_get_default_instance();

	switch (aEvent) {
	case OT_COMMISSIONER_JOINER_END:
	case OT_COMMISSIONER_JOINER_REMOVED:
		otCommissionerStop(ot_instance);
	default:
	}
}

static void ot_commissioner_work_handler(struct k_work *work)
{
	struct otInstance *ot_instance = openthread_get_default_instance();
	int ret;

	ret = otCommissionerStart(ot_instance, ot_commissioner_state_changed, joinerCallback, NULL);
	if (ret != OT_ERROR_NONE) {
		printf("Failed to start commissioner\n");
		led_blink(leds, 0, 200, 200);
	}
}

K_WORK_DEFINE(ot_commissioner_work, ot_commissioner_work_handler);

#endif

#ifdef CONFIG_OPENTHREAD_JOINER

K_MUTEX_DEFINE(joiner_mutex);
K_CONDVAR_DEFINE(joiner_condvar);

static void ot_joiner_start_handler(otError error, void *context)
{
	struct otInstance *ot_instance = openthread_get_default_instance();

	k_mutex_lock(&joiner_mutex, K_FOREVER);
	switch (error) {
	case OT_ERROR_NONE:
		printf("Join success\n");
		otThreadSetEnabled(ot_instance, true);
		break;
	default:
		printf("Join failed [%d]\n", error);
		if (error != OT_ERROR_NOT_FOUND) {
			led_blink(leds, 0, 500, 500);
		}
		k_condvar_signal(&joiner_condvar);
		break;
	}
	k_mutex_unlock(&joiner_mutex);
}

static void ot_joiner_thread(void *, void *, void *)
{
	struct openthread_context *ot_context = openthread_get_default_context();
	struct otInstance *ot_instance = openthread_get_default_instance();
	otError error = OT_ERROR_NONE;

	printf("Joining a Thread network!\n");
	error = otThreadSetEnabled(ot_instance, false);
	if (error != OT_ERROR_NONE) {
		printf("Failed to stop the OpenThread network [%d]\n", error);
	}

	printf("Set broadcast PANID\n");
	otLinkSetPanId(ot_instance, OT_PANID_BROADCAST);

	k_mutex_lock(&joiner_mutex, K_FOREVER);
	do {
		printf("Starting OpenThread join procedure.\n");
		error = otJoinerStart(ot_instance, OT_JOINER_PSKD, NULL, ZEPHYR_PACKAGE_NAME,
				      OT_PLATFORM_INFO, PACKAGE_VERSION, NULL,
				      &ot_joiner_start_handler, ot_context);
		if (error != OT_ERROR_NONE) {
			printf("Failed to start joiner [%d]\n", error);
			led_blink(leds, 0, 200, 200);
		} else {
			k_condvar_wait(&joiner_condvar, &joiner_mutex, K_FOREVER);
		}
	} while (!otDatasetIsCommissioned(ot_instance));
	printf("Done!\n");
	led_on(leds, 0);
	k_mutex_unlock(&joiner_mutex);
}

static void ot_state_changed(otChangedFlags flags, void *user_data)
{
	if (flags & OT_CHANGED_PENDING_DATASET) {
		k_mutex_lock(&joiner_mutex, K_FOREVER);
		k_condvar_signal(&joiner_condvar);
		k_mutex_unlock(&joiner_mutex);
	}
}

static struct openthread_state_changed_callback ot_state_changed_cb = {
	.otCallback = ot_state_changed,
};

#define JOINER_STACK_SIZE 2048
#define JOINER_PRIORITY   -1

K_THREAD_DEFINE(joiner_tid, JOINER_STACK_SIZE, ot_joiner_thread, NULL, NULL, NULL, JOINER_PRIORITY,
		0, -1);
#endif

static void button_pressed(struct input_event *event, void *user_data)
{
	if (!event->value) {
		return;
	}

	if (event->code == INPUT_KEY_X) {
#ifdef CONFIG_OPENTHREAD_JOINER
		struct otInstance *ot_instance = openthread_get_default_instance();

		printf("Factory Reset!\n");
		otInstanceFactoryReset(ot_instance);
#endif
#ifdef CONFIG_OPENTHREAD_COMMISSIONER
		k_work_submit(&ot_commissioner_work);
#endif
	}
}

INPUT_CALLBACK_DEFINE(NULL, button_pressed, NULL);

int main(void)
{
	if (!device_is_ready(leds)) {
		printf("Device %s is not ready", leds->name);
		return -ENODEV;
	}

#ifdef CONFIG_OPENTHREAD_JOINER
	struct otInstance *ot_instance = openthread_get_default_instance();

	if (!otDatasetIsCommissioned(ot_instance)) {
		led_blink(leds, 0, 1000, 1000);
		k_thread_start(joiner_tid);
	} else {
		led_on(leds, 0);
	}

	openthread_state_changed_callback_register(&ot_state_changed_cb);
#endif

	return 0;
}
