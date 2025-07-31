#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>

#include <zephyr/smf.h>

#include <hermes/hermes.h>
#include <hermes/service.h>
#include <hermes/discovery.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(hermes, CONFIG_HERMES_LOG_LEVEL);

#define HERMES_NET_EVENTS (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

#ifdef CONFIG_LED
#define LED_STATUS_0 0
static const struct device *status_led = DEVICE_DT_GET_OR_NULL(DT_INST(0, gpio_leds));
#endif

/* List of events */
#define EVENT_CONNECTED           BIT(0)
#define EVENT_DISCONNECTED        BIT(1)
#define EVENT_DISCOVERY_STARTED   BIT(2)
#define EVENT_DISCOVERY_COMPLETED BIT(3)
#define EVENT_DISCOVERY_FAILED    BIT(4)

#define EVENT_ALL 0x1F

enum hermes_state {
	HERMES_STATE_INIT,
	HERMES_STATE_DISCONNECTED,
	HERMES_STATE_DISCOVERING,
	HERMES_STATE_RUNNING,
};

struct hermes_state_ctx {
	struct smf_ctx ctx;

	/* Events */
	struct k_event smf_event;
	int32_t events;
};

const struct smf_state hermes_states[];
static struct hermes_state_ctx hermes_state_ctx;

void hermes_state_discovery_completed(void)
{
	LOG_DBG("Discovery completed event");
	k_event_post(&hermes_state_ctx.smf_event, EVENT_DISCOVERY_COMPLETED);
}

void hermes_state_discovery_failed(void)
{
	LOG_DBG("Discovery failed event");
	k_event_post(&hermes_state_ctx.smf_event, EVENT_DISCOVERY_FAILED);
}

void hermes_state_disconnected(void)
{
	LOG_DBG("Diconnected event");
	k_event_post(&hermes_state_ctx.smf_event, EVENT_DISCONNECTED);
}

static void hermes_running_entry(void *obj)
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

static void hermes_running_run(void *obj)
{
	struct hermes_state_ctx *ctx = obj;

	if (ctx->events & EVENT_DISCONNECTED) {
		smf_set_state(SMF_CTX(obj), &hermes_states[HERMES_STATE_DISCONNECTED]);
	}
}

static void hermes_running_exit(void *obj)
{
#ifdef CONFIG_LED
	if (status_led) {
		led_off(status_led, LED_STATUS_0);
	}
#endif
}

static void hermes_init_entry(void *obj)
{
	LOG_INF("Device is starting");

	hermes_init();
	hermes_devices_init();
}

static void hermes_disconnected_entry(void *obj)
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

static void hermes_disconnected_run(void *obj)
{
	struct hermes_state_ctx *ctx = obj;

	if (ctx->events & EVENT_CONNECTED) {
		smf_set_state(SMF_CTX(obj), &hermes_states[HERMES_STATE_DISCOVERING]);
	}
}

static void hermes_discovering_entry(void *obj)
{
	LOG_INF("Device is discovering server");
#ifdef CONFIG_LED
	if (status_led) {
		led_blink(status_led, LED_STATUS_0, 250, 250);
	}
#endif

	/* Start discovery process */
	hermes_discovery_start();
}

static void hermes_discovering_run(void *obj)
{
	struct hermes_state_ctx *ctx = obj;

	if (ctx->events & EVENT_DISCOVERY_COMPLETED) {
		smf_set_state(SMF_CTX(obj), &hermes_states[HERMES_STATE_RUNNING]);
	} else if (ctx->events & EVENT_DISCOVERY_FAILED) {
		/* Retry discovery after delay, or go back to disconnected */
		LOG_WRN("Discovery failed, returning to disconnected state");
		smf_set_state(SMF_CTX(obj), &hermes_states[HERMES_STATE_DISCONNECTED]);
	} else if (ctx->events & EVENT_DISCONNECTED) {
		smf_set_state(SMF_CTX(obj), &hermes_states[HERMES_STATE_DISCONNECTED]);
	}
}

const struct smf_state hermes_states[] = {
	[HERMES_STATE_INIT] =
		SMF_CREATE_STATE(hermes_init_entry, hermes_disconnected_run, NULL, NULL, NULL),
	[HERMES_STATE_DISCONNECTED] = SMF_CREATE_STATE(hermes_disconnected_entry,
						       hermes_disconnected_run, NULL, NULL, NULL),
	[HERMES_STATE_DISCOVERING] = SMF_CREATE_STATE(hermes_discovering_entry,
						      hermes_discovering_run, NULL, NULL, NULL),
	[HERMES_STATE_RUNNING] = SMF_CREATE_STATE(hermes_running_entry, hermes_running_run,
						  hermes_running_exit, NULL, NULL),
};

struct net_mgmt_event_callback net_event_callback;

static void net_event_handler(struct net_mgmt_event_callback *cb, unsigned int mgmt_event,
			      struct net_if *iface)
{
	struct hermes_state_ctx *ctx = &hermes_state_ctx;

	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		k_event_post(&ctx->smf_event, EVENT_CONNECTED);
	} else if (mgmt_event == NET_EVENT_IPV4_ADDR_DEL) {
		k_event_post(&ctx->smf_event, EVENT_DISCONNECTED);
	}
}

void hermes_run(void)
{
	int ret;

	/* Initialize the event */
	k_event_init(&hermes_state_ctx.smf_event);

	/* Set initial state */
	smf_set_initial(SMF_CTX(&hermes_state_ctx), &hermes_states[HERMES_STATE_INIT]);

	/* TODO manually trigger net mgmt handler to update state */

	net_mgmt_init_event_callback(&net_event_callback, net_event_handler,
				     NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
	net_mgmt_add_event_callback(&net_event_callback);

	while (1) {
		/* Block until an event is detected */
		LOG_ERR("Waiting event!");
		hermes_state_ctx.events =
			k_event_wait(&hermes_state_ctx.smf_event, EVENT_ALL, true, K_FOREVER);

		/* State machine terminates if a non-zero value is returned */
		ret = smf_run_state(SMF_CTX(&hermes_state_ctx));
		if (ret) {
			/* handle return code and terminate state machine */
			break;
		}
	}
}
