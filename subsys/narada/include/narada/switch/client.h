#ifndef NARADA_SWITCH_CLIENT_H
#define NARADA_SWITCH_CLIENT_H

#include <narada/switch/common.h>

struct narada_switch_client {
	struct narada_client *client;
	struct k_sem coap_done_sem;
	int id;
	void *data;
};

int narada_switch_set_state(struct narada_switch_client *sw, int state);
int narada_switch_get_state(struct narada_switch_client *sw);

#endif /* NARADA_SWITCH_CLIENT_H */
