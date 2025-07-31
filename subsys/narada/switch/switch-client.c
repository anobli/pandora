#include <narada/narada.h>
#include <narada/switch/client.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(narada, CONFIG_NARADA_LOG_LEVEL);

#if 0
static void narada_switch_send_req_cb(void *ctx, otMessage *msg, const otMessageInfo *msg_info,
				      otError error)
{
}

int narada_switch_set_state(struct narada_switch_client *sw, int state)
{
	uint8_t buf[COAP_MAX_BUF_SIZE];

	struct json_switch_state switch_data = {
		.state = state,
	};

	json_obj_encode_buf(json_switch_state_descr, json_switch_state_descr_size, &switch_data,
			    buf, COAP_MAX_BUF_SIZE);

	return narada_put_req_send(sw->dev, buf, strlen(buf) + 1, narada_switch_send_req_cb, NULL);
}

static void narada_switch_get_state_cb(void *ctx, otMessage *msg, const otMessageInfo *msg_info,
				       otError error)
{
	uint8_t buf[COAP_MAX_BUF_SIZE];
	int len = COAP_MAX_BUF_SIZE;
	struct narada_switch_client *sw = (struct narada_switch_client *)ctx;
	struct json_switch_state *switch_data = (struct json_switch_state *)sw->data;
	int ret;

	ret = narada_get_data(msg, buf, &len);
	if (ret) {
		goto exit;
	}

	json_obj_parse(buf, len, json_switch_state_descr, json_switch_state_descr_size,
		       switch_data);

exit:
	k_sem_give(&sw->sem);
}

int narada_switch_get_state(struct narada_switch_client *sw)
{
	struct json_switch_state switch_state;
	int ret;

	ret = narada_get_req_send(sw->dev, NULL, 0, narada_switch_get_state_cb, sw);
	if (ret) {
		return ret;
	}

	ret = k_sem_take(&sw->sem, K_FOREVER);
	if (ret) {
		return ret;
	}

	return switch_state.state;
}
#endif

#ifndef CONFIG_OPENTHREAD_COAP
#include <zephyr/net/coap_client.h>
#include <zephyr/net/socket.h>

static void on_coap_response(int16_t result_code, size_t offset, const uint8_t *payload, size_t len,
			     bool last_block, void *user_data)
{
	struct narada_switch_client *sw = user_data;
	//	LOG_INF("CoAP response, result_code=%d, offset=%u, len=%u", result_code, offset,
	// len);

	if ((COAP_RESPONSE_CODE_CONTENT == result_code) && last_block) {
		//		int64_t elapsed_time = k_uptime_delta(&start_time);
		size_t total_size = offset + len;

		//		LOG_INF("CoAP download done, got %u bytes in %" PRId64 " ms",
		// total_size, 			elapsed_time);
		k_sem_give(&sw->coap_done_sem);
	} else if (COAP_RESPONSE_CODE_CONTENT != result_code) {
		//		LOG_ERR("Error during CoAP download, result_code=%d", result_code);
		k_sem_give(&sw->coap_done_sem);
	}
}

int narada_switch_set_state(struct narada_switch_client *sw, int state)
{
	int ret;
	int sockfd;
	char path[64];
	uint8_t buf[CONFIG_COAP_CLIENT_MESSAGE_SIZE];
	struct coap_client_request request = {.method = COAP_METHOD_GET,
					      .confirmable = true,
					      .path = path,
					      .payload = buf,
					      .len = 0,
					      .cb = on_coap_response,
					      .options = NULL,
					      .num_options = 0,
					      .user_data = sw};

	sprintf(path, "%s/%d", SWITCH_BASE_URI, sw->id);

	struct json_switch_state switch_data = {
		.state = state,
	};

	json_obj_encode_buf(json_switch_state_descr, json_switch_state_descr_size, &switch_data,
			    buf, CONFIG_COAP_CLIENT_MESSAGE_SIZE);
	request.len = strlen(buf);
	request.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN;

	//	LOG_INF("Starting CoAP download using %s", (AF_INET == sa->sa_family) ? "IPv4" :
	//"IPv6");

	sockfd = zsock_socket(sw->client->sa.sa_family, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		LOG_ERR("Failed to create socket, err %d", errno);
		return sockfd;
	}

	coap_client_cancel_requests(&sw->client->client);
	ret = coap_client_req(&sw->client->client, sockfd, &sw->client->sa, &request, NULL);
	if (ret) {
		LOG_ERR("Failed to send CoAP request, err %d", ret);
		return ret;
	}

	/* Wait for CoAP request to complete */
	k_sem_take(&sw->coap_done_sem, K_FOREVER);

	coap_client_cancel_requests(&sw->client->client);
	zsock_close(sockfd);

	return 0;
}
#else
static void narada_switch_send_req_cb(void *ctx, otMessage *msg, const otMessageInfo *msg_info,
				      otError error)
{
}

int narada_switch_set_state(struct narada_switch_client *sw, int state)
{
	uint8_t buf[COAP_MAX_BUF_SIZE];

	struct json_switch_state switch_data = {
		.state = state,
	};

	json_obj_encode_buf(json_switch_state_descr, json_switch_state_descr_size, &switch_data,
			    buf, COAP_MAX_BUF_SIZE);

	return narada_put_req_send(sw->client, buf, strlen(buf) + 1, narada_switch_send_req_cb,
				   NULL);
}
#endif
