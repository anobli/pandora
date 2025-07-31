#ifndef NARADA_OT_H
#define NARADA_OT_H

#include <zephyr/net/openthread.h>
#include <openthread/coap.h>

#define COAP_MAX_BUF_SIZE 256

struct ot_narada_resource {
	otCoapResource rsc;
};

int narada_req_handler(void *ctx, otMessage *msg, const otMessageInfo *msg_info);

#define NARADA_RESOURCE_DEFINE(_name, _uri, _id, _user_data)                                       \
	STRUCT_SECTION_ITERABLE(ot_narada_resource, _name) = {                                     \
		.rsc =                                                                             \
			{                                                                          \
				.mUriPath = _uri "/" #_id,                                         \
				.mHandler = narada_req_handler,                                    \
				.mContext = &_user_data,                                           \
				.mNext = NULL,                                                     \
			},                                                                         \
	}

struct narada_client {
	otIp6Address *addr;
	const char *uri;
};

#endif /* NARADA_OT_H */
