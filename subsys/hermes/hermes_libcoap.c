#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hermes, CONFIG_HERMES_LOG_LEVEL);

#include <hermes/hermes.h>
#include <zephyr/posix/poll.h>
#include <zephyr/net/net_ip.h>
#include <string.h>

#ifdef CONFIG_HERMES_SERVER
static const uint16_t hermes_port = HERMES_PORT;

COAP_SERVICE_DEFINE(hermes_service, "0.0.0.0", &hermes_port, 0);

int hermes_handler_get(struct coap_resource *resource, struct coap_packet *request,
		       struct sockaddr *addr, socklen_t addr_len)
{

	uint8_t payload[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	uint16_t payload_len = CONFIG_COAP_SERVER_MESSAGE_SIZE;

	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	uint16_t id;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl, type;
	struct hermes_resource *hermes_resource = resource->user_data;

	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	/* Determine response type */
	type = (type == COAP_TYPE_CON) ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;

	coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, type, tkl, token,
			 COAP_RESPONSE_CODE_CONTENT, id);

	/* Set content format */
	coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
			       COAP_CONTENT_FORMAT_TEXT_PLAIN);

	hermes_resource->get(hermes_resource, payload, &payload_len);

	/* Append payload */
	coap_packet_append_payload_marker(&response);
	coap_packet_append_payload(&response, (uint8_t *)payload, payload_len);

	/* Send to response back to the client */
	return coap_resource_send(resource, &response, addr, addr_len, NULL);
}

int hermes_handler_put(struct coap_resource *resource, struct coap_packet *request,
		       struct sockaddr *addr, socklen_t addr_len)
{
	/* ... Handle the incoming request ... */

	const uint8_t *payload;
	uint16_t payload_len;
	struct hermes_resource *hermes_resource = resource->user_data;

	payload = coap_packet_get_payload(request, &payload_len);

	if (hermes_resource->put) {
		hermes_resource->put(hermes_resource, payload, payload_len);
	} else if (hermes_resource->put_resp) {
		uint16_t id;
		uint8_t tkl, type;
		uint8_t token[COAP_TOKEN_MAX_LEN];
		uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
		uint8_t payload_out[CONFIG_COAP_SERVER_MESSAGE_SIZE];
		uint16_t payload_out_len = CONFIG_COAP_SERVER_MESSAGE_SIZE;
		struct coap_packet response;

		type = coap_header_get_type(request);
		id = coap_header_get_id(request);
		tkl = coap_header_get_token(request, token);

		/* Determine response type */
		type = (type == COAP_TYPE_CON) ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;

		coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, type, tkl, token,
				 COAP_RESPONSE_CODE_CONTENT, id);

		/* Set content format */
		coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				       COAP_CONTENT_FORMAT_TEXT_PLAIN);

		hermes_resource->put_resp(hermes_resource, payload, payload_len, payload_out,
					  &payload_out_len);

		/* Append payload */
		coap_packet_append_payload_marker(&response);
		coap_packet_append_payload(&response, (uint8_t *)payload_out, payload_out_len);

		/* Send to response back to the client */
		return coap_resource_send(resource, &response, addr, addr_len, NULL);
	}

	/* Return a CoAP response code as a shortcut for an empty ACK message */
	return COAP_RESPONSE_CODE_CHANGED;
}
#endif /* CONFIG_HERMES_SERVER */

#define HERMES_MAX_PATH_LEN 256

/* Static multicast buffer management */
struct hermes_multicast_buffer {
	uint8_t data[CONFIG_HERMES_MULTICAST_BUFFER_SIZE];
	bool in_use;
};

static struct hermes_multicast_buffer multicast_buffers[CONFIG_HERMES_MULTICAST_BUFFER_COUNT];
static struct k_mutex multicast_buffer_mutex;

static uint8_t *hermes_multicast_buffer_alloc(void)
{
	k_mutex_lock(&multicast_buffer_mutex, K_FOREVER);

	for (int i = 0; i < CONFIG_HERMES_MULTICAST_BUFFER_COUNT; i++) {
		if (!multicast_buffers[i].in_use) {
			multicast_buffers[i].in_use = true;
			k_mutex_unlock(&multicast_buffer_mutex);
			return multicast_buffers[i].data;
		}
	}

	k_mutex_unlock(&multicast_buffer_mutex);
	return NULL;
}

static void hermes_multicast_buffer_free(uint8_t *buffer)
{
	k_mutex_lock(&multicast_buffer_mutex, K_FOREVER);

	for (int i = 0; i < CONFIG_HERMES_MULTICAST_BUFFER_COUNT; i++) {
		if (multicast_buffers[i].data == buffer) {
			multicast_buffers[i].in_use = false;
			break;
		}
	}

	k_mutex_unlock(&multicast_buffer_mutex);
}

int hermes_req_init(struct hermes_request *request, const char *path, uint8_t *buf, int len,
		    hermes_req_handler handler, void *data)
{
	request->request.path = path;
	request->request.payload = buf;
	request->request.len = len;
	request->request.user_data = request;

	request->handler = handler;
	request->multicast_handler = NULL;
	request->data = data;
	k_sem_init(&request->coap_done_sem, 0, 1);

	return 0;
}

int hermes_multicast_req_init(struct hermes_request *request, const char *path, uint8_t *buf,
			      int len, hermes_multicast_req_handler handler, void *data)
{
	request->request.path = path;
	request->request.payload = buf;
	request->request.len = len;
	request->request.user_data = request;

	request->handler = NULL;
	request->multicast_handler = handler;
	request->data = data;
	k_sem_init(&request->coap_done_sem, 0, 1);

	return 0;
}

static void on_coap_response(int16_t result_code, size_t offset, const uint8_t *payload, size_t len,
			     bool last_block, void *user_data)
{
	struct hermes_request *req = user_data;
	//	LOG_INF("CoAP response, result_code=%d, offset=%u, len=%u", result_code, offset,
	// len);

	if ((COAP_RESPONSE_CODE_CONTENT == result_code) && last_block) {
		//		int64_t elapsed_time = k_uptime_delta(&start_time);
		if (req->handler) {
			req->handler(req->data, payload, len);
		}

		k_sem_give(&req->coap_done_sem);
	} else if (COAP_RESPONSE_CODE_CONTENT != result_code) {
		//		LOG_ERR("Error during CoAP download, result_code=%d", result_code);
		k_sem_give(&req->coap_done_sem);
	}
}

int hermes_req_send(struct hermes_client *client, struct hermes_request *hermes_request,
		    enum coap_method method)
{
	int ret;
	int sockfd;
	struct coap_client_request *request = &hermes_request->request;

	request->method = method;
	request->confirmable = true;
	request->cb = on_coap_response;
	request->options = NULL;
	request->num_options = 0;
	request->fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN;

	sockfd = zsock_socket(client->sa.sa_family, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		LOG_ERR("Failed to create socket, err %d", errno);
		return sockfd;
	}

	coap_client_cancel_requests(&client->client);
	ret = coap_client_req(&client->client, sockfd, &client->sa, request, NULL);
	if (ret) {
		LOG_ERR("Failed to send CoAP request, err %d", ret);
		return ret;
	}

	/* Wait for CoAP request to complete */
	k_sem_take(&hermes_request->coap_done_sem, K_FOREVER);

	coap_client_cancel_requests(&client->client);
	zsock_close(sockfd);

	return 0;
}

int hermes_put_req_send(struct hermes_client *client, struct hermes_request *request)
{
	return hermes_req_send(client, request, COAP_METHOD_PUT);
}

int hermes_get_req_send(struct hermes_client *client, struct hermes_request *request)
{
	return hermes_req_send(client, request, COAP_METHOD_GET);
}

/* Structure to pass context to multicast response processing */
struct multicast_response_context {
	struct hermes_request *req;
	struct sockaddr server_addr;
	socklen_t server_addr_len;
};

int hermes_multicast_req(struct hermes_client *client, struct hermes_request *hermes_request,
			 k_timeout_t timeout, enum coap_method method)
{
	int ret;
	int sockfd;
	struct sockaddr_in mcast_addr;
	struct coap_packet request;
	uint8_t *data = NULL;
	const char *path_segments[10]; /* Max 10 path segments */
	int path_count = 0;
	char *path_copy = NULL;
	char *token_ptr, *saveptr;
	uint8_t response_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct sockaddr_in server_addr;
	socklen_t server_addr_len = sizeof(server_addr);
	struct pollfd poll_fd;
	int64_t timeout_ms;

	/* Allocate static buffer for CoAP packet */
	data = hermes_multicast_buffer_alloc();
	if (!data) {
		LOG_ERR("No available multicast buffers (increase "
			"CONFIG_HERMES_MULTICAST_BUFFER_COUNT)");
		return -ENOMEM;
	}

	/* Set up multicast address */
	mcast_addr.sin_family = AF_INET;
	mcast_addr.sin_port = htons(5683);
	ret = zsock_inet_pton(AF_INET, "224.0.1.187", &mcast_addr.sin_addr);
	if (ret != 1) {
		LOG_ERR("Failed to convert multicast address");
		ret = -EINVAL;
		goto cleanup;
	}

	/* Create UDP socket for multicast */
	sockfd = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0) {
		LOG_ERR("Failed to create socket, err %d", errno);
		ret = sockfd;
		goto cleanup;
	}

	/* Initialize CoAP packet for multicast request */
	ret = coap_packet_init(&request, data, CONFIG_HERMES_MULTICAST_BUFFER_SIZE, COAP_VERSION_1,
			       COAP_TYPE_NON_CON, COAP_TOKEN_MAX_LEN, coap_next_token(), method,
			       coap_next_id());
	if (ret < 0) {
		LOG_ERR("Failed to init CoAP packet: %d", ret);
		goto cleanup_socket;
	}

	/* Parse path and add URI-Path options */
	if (hermes_request->request.path) {
		size_t path_len = strlen(hermes_request->request.path);
		/* Use a small static buffer for path parsing */
		static char path_buffer[HERMES_MAX_PATH_LEN];
		if (path_len >= sizeof(path_buffer)) {
			LOG_ERR("Path too long: %zu bytes (max %zu)", path_len,
				sizeof(path_buffer) - 1);
			ret = -ENAMETOOLONG;
			goto cleanup_socket;
		}
		path_copy = path_buffer;
		strcpy(path_copy, hermes_request->request.path);

		/* Tokenize path by '/' */
		token_ptr = strtok_r(path_copy, "/", &saveptr);
		while (token_ptr && path_count < ARRAY_SIZE(path_segments)) {
			path_segments[path_count++] = token_ptr;
			token_ptr = strtok_r(NULL, "/", &saveptr);
		}

		/* Add URI-Path options */
		for (int i = 0; i < path_count; i++) {
			ret = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
							path_segments[i], strlen(path_segments[i]));
			if (ret < 0) {
				LOG_ERR("Failed to append URI-Path option: %d", ret);
				goto cleanup_path;
			}
		}
	}

	/* Add payload if present */
	if (hermes_request->request.payload && hermes_request->request.len > 0) {
		ret = coap_packet_append_payload_marker(&request);
		if (ret < 0) {
			LOG_ERR("Failed to append payload marker: %d", ret);
			goto cleanup_path;
		}

		ret = coap_packet_append_payload(&request, hermes_request->request.payload,
						 hermes_request->request.len);
		if (ret < 0) {
			LOG_ERR("Failed to append payload: %d", ret);
			goto cleanup_path;
		}
	}

	/* Send multicast request */
	ret = zsock_sendto(sockfd, request.data, request.offset, 0, (struct sockaddr *)&mcast_addr,
			   sizeof(mcast_addr));
	if (ret < 0) {
		LOG_ERR("Failed to send multicast request: %d", errno);
		ret = -errno;
		goto cleanup_path;
	}

	LOG_DBG("Sent multicast CoAP request (%d bytes)", request.offset);

	/* Set up polling for responses */
	poll_fd.fd = sockfd;
	poll_fd.events = POLLIN;
	poll_fd.revents = 0;

	/* Convert timeout to milliseconds */
	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		timeout_ms = -1;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		timeout_ms = 0;
	} else {
		timeout_ms = k_ticks_to_ms_floor64(timeout.ticks);
	}

	/* Wait for and process responses */
	int64_t start_time = k_uptime_get();
	while (true) {
		int64_t elapsed = k_uptime_get() - start_time;
		int64_t remaining_timeout = timeout_ms - elapsed;

		if (timeout_ms >= 0 && remaining_timeout <= 0) {
			break; /* Timeout reached */
		}

		ret = zsock_poll(&poll_fd, 1, remaining_timeout >= 0 ? (int)remaining_timeout : -1);
		if (ret < 0) {
			LOG_ERR("Poll error: %d", errno);
			ret = -errno;
			break;
		} else if (ret == 0) {
			break; /* Timeout */
		}

		if (poll_fd.revents & POLLIN) {
			/* Receive response with source address */
			ssize_t recv_len =
				zsock_recvfrom(sockfd, response_buf, sizeof(response_buf), 0,
					       (struct sockaddr *)&server_addr, &server_addr_len);
			if (recv_len < 0) {
				LOG_ERR("Failed to receive response: %d", errno);
				continue;
			}

			/* Parse CoAP response */
			struct coap_packet response;
			ret = coap_packet_parse(&response, response_buf, recv_len, NULL, 0);
			if (ret < 0) {
				LOG_WRN("Failed to parse CoAP response: %d", ret);
				continue;
			}

			/* Extract response payload */
			uint16_t payload_len;
			const uint8_t *payload = coap_packet_get_payload(&response, &payload_len);
			if (!payload) {
				payload = (uint8_t *)"";
				payload_len = 0;
			}

			/* Store server address in hermes_client for future use */
			memcpy(&client->sa, &server_addr, server_addr_len);

			//			LOG_INF("Received multicast response from %s:%d (%d
			// bytes)",
			// net_sprint_ipv4_addr(&server_addr.sin_addr),
			// ntohs(server_addr.sin_port),
			// payload_len);

			/* Call the appropriate response handler */
			if (hermes_request->multicast_handler) {
				ret = hermes_request->multicast_handler(
					hermes_request->data, payload, payload_len,
					(const struct sockaddr *)&server_addr);
				if (ret < 0) {
					LOG_WRN("Multicast handler returned error: %d", ret);
				}
			} else if (hermes_request->handler) {
				ret = hermes_request->handler(hermes_request->data, payload,
							      payload_len);
				if (ret < 0) {
					LOG_WRN("Handler returned error: %d", ret);
				}
			}
		}
	}

	ret = 0;

cleanup_path:
	/* path_copy points to static buffer, no need to free */
cleanup_socket:
	zsock_close(sockfd);
cleanup:
	if (data) {
		hermes_multicast_buffer_free(data);
	}
	return ret;
}

int hermes_multicast_put_req(struct hermes_client *client, struct hermes_request *hermes_request,
			     k_timeout_t timeout)
{
	return hermes_multicast_req(client, hermes_request, timeout, COAP_METHOD_PUT);
}

int hermes_multicast_get_req(struct hermes_client *client, struct hermes_request *hermes_request,
			     k_timeout_t timeout)
{
	return hermes_multicast_req(client, hermes_request, timeout, COAP_METHOD_GET);
}

int hermes_client_init(struct hermes_client *client)
{
	int ret;
	static bool multicast_buffers_initialized = false;

	memset(client, 0, sizeof(*client));
	ret = coap_client_init(&client->client, NULL);
	if (ret) {
		LOG_ERR("Failed to init coap client, err %d", ret);
		return ret;
	}
	coap_client_cancel_requests(&client->client);

	/* Initialize multicast buffer management (once) */
	if (!multicast_buffers_initialized) {
		k_mutex_init(&multicast_buffer_mutex);
		for (int i = 0; i < CONFIG_HERMES_MULTICAST_BUFFER_COUNT; i++) {
			multicast_buffers[i].in_use = false;
		}
		multicast_buffers_initialized = true;
	}

	return 0;
}

int hermes_client_set_ipv4(struct hermes_client *client, const char *addr, uint16_t port)
{
	struct sockaddr_in *addr4 = (struct sockaddr_in *)&client->sa;

	addr4->sin_family = AF_INET;
	addr4->sin_port = htons(port);
	zsock_inet_pton(AF_INET, addr, &addr4->sin_addr);

	return 0;
}

int hermes_client_set_ipv6(struct hermes_client *client, const char *addr, uint16_t port)
{
	struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&client->sa;

	addr6->sin6_family = AF_INET6;
	addr6->sin6_port = htons(port);
	zsock_inet_pton(AF_INET6, addr, &addr6->sin6_addr);

	return 0;
}

int hermes_client_get_ipv4(struct hermes_client *client, char *ip_str, uint16_t *port)
{
	struct sockaddr_in *addr4 = (struct sockaddr_in *)&client->sa;
	const char *ret;

	if (addr4->sin_family != AF_INET) {
		return -ENOENT;
	}

	*port = HERMES_PORT;
	ret = zsock_inet_ntop(AF_INET, &addr4->sin_addr, ip_str, INET_ADDRSTRLEN);
	if (!ret) {
		return errno;
	}

	return 0;
}

int hermes_client_get_ipv6(struct hermes_client *client, char *ip_str, uint16_t *port)
{
	struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&client->sa;
	const char *ret;

	if (addr6->sin6_family != AF_INET6) {
		return -ENOENT;
	}

	*port = HERMES_PORT;
	ret = zsock_inet_ntop(AF_INET, &addr6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
	if (!ret) {
		return errno;
	}

	return 0;
}

int hermes_init()
{
#ifdef CONFIG_HERMES_SERVER
	COAP_RESOURCE_FOREACH(hermes_service, coap_rsc) {
		struct hermes_resource *rsc = coap_rsc->user_data;

		if (rsc->dev) {
			LOG_INF("Init %s: %s/%s", rsc->dev->name, coap_rsc->path[0],
				coap_rsc->path[1]);
		} else {
			LOG_INF("Init: %s/%s", coap_rsc->path[0], coap_rsc->path[1]);
		}
	}
#endif

	return 0;
}

#ifdef CONFIG_HERMES_SERVER
int hermes_server_start(void)
{
	int ret = 0;

	ret = coap_service_start(&hermes_service);

	return ret;
}

int hermes_server_stop(void)
{
	int ret = 0;

	ret = coap_service_stop(&hermes_service);

	return ret;
}
#endif /* #ifdef CONFIG_HERMES_SERVER */
