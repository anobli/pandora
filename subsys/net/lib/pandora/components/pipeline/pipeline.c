/*
 * Copyright (c) 2026 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pandora_pipeline

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <pandora/components/switch.h>
#include <pandora/components/binary_sensor.h>

#define PIPELINE_CB_NAME(_node) _CONCAT(DT_NODE_FULL_NAME_TOKEN(_node), _cb)

#define PIPELINE_DEVICE(_node, prop) DEVICE_DT_GET(DT_PHANDLE(_node, prop))

#define PIPELINE_COND_CODE_1(_node, _prop, _if_1_code)                                             \
	COND_CODE_1(DT_NODE_HAS_PROP(_node, _prop), _if_1_code, ())

#define _PIPELINE_CB(_node, _prop, _domain, _cb)                                                   \
	PANDORA_CB(_domain, _cb, PIPELINE_CB_NAME(_node), PIPELINE_DEVICE(_node, _prop))

#define PIPELINE_CB(_node, _prop, _domain, _cb, _if_1_code)                                        \
	PIPELINE_COND_CODE_1(                                                                      \
		_node, _prop,                                                                      \
		(_PIPELINE_CB(_node, _prop, _domain, _cb) __GET_ARG2_DEBRACKET(0, _if_1_code)))

#define PIPELINE_CB_ENABLED(_node, _prop, _domain, _cb)                                            \
	PIPELINE_COND_CODE_1(                                                                      \
		_node, _prop,                                                                      \
		(if (pandora_cb_enabled(_domain, _cb, &PIPELINE_CB_NAME(_node))) { return 1; };));

#define SWITCH_TOGGLE(_node)                                                                       \
	PIPELINE_COND_CODE_1(_node, switch_toggle,                                                 \
			     (pandora_switch_toggle(PIPELINE_DEVICE(_node, switch_toggle))))

#define BINARY_SENSOR_ON_STATE(_node)                                                              \
	PIPELINE_CB(_node, binary_sensor_on_state, binary_sensor, on_state,                        \
		    ({ SWITCH_TOGGLE(_node); }))

#define BINARY_SENSOR_ON_TURN_ON(_node)                                                            \
	COND_CODE_1(DT_NODE_HAS_PROP(_node, binary_sensor_on_turn_on), (                           \
	PANDORA_CB(binary_sensor, on_state, PIPELINE_CB_NAME(_node), PIPELINE_DEVICE(_node ,binary_sensor_on_turn_on)) { \
		if (state == 1) {                                                                  \
			SWITCH_TOGGLE(_node);                                                      \
		}                                                                                  \
	}), ())

#define BINARY_SENSOR_ON_TURN_OFF(_node)                                                           \
	COND_CODE_1(DT_NODE_HAS_PROP(_node, binary_sensor_on_turn_off), (                           \
	PANDORA_CB(binary_sensor, on_state, PIPELINE_CB_NAME(_node), PIPELINE_DEVICE(_node ,binary_sensor_on_turn_off)) { \
		if (state == 0) {                                                                  \
			SWITCH_TOGGLE(_node);                                                      \
		}                                                                                  \
	}), ())

#define _DEFINE_PANDORA_PIPELINE_FN(_node, _prop, _domain, _cb, _suffix)                           \
	PIPELINE_COND_CODE_1(                                                                      \
		_node, _prop,                                                                      \
		(_CONCAT(pandora_cb_, _suffix)(_domain, _cb, &PIPELINE_CB_NAME(_node))))

#define DEFINE_PANDORA_PIPELINE_FN(_node, _suffix)                                                 \
	void CONCAT(pipeline_, _suffix, _, DT_NODE_FULL_NAME_TOKEN(_node))(void)                   \
	{                                                                                          \
		_DEFINE_PANDORA_PIPELINE_FN(_node, binary_sensor_on_state, binary_sensor,          \
					    on_state, _suffix);                                    \
		_DEFINE_PANDORA_PIPELINE_FN(_node, binary_sensor_on_turn_on, binary_sensor,        \
					    on_turn_on, _suffix);                                  \
		_DEFINE_PANDORA_PIPELINE_FN(_node, binary_sensor_on_turn_off, binary_sensor,       \
					    on_turn_off, _suffix);                                 \
	}

#define DEFINE_PANDORA_PIPELINE_SWITCH_SET_STATE(_node)                                            \
	int CONCAT(pipeline_set_state, _,                                                          \
		   DT_NODE_FULL_NAME_TOKEN(_node))(const struct device *dev, int state)            \
	{                                                                                          \
		if (state) {                                                                       \
			CONCAT(pipeline_enable, _, DT_NODE_FULL_NAME_TOKEN(_node))();              \
		} else {                                                                           \
			CONCAT(pipeline_disable, _, DT_NODE_FULL_NAME_TOKEN(_node))();             \
		}                                                                                  \
		return 0;                                                                          \
	}

#define DEFINE_PANDORA_PIPELINE_ENABLE_DISABLE_GET_STATE(_node)                                    \
	int CONCAT(pipeline_get_state, _,                                                          \
		   DT_NODE_FULL_NAME_TOKEN(_node))(const struct device *dev)                       \
	{                                                                                          \
		PIPELINE_CB_ENABLED(_node, binary_sensor_on_state, binary_sensor, on_state);       \
		return 0;                                                                          \
	}

#define DEFINE_PANDORA_SWITCH_PIPELINE_DEVICE(_node)                                               \
	DEFINE_PANDORA_PIPELINE_SWITCH_SET_STATE(_node)                                            \
	DEFINE_PANDORA_PIPELINE_ENABLE_DISABLE_GET_STATE(_node)                                    \
	static struct pandora_switch_component_api CONCAT(                                         \
		switch_pipeline, DT_NODE_FULL_NAME_TOKEN(_node), _api) = {                         \
		.set_state = CONCAT(pipeline_set_state_, DT_NODE_FULL_NAME_TOKEN(_node)),          \
		.get_state = CONCAT(pipeline_get_state_, DT_NODE_FULL_NAME_TOKEN(_node)),          \
	};                                                                                         \
	DEVICE_DT_DEFINE(_node, NULL, NULL, NULL, NULL, POST_KERNEL,                               \
			 CONFIG_PANDORA_COMPONENT_INIT_PRIORITY,                                   \
			 &CONCAT(switch_pipeline, DT_NODE_FULL_NAME_TOKEN(_node), _api));          \
	DT_DEFINE_PANDORA_SWITCH_COMPONENT(_node);

#define DEFINE_PANDORA_PIPELINE_ENABLE_DISABLE(_node)                                              \
	DEFINE_PANDORA_PIPELINE_FN(_node, enable)                                                  \
	DEFINE_PANDORA_PIPELINE_FN(_node, disable)

#define DEFINE_PANDORA_PIPELINE(_num)                                                              \
	BINARY_SENSOR_ON_STATE(DT_DRV_INST(_num));                                                 \
	BINARY_SENSOR_ON_TURN_ON(DT_DRV_INST(_num));                                               \
	BINARY_SENSOR_ON_TURN_OFF(DT_DRV_INST(_num));                                              \
	DEFINE_PANDORA_PIPELINE_ENABLE_DISABLE(DT_DRV_INST(_num));                                 \
	DEFINE_PANDORA_SWITCH_PIPELINE_DEVICE(DT_DRV_INST(_num));

DT_INST_FOREACH_STATUS_OKAY(DEFINE_PANDORA_PIPELINE);

#define ENABLE_PANDORA_PIPELINE(_num)                                                              \
	_CONCAT(pipeline_enable_, DT_NODE_FULL_NAME_TOKEN(DT_DRV_INST(_num)))();

static int pipeline_init(void)
{
	DT_INST_FOREACH_STATUS_OKAY(ENABLE_PANDORA_PIPELINE);

	return 0;
}

SYS_INIT(pipeline_init, APPLICATION, 99);
