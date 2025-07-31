#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(narada, CONFIG_NARADA_LOG_LEVEL);

#include <narada/narada.h>
#include <narada/narada_state.h>
#include <narada/pairing/client.h>

#define PAIRING_STATE_ACTIVE   1
#define PAIRING_STATE_DISABLED 0

#define PAIRING_TIMEOUT K_MSEC(10000)

int narada_pairing_request_cb(void *ctx, uint8_t *buf, int len)
{
	struct narada_client *client = ctx;

	struct pairing_request_done pairing_done;

	json_obj_parse((void *)buf, len, json_pairing_done_descr, json_pairing_done_descr_size,
		       &pairing_done);

	/* TODO: handle pairing with multiple devices */
	/* TODO: figure out if we got an IPv4 or IPv6 address */
	/* TODO: Use symbol for the port */
	LOG_ERR("Pairing accepted: %s", pairing_done.ip);
	narada_client_set_ipv4(client, pairing_done.ip, 5683);
	narada_pairing_ended();
	return 0;
}

static struct narada_request narada_req;
int narada_pairing_request(struct narada_client *client, int device_type)
{
	uint8_t buf[CONFIG_COAP_CLIENT_MESSAGE_SIZE];
	struct pairing_request req = {.device_type = device_type};

	LOG_ERR("Requesting a pairing: %d", device_type);
	json_obj_encode_buf(json_pairing_request_descr, json_pairing_request_descr_size, &req, buf,
			    CONFIG_COAP_CLIENT_MESSAGE_SIZE);

	narada_req_init(&narada_req, PAIRING_BASE_URI "/0", buf, strlen(buf) + 1,
			narada_pairing_request_cb, client);
	narada_pairing_started();
	return narada_multicast_put_req(client, &narada_req, PAIRING_TIMEOUT);
}
