#include <narada/narada.h>
#include <narada/pairing/client.h>
#include <narada/switch/client.h>

int main(void)
{
//	struct narada_client client;
//	struct narada_switch_client sw = {
//		.client = &client,
//	};
//	k_sem_init(&sw.coap_done_sem, 0, 1);
//	sw.id = 0;
//
//	k_sleep(K_MSEC(10000));
//	narada_init();
//	narada_client_init(&client);
	narada_run();
//	while (1)
//		if (narada_pairing_request(&client, NARADA_SWITCH_DEVICE))
//			k_sleep(K_MSEC(5000));
//	narada_client_init6(&client, "fd65:141e:2c57:1:da3b:9567:4192:c7ef", 5683);
//
//	while(1) {
//		narada_switch_set_state(&sw, 1);
//		k_sleep(K_MSEC(1000));
//
//		narada_switch_set_state(&sw, 0);
//		k_sleep(K_MSEC(1000));
//	}

	return 0;
}
