#include <zephyr/kernel.h>
#include <pandora/events.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(Pandora, CONFIG_PANDORA_LOG_LEVEL);

static struct k_event pandora_event;

void pandora_event_post_discovery_completed(void)
{
	LOG_DBG("Discovery completed event");
	k_event_post(&pandora_event, PANDORA_EVENT_DISCOVERY_COMPLETED);
}

void pandora_event_post_discovery_failed(void)
{
	LOG_DBG("Discovery failed event");
	k_event_post(&pandora_event, PANDORA_EVENT_DISCOVERY_FAILED);
}

void pandora_event_post_connected(void)
{
	LOG_DBG("Connected event");
	k_event_post(&pandora_event, PANDORA_EVENT_CONNECTED);
}

void pandora_event_post_disconnected(void)
{
	LOG_DBG("Diconnected event");
	k_event_post(&pandora_event, PANDORA_EVENT_DISCONNECTED);
}

void pandora_event_init(void)
{
	k_event_init(&pandora_event);
}

int pandora_event_wait_any(void)
{
	return k_event_wait(&pandora_event, PANDORA_EVENT_ALL, true, K_FOREVER);
}
