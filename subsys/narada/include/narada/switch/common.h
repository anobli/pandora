#ifndef NARADA_SWITCH_COMMON_H
#define NARADA_SWITCH_COMMON_H

#include <zephyr/data/json.h>
#include <zephyr/kernel.h>

#include <narada/narada.h>

#define SWITCH_BASE_URI      "switch"
#define SWITCH_MSG_STATE_ON  1
#define SWITCH_MSG_STATE_OFF 0

struct json_switch_state {
	int state;
};

// struct narada_switch {
//	struct narada_device *dev;
//	struct k_sem sem;
//	void *data;
// };

extern const struct json_obj_descr json_switch_state_descr[];
extern const int json_switch_state_descr_size;

#endif /* NARADA_SWITCH_COMMON */
