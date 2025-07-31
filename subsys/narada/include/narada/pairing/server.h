#ifndef NARADA_PAIRING_H
#define NARADA_PAIRING_H

#include <narada/pairing/common.h>

struct narada_pairing_resource {
	struct narada_resource rsc;
};

int narada_pairing_init(struct narada_resource *rsc);
void narada_pairing_handler_get(struct narada_resource *sw, void *data, uint16_t *len);
void narada_pairing_handler_put(struct narada_resource *sw, void *data, uint16_t len,
				void *data_out, uint16_t *len_out);

int narada_pairing_start(k_timeout_t timeout);
int narada_pairing_stop();

#define DEFINE_NARADA_PAIRING(_data)                                                               \
	static struct narada_pairing_resource narada_rsc_pairing = {                               \
		.rsc = NARADA_RESOURCE_INIT("pairing", narada_pairing_init,                        \
					    narada_pairing_handler_get, NULL,                      \
					    narada_pairing_handler_put, _data),                    \
	};                                                                                         \
	NARADA_RESOURCE_DEFINE(switch_resource_pairing, PAIRING_BASE_URI, 0, narada_rsc_pairing);

#endif /* NARADA_PAIRING_H */
