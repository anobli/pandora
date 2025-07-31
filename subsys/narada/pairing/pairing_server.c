#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(narada, CONFIG_NARADA_LOG_LEVEL);

#include <narada/narada_state.h>
#include <narada/pairing/server.h>
#include <narada/pairing/common.h>

#define PAIRING_STATE_ACTIVE   1
#define PAIRING_STATE_DISABLED 0

#define PAIRING_TIMEOUT K_MSEC(30000)

#define PAIRING_BUTTON_NODE DT_ALIAS(pairing_button)
#include <zephyr/drivers/gpio.h>
#if DT_NODE_HAS_STATUS_OKAY(PAIRING_BUTTON_NODE)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(PAIRING_BUTTON_NODE, gpios, {0});
#endif

struct narada_pairing {
	int state;
	int filter;
	struct k_timer pairing_timer;
#if DT_NODE_HAS_STATUS_OKAY(PAIRING_BUTTON_NODE)
	struct gpio_callback button_cb_data;
#endif
};

static void narada_pairing_set_disabled(struct narada_pairing *pairing)
{
	pairing->state = PAIRING_STATE_DISABLED;
	narada_pairing_ended();
}

static void narada_pairing_set_active(struct narada_pairing *pairing, k_timeout_t timeout)
{
	k_timer_start(&pairing->pairing_timer, timeout, K_FOREVER);
	pairing->state = PAIRING_STATE_ACTIVE;
	narada_pairing_started();
}

static void pairing_timer_timeout(struct k_timer *timer_id)
{
	struct narada_pairing *pairing =
		CONTAINER_OF(timer_id, struct narada_pairing, pairing_timer);

	narada_pairing_set_disabled(pairing);
}

#if DT_NODE_HAS_STATUS_OKAY(PAIRING_BUTTON_NODE)
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct narada_pairing *pairing = CONTAINER_OF(cb, struct narada_pairing, button_cb_data);
	narada_pairing_set_active(pairing, PAIRING_TIMEOUT);
}
#endif

int narada_pairing_init(struct narada_resource *rsc)
{
	struct narada_pairing *pairing = rsc->data;
	int ret = 0;

	pairing->state = PAIRING_STATE_DISABLED;

#if DT_NODE_HAS_STATUS_OKAY(PAIRING_BUTTON_NODE)
	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n", button.port->name);
		return ret;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name,
		       button.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
		       button.port->name, button.pin);
		return ret;
	}

	k_timer_init(&pairing->pairing_timer, pairing_timer_timeout, NULL);

	gpio_init_callback(&pairing->button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &pairing->button_cb_data);
#endif

	return ret;
}

void narada_pairing_handler_get(struct narada_resource *sw, void *data, uint16_t *len)
{
}

int get_local_ip_address(char *ip_str)
{
	struct net_if *iface;
	struct in_addr *in_addr;

	/* Get the default network interface */
	iface = net_if_get_default();
	if (!iface) {
		LOG_ERR("No default network interface found");
		return -ENODEV;
	}

	in_addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
	/* For IPv4 */
	if (in_addr) {
		net_addr_ntop(AF_INET, in_addr, ip_str, INET_ADDRSTRLEN);
		LOG_INF("Local IPv4 address: %s", ip_str);
		return 0;
	}

	//	/* For IPv6 */
	//	if_addr =
	// net_if_ipv6_addr_lookup(&iface->config.ip.ipv6->unicast[0].address.in6_addr,
	// &iface); 	if (if_addr) { 		net_addr_ntop(AF_INET6, &if_addr->address.in6_addr,
	// ip_str, INET6_ADDRSTRLEN); 		LOG_INF("Local IPv6 address: %s", ip_str);
	// return 0;
	//	}

	LOG_ERR("No IP address found on default interface");
	return -ENOENT;
}

void narada_pairing_handler_put(struct narada_resource *sw, void *data, uint16_t len,
				void *data_out, uint16_t *len_out)
{
	struct pairing_request pairing_data;
	struct pairing_request_done pairing_done;
	struct narada_pairing *pairing = sw->data;
	char ip[INET6_ADDRSTRLEN];

	json_obj_parse((void *)data, len, json_pairing_request_descr,
		       json_pairing_request_descr_size, &pairing_data);

	if (pairing->state != PAIRING_STATE_ACTIVE) {
		*len_out = 0;
		return;
	}

	if (!(pairing_data.device_type & pairing->filter)) {
		*len_out = 0;
		return;
	}

	get_local_ip_address(ip);
	pairing_done.ip = ip;
	json_obj_encode_buf(json_pairing_done_descr, json_pairing_done_descr_size, &pairing_done,
			    data_out, *len_out);
	*len_out = strlen(data_out);
}

static struct narada_pairing narada_pairing = {
	.filter = NARADA_SWITCH_DEVICE,
};
DEFINE_NARADA_PAIRING(&narada_pairing);

int narada_pairing_start(k_timeout_t timeout)
{
	narada_pairing_set_active(&narada_pairing, timeout);

	return 0;
}

int narada_pairing_stop()
{
	narada_pairing_set_disabled(&narada_pairing);

	return 0;
}
