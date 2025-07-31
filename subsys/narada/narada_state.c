#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>

#include <zephyr/smf.h>

#include <narada/narada.h>
#include <narada/settings.h>
#include <narada/narada_state.h>
#ifdef CONFIG_NARADA_DISCOVERY
#include <narada/discovery/discovery.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(narada, CONFIG_NARADA_LOG_LEVEL);

#define NARADA_NET_EVENTS (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

#ifdef CONFIG_LED
#define LED_STATUS_0 0
static const struct device *status_led = DEVICE_DT_GET_OR_NULL(DT_INST(0, gpio_leds));
#endif

/* List of events */
#define EVENT_CONNECTED           BIT(0)
#define EVENT_DISCONNECTED        BIT(1)
#define EVENT_PAIRING_REQUESTED   BIT(2)
#define EVENT_PAIRING_DONE        BIT(3)
#define EVENT_DISCOVERY_STARTED   BIT(4)
#define EVENT_DISCOVERY_COMPLETED BIT(5)
#define EVENT_DISCOVERY_FAILED    BIT(6)

#define EVENT_ALL 0x7F

enum narada_state {
	NARADA_STATE_INIT,
	NARADA_STATE_DISCONNECTED,
	NARADA_STATE_DISCOVERING,
//	NARADA_STATE_READY,
	NARADA_STATE_RUNNING,
	NARADA_STATE_PAIRING,
	//	NARADA_STATE_ERROR,
	//	NARADA_STATE_FATAL_ERROR,
};

struct narada_state_ctx {
	struct smf_ctx ctx;

	/* Events */
	struct k_event smf_event;
	int32_t events;
};

const struct smf_state narada_states[];

static void narada_running_entry(void *obj)
{
	LOG_INF("Device is connected");
#ifdef CONFIG_LED
	if (status_led) {
		led_on(status_led, LED_STATUS_0);
	}
#endif

#ifdef CONFIG_NARADA_SERVER
	int ret;

	ret = narada_server_start();
	if (ret) {
		smf_set_terminate(SMF_CTX(obj), ret);
	}
#endif
}

static void narada_running_run(void *obj)
{
	struct narada_state_ctx *ctx = obj;

	if (ctx->events & EVENT_PAIRING_REQUESTED) {
		smf_set_state(SMF_CTX(obj), &narada_states[NARADA_STATE_PAIRING]);
	} else if (ctx->events & EVENT_DISCONNECTED) {
		smf_set_state(SMF_CTX(obj), &narada_states[NARADA_STATE_DISCONNECTED]);
	}
}

static void narada_running_exit(void *obj)
{
#ifdef CONFIG_LED
	if (status_led) {
		led_off(status_led, LED_STATUS_0);
	}
#endif
}

static void narada_init_entry(void *obj)
{
	LOG_INF("Device is starting");

	narada_init();
#ifdef CONFIG_NARADA_CLIENT
	narada_devices_init();
	narada_settings_load();
#endif
}

static void narada_disconnected_entry(void *obj)
{
	LOG_INF("Device is disconnected");
#ifdef CONFIG_LED
	if (status_led) {
		led_blink(status_led, LED_STATUS_0, 500, 500);
	}
#endif

#ifdef CONFIG_NARADA_SERVER
	int ret;

	ret = narada_server_stop();
	if (ret) {
		smf_set_terminate(SMF_CTX(obj), ret);
	}
#endif
}

static void narada_disconnected_run(void *obj)
{
	struct narada_state_ctx *ctx = obj;

	if (ctx->events & EVENT_CONNECTED) {
#ifdef CONFIG_NARADA_DISCOVERY
		smf_set_state(SMF_CTX(obj), &narada_states[NARADA_STATE_DISCOVERING]);
#else
		smf_set_state(SMF_CTX(obj), &narada_states[NARADA_STATE_RUNNING]);
#endif
	}
}

static void narada_pairing_entry(void *obj)
{
	LOG_INF("Device is in pairing state");
#ifdef CONFIG_LED
	if (status_led) {
		led_blink(status_led, LED_STATUS_0, 800, 200);
	}
#endif
}

static void narada_pairing_run(void *obj)
{
	struct narada_state_ctx *ctx = obj;
	LOG_ERR("Pairing!");
	if (ctx->events & EVENT_PAIRING_DONE) {
		smf_set_state(SMF_CTX(obj), &narada_states[NARADA_STATE_RUNNING]);
	}
}

static void narada_pairing_exit(void *obj)
{
#ifdef CONFIG_NARADA_CLIENT
	LOG_ERR("Saving!!!");
	narada_settings_save();
#endif
}

#ifdef CONFIG_NARADA_DISCOVERY
static void narada_discovering_entry(void *obj)
{
	LOG_INF("Device is discovering server");
#ifdef CONFIG_LED
	if (status_led) {
		led_blink(status_led, LED_STATUS_0, 250, 250);
	}
#endif

	/* Start discovery process */
	narada_discovery_start();
}

static void narada_discovering_run(void *obj)
{
	struct narada_state_ctx *ctx = obj;

	if (ctx->events & EVENT_DISCOVERY_COMPLETED) {
		smf_set_state(SMF_CTX(obj), &narada_states[NARADA_STATE_RUNNING]);
	} else if (ctx->events & EVENT_DISCOVERY_FAILED) {
		/* Retry discovery after delay, or go back to disconnected */
		LOG_WRN("Discovery failed, returning to disconnected state");
		smf_set_state(SMF_CTX(obj), &narada_states[NARADA_STATE_DISCONNECTED]);
	} else if (ctx->events & EVENT_DISCONNECTED) {
		smf_set_state(SMF_CTX(obj), &narada_states[NARADA_STATE_DISCONNECTED]);
	}
}

static void narada_ready_entry(void *obj)
{
	LOG_INF("Device is ready for operation");
#ifdef CONFIG_LED
	if (status_led) {
		led_on(status_led, LED_STATUS_0);
	}
#endif
}

static void narada_ready_run(void *obj)
{
	struct narada_state_ctx *ctx = obj;

	if (ctx->events & EVENT_PAIRING_REQUESTED) {
		smf_set_state(SMF_CTX(obj), &narada_states[NARADA_STATE_PAIRING]);
	} else if (ctx->events & EVENT_DISCONNECTED) {
		/* Connection lost - need to rediscover when reconnected */
		smf_set_state(SMF_CTX(obj), &narada_states[NARADA_STATE_DISCONNECTED]);
	}
	/* Ready state can transition to RUNNING when application-specific conditions are met */
}

static void narada_ready_exit(void *obj)
{
#ifdef CONFIG_LED
	if (status_led) {
		led_off(status_led, LED_STATUS_0);
	}
#endif
}
#endif /* CONFIG_NARADA_DISCOVERY */

// static void narada_error_run(void *obj)
//{
//	led_blink(status_led, LED_STATUS_0, 200, 200);
//
// }
//
// static void narada_fatal_error_run(void *obj)
//{
//	led_off(status_led, LED_STATUS_0);
//
// }

const struct smf_state narada_states[] = {
	[NARADA_STATE_INIT] =
		SMF_CREATE_STATE(narada_init_entry, narada_disconnected_run, NULL, NULL, NULL),
	[NARADA_STATE_DISCONNECTED] = SMF_CREATE_STATE(narada_disconnected_entry,
						       narada_disconnected_run, NULL, NULL, NULL),
#ifdef CONFIG_NARADA_DISCOVERY
	[NARADA_STATE_DISCOVERING] = SMF_CREATE_STATE(narada_discovering_entry,
						      narada_discovering_run, NULL, NULL, NULL),
//	[NARADA_STATE_READY] = SMF_CREATE_STATE(narada_ready_entry, narada_ready_run,
//						narada_ready_exit, NULL, NULL),
#endif
	[NARADA_STATE_RUNNING] = SMF_CREATE_STATE(narada_running_entry, narada_running_run,
						  narada_running_exit, NULL, NULL),
	[NARADA_STATE_PAIRING] = SMF_CREATE_STATE(narada_pairing_entry, narada_pairing_run,
						  narada_pairing_exit, NULL, NULL),
	//   [NARADA_STATE_ERROR] = SMF_CREATE_STATE(NULL, narada_error_run, NULL, NULL, NULL),
	//   [NARADA_STATE_FATAL_ERROR] = SMF_CREATE_STATE(NULL, narada_fatal_error_run, NULL, NULL,
	//   NULL),
};

// NET_MGMT_REGISTER_EVENT_HANDLER(narada_l4_event_handler,
//                                 0xffffffff,
//                                 &l4_event_handler, NULL);

struct net_mgmt_event_callback net_event_callback;
static struct narada_state_ctx narada_state_ctx;

static void net_event_handler(struct net_mgmt_event_callback *cb, unsigned int mgmt_event,
			      struct net_if *iface)
{
	struct narada_state_ctx *ctx = &narada_state_ctx;

	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		k_event_post(&ctx->smf_event, EVENT_CONNECTED);
	} else if (mgmt_event == NET_EVENT_IPV4_ADDR_DEL) {
		k_event_post(&ctx->smf_event, EVENT_DISCONNECTED);
	}
}

void narada_pairing_started()
{
	k_event_post(&narada_state_ctx.smf_event, EVENT_PAIRING_REQUESTED);
}

void narada_pairing_ended()
{
	k_event_post(&narada_state_ctx.smf_event, EVENT_PAIRING_DONE);
}

void narada_state_discovery_completed(void)
{
	k_event_post(&narada_state_ctx.smf_event, EVENT_DISCOVERY_COMPLETED);
}

void narada_state_discovery_failed(void)
{
	k_event_post(&narada_state_ctx.smf_event, EVENT_DISCOVERY_FAILED);
}

void narada_run(void)
{
	int ret;

	/* Initialize the event */
	k_event_init(&narada_state_ctx.smf_event);

	/* Set initial state */
	smf_set_initial(SMF_CTX(&narada_state_ctx), &narada_states[NARADA_STATE_INIT]);

	/* TODO manually trigger net mgmt handler to update state */

	net_mgmt_init_event_callback(&net_event_callback, net_event_handler,
				     NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
	net_mgmt_add_event_callback(&net_event_callback);

	while (1) {
		/* Block until an event is detected */
		LOG_ERR("Waiting event!");
		narada_state_ctx.events =
			k_event_wait(&narada_state_ctx.smf_event, EVENT_ALL, true, K_FOREVER);

		/* State machine terminates if a non-zero value is returned */
		ret = smf_run_state(SMF_CTX(&narada_state_ctx));
		if (ret) {
			/* handle return code and terminate state machine */
			break;
		}
	}
}
