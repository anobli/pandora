#ifndef PANDORA_SWITCH_COMPONENT
#define PANDORA_SWITCH_COMPONENT

#include <zephyr/device.h>

#include <pandora/components/component.h>
#include <pandora/components/component_callback.h>

#define DT_DEFINE_PANDORA_SWITCH_COMPONENT(_node)                                                  \
	DT_DEFINE_PANDORA_COMPONENT(_node, PANDORA_COMPONENT_SWITCH)

#define DT_INST_DEFINE_PANDORA_SWITCH_COMPONENT(_inst)                                             \
	DT_INST_DEFINE_PANDORA_COMPONENT(_inst, PANDORA_COMPONENT_SWITCH)

#define switch_on_state_params int, state
DECLARE_PANDORA_CB(switch, on_state);

struct pandora_switch_component_api {
	int (*set_state)(const struct device *dev, int state);
	int (*get_state)(const struct device *dev);
};

int pandora_switch_get_state(const struct device *dev, int *state);
int pandora_switch_set_state(const struct device *dev, int state);
int pandora_switch_turn_on(const struct device *dev);
int pandora_switch_turn_off(const struct device *dev);
int pandora_switch_toggle(const struct device *dev);

#endif /* PANDORA_SWITCH_COMPONENT */
