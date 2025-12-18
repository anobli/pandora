/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>

#include <pandora/events.h>
#include <pandora/wifi.h>
#include <pandora/native_sim.h>

#include <zephyr/smf.h>

#ifdef CONFIG_HERMES
#include <hermes/hermes.h>
#include <hermes/discovery.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(Pandora, CONFIG_PANDORA_LOG_LEVEL);

#define PANDORA_NET_EVENTS (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

#ifdef CONFIG_LED
#define LED_STATUS_0 0
static const struct device *status_led = DEVICE_DT_GET_OR_NULL(DT_INST(0, gpio_leds));
#endif

enum pandora_state {
	PANDORA_STATE_INIT,
	PANDORA_STATE_DISCONNECTED,
	PANDORA_STATE_DISCOVERING,
	PANDORA_STATE_RUNNING,
};

struct pandora_state_ctx {
	struct smf_ctx ctx;

	/* Events */
	int32_t events;
};

const struct smf_state pandora_states[];
static struct pandora_state_ctx pandora_state_ctx;

static void pandora_running_entry(void *obj)
{
	LOG_INF("Device is connected");
#ifdef CONFIG_LED
	if (status_led) {
		led_on(status_led, LED_STATUS_0);
	}
#endif

#ifdef CONFIG_HERMES_SERVER
	int ret;

	ret = hermes_server_start();
	if (ret) {
		smf_set_terminate(SMF_CTX(obj), ret);
	}
#endif
}

static enum smf_state_result pandora_running_run(void *obj)
{
	struct pandora_state_ctx *ctx = obj;

	if (ctx->events & PANDORA_EVENT_DISCONNECTED) {
		smf_set_state(SMF_CTX(obj), &pandora_states[PANDORA_STATE_DISCONNECTED]);
	}

	return SMF_EVENT_HANDLED;
}

static void pandora_running_exit(void *obj)
{
#ifdef CONFIG_LED
	if (status_led) {
		led_off(status_led, LED_STATUS_0);
	}
#endif
}

static void pandora_init_entry(void *obj)
{
	LOG_INF("Device is starting");

#ifdef CONFIG_HERMES
	hermes_init();
	hermes_devices_init();
#endif

#ifdef CONFIG_WIFI
	pandora_wifi_init();
	pandora_wifi_try_connect();
#endif
#ifdef CONFIG_BOARD_NATIVE_SIM
	pandora_native_net_init();
#endif
}

static void pandora_disconnected_entry(void *obj)
{
	LOG_INF("Device is disconnected");
#ifdef CONFIG_LED
	if (status_led) {
		led_blink(status_led, LED_STATUS_0, 500, 500);
	}
#endif

#ifdef CONFIG_HERMES_SERVER
	int ret;

	ret = hermes_server_stop();
	if (ret) {
		smf_set_terminate(SMF_CTX(obj), ret);
	}
#endif
}

static enum smf_state_result pandora_disconnected_run(void *obj)
{
	struct pandora_state_ctx *ctx = obj;

	if (ctx->events & PANDORA_EVENT_CONNECTED) {
		smf_set_state(SMF_CTX(obj), &pandora_states[PANDORA_STATE_DISCOVERING]);
	}

	if (ctx->events & PANDORA_EVENT_DISCONNECTED) {
#ifdef CONFIG_WIFI
		/* TODO: Add a timer to switch to AP mode if we can't still connect after to much
		 * time*/
		pandora_wifi_try_connect();
#endif
	}

	return SMF_EVENT_HANDLED;
}

static void pandora_discovering_entry(void *obj)
{
	LOG_INF("Device is discovering server");
#ifdef CONFIG_LED
	if (status_led) {
		led_blink(status_led, LED_STATUS_0, 250, 250);
	}
#endif

#ifdef CONFIG_HERMES
	/* Start discovery process */
	hermes_discovery_start();
#endif
}

static enum smf_state_result pandora_discovering_run(void *obj)
{
	struct pandora_state_ctx *ctx = obj;

	if (ctx->events & PANDORA_EVENT_DISCOVERY_COMPLETED) {
		smf_set_state(SMF_CTX(obj), &pandora_states[PANDORA_STATE_RUNNING]);
	} else if (ctx->events & PANDORA_EVENT_DISCOVERY_FAILED) {
		/* Retry discovery after delay, or go back to disconnected */
		LOG_WRN("Discovery failed, returning to disconnected state");
		smf_set_state(SMF_CTX(obj), &pandora_states[PANDORA_STATE_DISCONNECTED]);
	} else if (ctx->events & PANDORA_EVENT_DISCONNECTED) {
		smf_set_state(SMF_CTX(obj), &pandora_states[PANDORA_STATE_DISCONNECTED]);
	}

	return SMF_EVENT_HANDLED;
}

const struct smf_state pandora_states[] = {
	[PANDORA_STATE_INIT] =
		SMF_CREATE_STATE(pandora_init_entry, pandora_disconnected_run, NULL, NULL, NULL),
	[PANDORA_STATE_DISCONNECTED] = SMF_CREATE_STATE(pandora_disconnected_entry,
							pandora_disconnected_run, NULL, NULL, NULL),
	[PANDORA_STATE_DISCOVERING] = SMF_CREATE_STATE(pandora_discovering_entry,
						       pandora_discovering_run, NULL, NULL, NULL),
	[PANDORA_STATE_RUNNING] = SMF_CREATE_STATE(pandora_running_entry, pandora_running_run,
						   pandora_running_exit, NULL, NULL),
};

struct net_mgmt_event_callback net_event_callback;

static void net_event_handler(struct net_mgmt_event_callback *cb, unsigned int mgmt_event,
			      struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		pandora_event_post_connected();
	} else if (mgmt_event == NET_EVENT_IPV4_ADDR_DEL) {
		pandora_event_post_disconnected();
	}
}

void pandora_run(void *arg0, void *arg1, void *arg2)
{
	int ret;

	/* Initialize the event */
	pandora_event_init();

	/* Set initial state */
	smf_set_initial(SMF_CTX(&pandora_state_ctx), &pandora_states[PANDORA_STATE_INIT]);

	/* TODO manually trigger net mgmt handler to update state */

	net_mgmt_init_event_callback(&net_event_callback, net_event_handler,
				     NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
	net_mgmt_add_event_callback(&net_event_callback);

	while (1) {
		/* Block until an event is detected */
		LOG_ERR("Waiting event!");
		pandora_state_ctx.events = pandora_event_wait_any();

		/* State machine terminates if a non-zero value is returned */
		ret = smf_run_state(SMF_CTX(&pandora_state_ctx));
		if (ret) {
			/* handle return code and terminate state machine */
			break;
		}
	}
}

#define PANDORA_SERVICE_STACK_SIZE 500
#define PANDORA_SERVICE_PRIORITY   5

K_THREAD_DEFINE(pandora_tid, PANDORA_SERVICE_STACK_SIZE, pandora_run, NULL, NULL, NULL,
		PANDORA_SERVICE_PRIORITY, 0, 0);
