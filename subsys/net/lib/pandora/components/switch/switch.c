/*
 * Copyright (c) 2026 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pandora/settings.h>
#include <pandora/components/switch.h>

DEFINE_PANDORA_CB(switch, on_state);

int pandora_switch_get_state(const struct device *dev, int *state)
{
	const struct pandora_switch_component_api *api = dev->api;

	*state = api->get_state(dev);
	return *state < 0 ? *state : 0;
}

int pandora_switch_set_state(const struct device *dev, int state)
{
	const struct pandora_switch_component_api *api = dev->api;
	int ret;

	ret = api->set_state(dev, state);
	pandora_cb_call(switch, on_state, dev, state);
	pandora_settings_save_one(dev->name, "state", &state, sizeof(state));

	return ret;
}

int pandora_switch_turn_on(const struct device *dev)
{
	return pandora_switch_set_state(dev, true);
}

int pandora_switch_turn_off(const struct device *dev)
{
	return pandora_switch_set_state(dev, false);
}

int pandora_switch_toggle(const struct device *dev)
{
	int state;
	int ret;

	ret = pandora_switch_get_state(dev, &state);
	if (ret < 0) {
		return ret;
	}

	return pandora_switch_set_state(dev, !state);
}

#if IS_ENABLED(CONFIG_SETTINGS)
static int pandora_switch_settings_load(const struct device *dev)
{
	int state;

	/* Restore saved state */
	pandora_settings_load_one(dev->name, "state", &state, sizeof(state));
	pandora_switch_set_state(dev, state);

	return 0;
}
#endif

#define PANDORA_SWITCH_SETTINGS(node_id)                                                           \
	DT_PANDORA_SETTINGS(node_id, pandora_switch_settings_load, NULL, NULL);

DT_FOREACH_STATUS_OKAY(pandora_switch_gpio, PANDORA_SWITCH_SETTINGS)
DT_FOREACH_STATUS_OKAY(pandora_switch_hbridge, PANDORA_SWITCH_SETTINGS)
