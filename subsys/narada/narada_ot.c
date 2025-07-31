#include <zephyr/net/openthread.h>
#include <openthread/coap.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(narada, CONFIG_NARADA_LOG_LEVEL);

#include <narada/narada.h>

int narada_resp_send(otMessage *req, const otMessageInfo *req_info, const uint8_t *buf, int len)
{
	otInstance *ot;
	otMessage *resp;
	otCoapCode resp_code;
	otCoapType resp_type;
	otError err;
	int ret;

	ot = openthread_get_default_instance();
	if (!ot) {
		LOG_ERR("Failed to get an OpenThread instance");
		return -ENODEV;
	}

	resp = otCoapNewMessage(ot, NULL);
	if (!resp) {
		LOG_ERR("Failed to allocate a new CoAP message");
		return -ENOMEM;
	}

	switch (otCoapMessageGetType(req)) {
	case OT_COAP_TYPE_CONFIRMABLE:
		resp_type = OT_COAP_TYPE_ACKNOWLEDGMENT;
		break;
	case OT_COAP_TYPE_NON_CONFIRMABLE:
		resp_type = OT_COAP_TYPE_NON_CONFIRMABLE;
		break;
	default:
		LOG_ERR("Invalid message type");
		ret = -EINVAL;
		goto err;
	}

	switch (otCoapMessageGetCode(req)) {
	case OT_COAP_CODE_GET:
		resp_code = OT_COAP_CODE_CONTENT;
		break;
	case OT_COAP_CODE_PUT:
		resp_code = OT_COAP_CODE_CHANGED;
		break;
	default:
		LOG_ERR("Invalid message code");
		ret = -EINVAL;
		goto err;
	}

	err = otCoapMessageInitResponse(resp, req, resp_type, resp_code);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to initialize the response: %s", otThreadErrorToString(err));
		ret = -EBADMSG;
		goto err;
	}

	err = otCoapMessageSetPayloadMarker(resp);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to set payload marker: %s", otThreadErrorToString(err));
		ret = -EBADMSG;
		goto err;
	}

	err = otMessageAppend(resp, buf, len);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to set append payload to response: %s", otThreadErrorToString(err));
		ret = -EBADMSG;
		goto err;
	}

	err = otCoapSendResponse(ot, resp, req_info);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to send the response: %s", otThreadErrorToString(err));
		ret = -EIO;
		goto err;
	}

	return 0;

err:
	otMessageFree(resp);
	return ret;
}

int narada_handler_put(void *ctx, otMessage *msg, const otMessageInfo *msg_info)
{
	struct narada_resource *narada_resource = ctx;
	static uint8_t payload[COAP_MAX_BUF_SIZE];
	int payload_len = otMessageGetLength(msg) - otMessageGetOffset(msg);

	otMessageRead(msg, otMessageGetOffset(msg), payload, payload_len);

	narada_resource->put(narada_resource, payload, payload_len);

	return 0;
}

int narada_handler_get(void *ctx, otMessage *msg, const otMessageInfo *msg_info)
{
	struct narada_resource *narada_resource = ctx;
	uint8_t payload[COAP_MAX_BUF_SIZE];
	uint16_t payload_len;

	narada_resource->get(narada_resource, payload, &payload_len);

	return narada_resp_send(msg, msg_info, payload, payload_len);
}

int narada_req_handler(void *ctx, otMessage *msg, const otMessageInfo *msg_info)
{
	otCoapCode msg_code = otCoapMessageGetCode(msg);
	otCoapType msg_type = otCoapMessageGetType(msg);
	int ret;

	if (msg_type != OT_COAP_TYPE_CONFIRMABLE && msg_type != OT_COAP_TYPE_NON_CONFIRMABLE) {
		return -EINVAL;
	}

	if (msg_code == OT_COAP_CODE_PUT) {
		ret = narada_handler_put(ctx, msg, msg_info);
		if (ret) {
			return ret;
		}

		if (msg_type == OT_COAP_TYPE_CONFIRMABLE) {
			ret = narada_handler_get(ctx, msg, msg_info);
		}

		return ret;
	}

	if (msg_code == OT_COAP_CODE_GET) {
		return narada_handler_get(ctx, msg, msg_info);
	}

	return -EINVAL;
}

static void coap_default_handler(void *context, otMessage *message,
				 const otMessageInfo *message_info)
{
	ARG_UNUSED(context);
	ARG_UNUSED(message);
	ARG_UNUSED(message_info);

	LOG_INF("Received CoAP message that does not match any request "
		"or resource");
}

static int narada_req_send(struct narada_client *client, uint8_t *buf, int len,
			   otCoapResponseHandler handler, void *ctx, otCoapCode code)
{
	otInstance *ot;
	otMessage *msg;
	otMessageInfo msg_info;
	otError err;
	int ret;

	ot = openthread_get_default_instance();
	if (!ot) {
		LOG_ERR("Failed to get an OpenThread instance");
		return -ENODEV;
	}

	memset(&msg_info, 0, sizeof(msg_info));
	memcpy(&msg_info.mPeerAddr, client->addr, sizeof(otIp6Address));
	msg_info.mPeerPort = OT_DEFAULT_COAP_PORT;

	msg = otCoapNewMessage(ot, NULL);
	if (!msg) {
		LOG_ERR("Failed to allocate a new CoAP message");
		return -ENOMEM;
	}

	otCoapMessageInit(msg, OT_COAP_TYPE_CONFIRMABLE, code);

	err = otCoapMessageAppendUriPathOptions(msg, client->uri);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to append uri-path: %s", otThreadErrorToString(err));
		ret = -EBADMSG;
		goto err;
	}

	err = otCoapMessageSetPayloadMarker(msg);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to set payload marker: %s", otThreadErrorToString(err));
		ret = -EBADMSG;
		goto err;
	}

	err = otMessageAppend(msg, buf, len);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to set append payload to response: %s", otThreadErrorToString(err));
		ret = -EBADMSG;
		goto err;
	}

	err = otCoapSendRequest(ot, msg, &msg_info, handler, ctx);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Failed to send the request: %s", otThreadErrorToString(err));
		ret = -EIO; /* Find a better error code */
		goto err;
	}

	return 0;

err:
	otMessageFree(msg);
	return ret;
}

int narada_put_req_send(struct narada_client *client, uint8_t *buf, int len,
			otCoapResponseHandler handler, void *ctx)
{
	return narada_req_send(client, buf, len, handler, ctx, OT_COAP_CODE_PUT);
}

int narada_get_req_send(struct narada_client *client, uint8_t *buf, int len,
			otCoapResponseHandler handler, void *ctx)
{
	return narada_req_send(client, buf, len, handler, ctx, OT_COAP_CODE_GET);
}

int (*narada_req_handler)(void *ctx, uint8_t *buf, int len);
int narada_put_req_send(struct narada_client *client, uint8_t *buf, int len,
			narada_req_handler handler, void *ctx);
int narada_get_req_send(struct narada_client *client, uint8_t *buf, int len,
			narada_req_handler handler, void *ctx);

int narada_init(void)
{

	otError err;
	otInstance *ot;
	int ret;

	LOG_INF("Initializing OpenThread CoAP server");
	ot = openthread_get_default_instance();
	if (!ot) {
		LOG_ERR("Failed to get an OpenThread instance");
		return -ENODEV;
	}

	otCoapSetDefaultHandler(ot, coap_default_handler, NULL);

	err = otCoapStart(ot, OT_DEFAULT_COAP_PORT);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Cannot start CoAP: %s", otThreadErrorToString(err));
		return -EBADMSG;
	}

	STRUCT_SECTION_FOREACH(ot_narada_resource, ot_rsc) {
		struct narada_resource *rsc = ot_rsc->rsc.mContext;

		ret = rsc->init(rsc);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

int narada_client_init6(struct narada_client *client, const char *addr, uint16_t port)
{
	client->addr = addr;
	client->uri = "switch/0";

	return 0;
}
#endif /* CONFIG_OPENTHREAD_COAP */
