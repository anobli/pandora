/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#ifndef HERMES_H
#define HERMES_H

#include <hermes/libcoap.h>

#define HERMES_PORT 5683

#define HERMES_SWITCH_DEVICE_TYPE 1
#define HERMES_LIGHT_DEVICE_TYPE  2

#include <hermes/device.h>

#ifdef CONFIG_HERMES_SERVER
struct hermes_resource {
	const struct device *dev;
	void (*get)(struct hermes_resource *rsc, void *payload, uint16_t *payload_len);
	void (*put)(struct hermes_resource *rsc, const void *payload, uint16_t payload_len);
	void (*put_resp)(struct hermes_resource *rsc, const void *payload, uint16_t payload_len,
			 void *payload_out, uint16_t *payload_out_len);
};

#define HERMES_RESOURCE_INIT(_dev, _get, _put, _put_resp)                                          \
	{                                                                                          \
		.dev = _dev,                                                                       \
		.get = _get,                                                                       \
		.put = _put,                                                                       \
		.put_resp = _put_resp,                                                             \
	}

#define HERMES_RESOURCE_DEFINE_DOMAIN(_dev, _domain, _ep, _get, _put, _put_resp)                   \
	static struct hermes_resource DT_CAT3(hermes_rsc_data_, _domain, _ep) =                    \
		HERMES_RESOURCE_INIT(_dev, _get, _put, _put_resp);                                 \
	HERMES_RESOURCE_DEFINE(DT_CAT3(hermes_rsc_, _domain, _ep), _domain, _ep,                   \
			       DT_CAT3(hermes_rsc_data_, _domain, _ep));

#define DT_HERMES_RESOURCE_DEFINE_DOMAIN(node_id, _domain, _ep, _get, _put, _put_resp)             \
	HERMES_RESOURCE_DEFINE_DOMAIN(DEVICE_DT_GET(node_id), _domain, _ep, _get, _put, _put_resp)

#define DT_HERMES_RESOURCE_DEFINE(node_id, _ep, _get, _put, _put_resp)                             \
	DT_HERMES_RESOURCE_DEFINE_DOMAIN(node_id, DT_NODE_FULL_NAME_TOKEN(node_id), _ep, _get,     \
					 _put, _put_resp)

int hermes_server_start(void);
int hermes_server_stop(void);
#endif /* CONFIG_HERMES_SERVER */

int hermes_req_init(struct hermes_request *request, const char *path, uint8_t *buf, int len,
		    hermes_req_handler handler, void *data);
int hermes_multicast_req_init(struct hermes_request *request, const char *path, uint8_t *buf,
			      int len, hermes_multicast_req_handler handler, void *data);
int hermes_put_req_send(struct hermes_client *client, struct hermes_request *request);
int hermes_get_req_send(struct hermes_client *client, struct hermes_request *request);
int hermes_multicast_put_req(struct hermes_client *client, struct hermes_request *hermes_request,
			     k_timeout_t timeout);
int hermes_multicast_get_req(struct hermes_client *client, struct hermes_request *hermes_request,
			     k_timeout_t timeout);

int hermes_init(void);

#endif /* HERMES_H */
