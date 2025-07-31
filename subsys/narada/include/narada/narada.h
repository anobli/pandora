#ifndef NARADA_H
#define NARADA_H

#ifdef CONFIG_OPENTHREAD_COAP
#include <narada/ot.h>
#else
#include <narada/libcoap.h>
#endif

#define NARADA_PORT 5683

#define NARADA_SWITCH_DEVICE_TYPE 1
#define NARADA_LIGHT_DEVICE_TYPE  2

#ifdef CONFIG_NARADA_CLIENT
#include <narada/device.h>
#endif

#ifdef CONFIG_NARADA_SERVER
struct narada_resource {
	const char *name;
	int (*init)(struct narada_resource *rsc);
	void (*get)(struct narada_resource *rsc, void *payload, uint16_t *payload_len);
	void (*put)(struct narada_resource *rsc, void *payload, uint16_t payload_len);
	void (*put_resp)(struct narada_resource *rsc, void *payload, uint16_t payload_len,
			 void *payload_out, uint16_t *payload_out_len);
	void *data;
};

#define DT_NODE_NARADA_RESOURCE_NAME(node) STRINGIFY(DT_NODE_FULL_NAME_TOKEN(node))

#define DT_INST_NARADA_RESOURCE_NAME(inst)                                                         \
	DT_NODE_NARADA_RESOURCE_NAME(DT_INST(inst, DT_DRV_COMPAT))

#define NARADA_RESOURCE_INIT(_id, _init, _get, _put, _put_resp, _data)                             \
	{                                                                                          \
		.name = _id,                                                                       \
		.init = _init,                                                                     \
		.get = _get,                                                                       \
		.put = _put,                                                                       \
		.put_resp = _put_resp,                                                             \
		.data = _data,                                                                     \
	}

#endif /* CONFIG_NARADA_SERVER */

#ifdef CONFIG_NARADA_CLIENT
int narada_req_init(struct narada_request *request, const char *path, uint8_t *buf, int len,
		    narada_req_handler handler, void *data);
int narada_multicast_req_init(struct narada_request *request, const char *path, uint8_t *buf,
			      int len, narada_multicast_req_handler handler, void *data);
int narada_put_req_send(struct narada_client *client, struct narada_request *request);
int narada_get_req_send(struct narada_client *client, struct narada_request *request);
int narada_multicast_put_req(struct narada_client *client, struct narada_request *narada_request,
			     k_timeout_t timeout);
int narada_multicast_get_req(struct narada_client *client, struct narada_request *narada_request,
			     k_timeout_t timeout);
#endif /* CONFIG_NARADA_CLIENT */

int narada_init(void);

#endif /* NARADA_H */
