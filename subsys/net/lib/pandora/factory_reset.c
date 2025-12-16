/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/input/input.h>
#include <zephyr/sys/reboot.h>
#include <pandora/settings.h>

#ifdef CONFIG_OPENTHREAD
#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(Pandora);

#define FACTORY_RESET_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(pandora_factory_reset)

static void factory_reset_handler(struct k_work *work)
{
	LOG_INF("Factory Reset!\n");

	pandora_settings_erase_all();

	/*
	 * Must be kept at the end!
	 * From here, whatever we use OpenThread or not,
	 * device is going to reboot.
	 */
	/* FIXME: Understand why reboot hangs without this */
	k_sleep(K_SECONDS(1));
#ifdef CONFIG_OPENTHREAD
	struct otInstance *ot_instance = openthread_get_default_instance();

	otInstanceFactoryReset(ot_instance);
#else
	sys_reboot(SYS_REBOOT_COLD);
#endif
}
static K_WORK_DEFINE(factory_reset_work, factory_reset_handler);

static void button_pressed(struct input_event *event, void *user_data)
{
	if (!event->value) {
		return;
	}

	if (event->code == DT_PROP(FACTORY_RESET_NODE, input_code)) {
		k_work_submit(&factory_reset_work);
	}
}

INPUT_CALLBACK_DEFINE(NULL, button_pressed, NULL);
