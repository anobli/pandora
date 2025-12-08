#include <zephyr/kernel.h>

#include <pandora/events.h>

static void native_sim_network_handler(struct k_work *work);

/* Discovery worker thread */
K_WORK_DELAYABLE_DEFINE(native_sim_work, native_sim_network_handler);

static void native_sim_network_handler(struct k_work *work)
{
	pandora_event_post_connected();
}

void pandora_native_net_init(void)
{
	k_work_schedule(&native_sim_work, K_MSEC(2000));
}
