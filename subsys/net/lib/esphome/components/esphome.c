#include <zephyr/device.h>
#include <zephyr/kernel.h>

#define DT_DRV_COMPAT NABUCASA_ESPHOME
#define ESPHOME_NODE  DT_PATH(esphome)

#define DEFINE_ACTION_FUNCTION(node_id, prop)                                                      \
	extern void DT_STRING_UNQUOTED(node_id, prop)(const struct device *dev);                   \
	static inline void prop(const struct device *dev)                                          \
	{                                                                                          \
		DT_STRING_UNQUOTED(node_id, prop)(dev);                                            \
	}

#define DT_DEFINE_ACTION_FUNCTION(node_id, prop)                                                   \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop), (DEFINE_ACTION_FUNCTION(node_id, prop)),      \
		    (static inline void prop(const struct device *dev){}))

DT_DEFINE_ACTION_FUNCTION(ESPHOME_NODE, on_boot);
DT_DEFINE_ACTION_FUNCTION(ESPHOME_NODE, on_loop);
DT_DEFINE_ACTION_FUNCTION(ESPHOME_NODE, on_shutdown);

int esphome_service(void *arg1, void *arg2, void *arg3)
{
	on_boot(NULL);
	while (1) {
		on_loop(NULL);
		k_sleep(K_MSEC(16));
	};
}

#define ESPHOME_STACK_SIZE (4096)

K_THREAD_DEFINE(esphome_tid, ESPHOME_STACK_SIZE, esphome_service, NULL, NULL, NULL,
		0 /* todo: set priority */, 0, 0);