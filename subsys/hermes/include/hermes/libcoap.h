/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#ifndef HERMES_LIBCOAP_H
#define HERMES_LIBCOAP_H

#include <zephyr/net/socket.h>
#include <zephyr/net/coap_client.h>
#ifdef CONFIG_HERMES_SERVER
#include <zephyr/net/coap_service.h>
#include <zephyr/sys/iterable_sections.h>

int hermes_handler_put(struct coap_resource *resource, struct coap_packet *request,
		       struct sockaddr *addr, socklen_t addr_len);
int hermes_handler_get(struct coap_resource *resource, struct coap_packet *request,
		       struct sockaddr *addr, socklen_t addr_len);

#define HERMES_RESOURCE_DEFINE(_name, _domain, _ep, _user_data)                                    \
	static const char *const DT_CAT(_name, _path)[] = {#_domain, #_ep, NULL};                  \
	COAP_RESOURCE_DEFINE(_name, hermes_service,                                                \
			     {                                                                     \
				     .path = DT_CAT(_name, _path),                                 \
				     .get = hermes_handler_get,                                    \
				     .put = hermes_handler_put,                                    \
				     .user_data = &_user_data,                                     \
			     });

#endif /* CONFIG_COAP_SERVER */

struct hermes_client {
	struct coap_client client;
	struct sockaddr sa;
};

typedef int (*hermes_req_handler)(void *ctx, const uint8_t *buf, int len);
typedef int (*hermes_multicast_req_handler)(void *ctx, const uint8_t *buf, int len,
					    const struct sockaddr *server_addr);
struct hermes_request {
	struct coap_client_request request;

	struct k_sem coap_done_sem;
	hermes_req_handler handler;
	hermes_multicast_req_handler multicast_handler;
	void *data;
};

int hermes_client_init(struct hermes_client *client);
int hermes_client_set_ipv4(struct hermes_client *client, const char *addr, uint16_t port);
int hermes_client_set_ipv6(struct hermes_client *client, const char *addr, uint16_t port);
int hermes_client_get_ipv4(struct hermes_client *client, char *addr, uint16_t *port);
int hermes_client_get_ipv6(struct hermes_client *client, char *addr, uint16_t *port);

#endif /* HERMES_LIBCOAP_H */
