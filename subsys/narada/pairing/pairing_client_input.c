#include <zephyr/kernel.h>
#include <zephyr/input/input.h>

#include <narada/narada.h>
#include <narada/pairing/client.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(narada, CONFIG_NARADA_LOG_LEVEL);

#define DT_DRV_COMPAT narada_pairing_client_input

#define NARADA_PAIRING_CLIENT_INPUT(inst)                                                          \
	static void narada_pairing_work_handler(struct k_work *work)                               \
	{                                                                                          \
		struct narada_device *dev = NARADA_GET_DEVICE(DT_INST_PROP(inst, device));         \
                                                                                                   \
		narada_pairing_request(&dev->client, dev->device_type);                            \
	}                                                                                          \
                                                                                                   \
	K_WORK_DEFINE(narada_pairing_work_##inst, narada_pairing_work_handler);                    \
	static void button_pressed_##inst(struct input_event *event, void *user_data)              \
	{                                                                                          \
		if (!event->value) {                                                               \
			return;                                                                    \
		}                                                                                  \
                                                                                                   \
		if (event->code == DT_INST_PROP(inst, input_code)) {                               \
			k_work_submit(&narada_pairing_work_##inst);                                \
		}                                                                                  \
	}                                                                                          \
	INPUT_CALLBACK_DEFINE(NULL, button_pressed_##inst, NULL);

DT_INST_FOREACH_STATUS_OKAY(NARADA_PAIRING_CLIENT_INPUT)
