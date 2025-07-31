#ifndef NARADA_LIBCOAP_H
#define NARADA_LIBCOAP_H

#include <zephyr/net/socket.h>
#include <zephyr/net/coap_client.h>
#ifdef CONFIG_NARADA_SERVER
#include <zephyr/net/coap_service.h>
#include <zephyr/sys/iterable_sections.h>

int narada_handler_put(struct coap_resource *resource, struct coap_packet *request,
		       struct sockaddr *addr, socklen_t addr_len);
int narada_handler_get(struct coap_resource *resource, struct coap_packet *request,
		       struct sockaddr *addr, socklen_t addr_len);

#define NARADA_RESOURCE_DEFINE(_name, _uri, _cmd, _user_data)                                      \
	static const char *const _name##_path[] = {_uri, #_cmd, NULL};                              \
	COAP_RESOURCE_DEFINE(_name, narada_service,                                                \
			     {                                                                     \
				     .path = _name##_path,                                         \
				     .get = narada_handler_get,                                    \
				     .put = narada_handler_put,                                    \
				     .user_data = &_user_data,                                     \
			     });
#endif /* CONFIG_COAP_SERVER */

struct narada_client {
	struct coap_client client;
	struct sockaddr sa;
};

typedef int (*narada_req_handler)(void *ctx, uint8_t *buf, int len);
typedef int (*narada_multicast_req_handler)(void *ctx, uint8_t *buf, int len,
					    const struct sockaddr *server_addr);
struct narada_request {
	struct coap_client_request request;

	struct k_sem coap_done_sem;
	narada_req_handler handler;
	narada_multicast_req_handler multicast_handler;
	void *data;
};

int narada_client_init(struct narada_client *client);
int narada_client_set_ipv4(struct narada_client *client, const char *addr, uint16_t port);
int narada_client_set_ipv6(struct narada_client *client, const char *addr, uint16_t port);
int narada_client_get_ipv4(struct narada_client *client, char *addr, uint16_t *port);
int narada_client_get_ipv6(struct narada_client *client, char *addr, uint16_t *port);

#endif /* NARADA_LIBCOAP_H */
