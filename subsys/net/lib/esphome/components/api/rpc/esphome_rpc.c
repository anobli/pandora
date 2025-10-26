

/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "api.pb-c.h"
#include "esphome_rpc.h"

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esphome_rpc, CONFIG_ESPHOME_RPC_LOG_LEVEL);

static int varint_encode(uint64_t val, uint8_t *out);
static int esphome_header_size(uint32_t rpc_id, size_t len);
static int esphome_encode_header(uint32_t rpc_id, size_t len, uint8_t *out);
static int esphome_rpc_send(const struct device *dev, void *out, size_t len);
static void *zephyr_alloc(void *allocator_data, size_t size);
static void zephyr_free(void *allocator_data, void *pointer);

ProtobufCAllocator esphome_pb_allocator = {
	.alloc = zephyr_alloc,
	.free = zephyr_free,
	.allocator_data = NULL,
};

#ifdef HAS_PROTO_MESSAGE_DUMP

static void esphome_HelloRequestDump(HelloRequest *msg)
{
	LOG_PRINTK("HelloRequest: {\n");

	LOG_PRINTK("\tclient_info: %s\n", msg->client_info);

	LOG_PRINTK("\tapi_version_major: %u\n", msg->api_version_major);

	LOG_PRINTK("\tapi_version_minor: %u\n", msg->api_version_minor);

	LOG_PRINTK("}\n");
}

static void esphome_HelloResponseDump(HelloResponse *msg)
{
	LOG_PRINTK("HelloResponse: {\n");

	LOG_PRINTK("\tapi_version_major: %u\n", msg->api_version_major);

	LOG_PRINTK("\tapi_version_minor: %u\n", msg->api_version_minor);

	LOG_PRINTK("\tserver_info: %s\n", msg->server_info);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("}\n");
}

static void esphome_ConnectRequestDump(ConnectRequest *msg)
{
	LOG_PRINTK("ConnectRequest: {\n");

	LOG_PRINTK("\tpassword: %s\n", msg->password);

	LOG_PRINTK("}\n");
}

static void esphome_ConnectResponseDump(ConnectResponse *msg)
{
	LOG_PRINTK("ConnectResponse: {\n");

	LOG_PRINTK("\tinvalid_password: %s\n", msg->invalid_password ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_DisconnectRequestDump(void)
{
	LOG_PRINTK("DisconnectRequest: {\n");
	LOG_PRINTK("}\n");
}

static void esphome_DisconnectResponseDump(void)
{
	LOG_PRINTK("DisconnectResponse: {\n");
	LOG_PRINTK("}\n");
}

static void esphome_PingRequestDump(void)
{
	LOG_PRINTK("PingRequest: {\n");
	LOG_PRINTK("}\n");
}

static void esphome_PingResponseDump(void)
{
	LOG_PRINTK("PingResponse: {\n");
	LOG_PRINTK("}\n");
}

static void esphome_DeviceInfoRequestDump(void)
{
	LOG_PRINTK("DeviceInfoRequest: {\n");
	LOG_PRINTK("}\n");
}

static void esphome_DeviceInfoResponseDump(DeviceInfoResponse *msg)
{
	LOG_PRINTK("DeviceInfoResponse: {\n");

	LOG_PRINTK("\tuses_password: %s\n", msg->uses_password ? "True" : "False");

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tmac_address: %s\n", msg->mac_address);

	LOG_PRINTK("\tesphome_version: %s\n", msg->esphome_version);

	LOG_PRINTK("\tcompilation_time: %s\n", msg->compilation_time);

	LOG_PRINTK("\tmodel: %s\n", msg->model);

	LOG_PRINTK("\thas_deep_sleep: %s\n", msg->has_deep_sleep ? "True" : "False");

	LOG_PRINTK("\tproject_name: %s\n", msg->project_name);

	LOG_PRINTK("\tproject_version: %s\n", msg->project_version);

	LOG_PRINTK("\twebserver_port: %u\n", msg->webserver_port);

	LOG_PRINTK("\tlegacy_bluetooth_proxy_version: %u\n", msg->legacy_bluetooth_proxy_version);

	LOG_PRINTK("\tbluetooth_proxy_feature_flags: %u\n", msg->bluetooth_proxy_feature_flags);

	LOG_PRINTK("\tmanufacturer: %s\n", msg->manufacturer);

	LOG_PRINTK("\tfriendly_name: %s\n", msg->friendly_name);

	LOG_PRINTK("\tlegacy_voice_assistant_version: %u\n", msg->legacy_voice_assistant_version);

	LOG_PRINTK("\tvoice_assistant_feature_flags: %u\n", msg->voice_assistant_feature_flags);

	LOG_PRINTK("\tsuggested_area: %s\n", msg->suggested_area);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesRequestDump(void)
{
	LOG_PRINTK("ListEntitiesRequest: {\n");
	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesDoneResponseDump(void)
{
	LOG_PRINTK("ListEntitiesDoneResponse: {\n");
	LOG_PRINTK("}\n");
}

static void esphome_SubscribeStatesRequestDump(void)
{
	LOG_PRINTK("SubscribeStatesRequest: {\n");
	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesBinarySensorResponseDump(ListEntitiesBinarySensorResponse *msg)
{
	LOG_PRINTK("ListEntitiesBinarySensorResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\tdevice_class: %s\n", msg->device_class);

	LOG_PRINTK("\tis_status_binary_sensor: %s\n",
		   msg->is_status_binary_sensor ? "True" : "False");

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("}\n");
}

static void esphome_BinarySensorStateResponseDump(BinarySensorStateResponse *msg)
{
	LOG_PRINTK("BinarySensorStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", msg->state ? "True" : "False");

	LOG_PRINTK("\tmissing_state: %s\n", msg->missing_state ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesCoverResponseDump(ListEntitiesCoverResponse *msg)
{
	LOG_PRINTK("ListEntitiesCoverResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\tassumed_state: %s\n", msg->assumed_state ? "True" : "False");

	LOG_PRINTK("\tsupports_position: %s\n", msg->supports_position ? "True" : "False");

	LOG_PRINTK("\tsupports_tilt: %s\n", msg->supports_tilt ? "True" : "False");

	LOG_PRINTK("\tdevice_class: %s\n", msg->device_class);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tsupports_stop: %s\n", msg->supports_stop ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_CoverStateResponseDump(CoverStateResponse *msg)
{
	LOG_PRINTK("CoverStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tlegacy_state: %s\n", .LegacyCoverState_enum_to_string(msg->legacy_state));

	LOG_PRINTK("\tposition: %f\n", msg->position);

	LOG_PRINTK("\ttilt: %f\n", msg->tilt);

	LOG_PRINTK("\tcurrent_operation: %s\n",
		   .CoverOperation_enum_to_string(msg->current_operation));

	LOG_PRINTK("}\n");
}

static void esphome_CoverCommandRequestDump(CoverCommandRequest *msg)
{
	LOG_PRINTK("CoverCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\thas_legacy_command: %s\n", msg->has_legacy_command ? "True" : "False");

	LOG_PRINTK("\tlegacy_command: %s\n",
		   .LegacyCoverCommand_enum_to_string(msg->legacy_command));

	LOG_PRINTK("\thas_position: %s\n", msg->has_position ? "True" : "False");

	LOG_PRINTK("\tposition: %f\n", msg->position);

	LOG_PRINTK("\thas_tilt: %s\n", msg->has_tilt ? "True" : "False");

	LOG_PRINTK("\ttilt: %f\n", msg->tilt);

	LOG_PRINTK("\tstop: %s\n", msg->stop ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesFanResponseDump(ListEntitiesFanResponse *msg)
{
	LOG_PRINTK("ListEntitiesFanResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\tsupports_oscillation: %s\n", msg->supports_oscillation ? "True" : "False");

	LOG_PRINTK("\tsupports_speed: %s\n", msg->supports_speed ? "True" : "False");

	LOG_PRINTK("\tsupports_direction: %s\n", msg->supports_direction ? "True" : "False");

	LOG_PRINTK("\tsupported_speed_count: %d\n", msg->supported_speed_count);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tsupported_preset_modes: %s\n", msg->supported_preset_modes);

	LOG_PRINTK("}\n");
}

static void esphome_FanStateResponseDump(FanStateResponse *msg)
{
	LOG_PRINTK("FanStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", msg->state ? "True" : "False");

	LOG_PRINTK("\toscillating: %s\n", msg->oscillating ? "True" : "False");

	LOG_PRINTK("\tspeed: %s\n", .FanSpeed_enum_to_string(msg->speed));

	LOG_PRINTK("\tdirection: %s\n", .FanDirection_enum_to_string(msg->direction));

	LOG_PRINTK("\tspeed_level: %d\n", msg->speed_level);

	LOG_PRINTK("\tpreset_mode: %s\n", msg->preset_mode);

	LOG_PRINTK("}\n");
}

static void esphome_FanCommandRequestDump(FanCommandRequest *msg)
{
	LOG_PRINTK("FanCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\thas_state: %s\n", msg->has_state ? "True" : "False");

	LOG_PRINTK("\tstate: %s\n", msg->state ? "True" : "False");

	LOG_PRINTK("\thas_speed: %s\n", msg->has_speed ? "True" : "False");

	LOG_PRINTK("\tspeed: %s\n", .FanSpeed_enum_to_string(msg->speed));

	LOG_PRINTK("\thas_oscillating: %s\n", msg->has_oscillating ? "True" : "False");

	LOG_PRINTK("\toscillating: %s\n", msg->oscillating ? "True" : "False");

	LOG_PRINTK("\thas_direction: %s\n", msg->has_direction ? "True" : "False");

	LOG_PRINTK("\tdirection: %s\n", .FanDirection_enum_to_string(msg->direction));

	LOG_PRINTK("\thas_speed_level: %s\n", msg->has_speed_level ? "True" : "False");

	LOG_PRINTK("\tspeed_level: %d\n", msg->speed_level);

	LOG_PRINTK("\thas_preset_mode: %s\n", msg->has_preset_mode ? "True" : "False");

	LOG_PRINTK("\tpreset_mode: %s\n", msg->preset_mode);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesLightResponseDump(ListEntitiesLightResponse *msg)
{
	LOG_PRINTK("ListEntitiesLightResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\tsupported_color_modes: %s\n",
		   .ColorMode_enum_to_string(msg->supported_color_modes));

	LOG_PRINTK("\tlegacy_supports_brightness: %s\n",
		   msg->legacy_supports_brightness ? "True" : "False");

	LOG_PRINTK("\tlegacy_supports_rgb: %s\n", msg->legacy_supports_rgb ? "True" : "False");

	LOG_PRINTK("\tlegacy_supports_white_value: %s\n",
		   msg->legacy_supports_white_value ? "True" : "False");

	LOG_PRINTK("\tlegacy_supports_color_temperature: %s\n",
		   msg->legacy_supports_color_temperature ? "True" : "False");

	LOG_PRINTK("\tmin_mireds: %f\n", msg->min_mireds);

	LOG_PRINTK("\tmax_mireds: %f\n", msg->max_mireds);

	LOG_PRINTK("\teffects: %s\n", msg->effects);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("}\n");
}

static void esphome_LightStateResponseDump(LightStateResponse *msg)
{
	LOG_PRINTK("LightStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", msg->state ? "True" : "False");

	LOG_PRINTK("\tbrightness: %f\n", msg->brightness);

	LOG_PRINTK("\tcolor_mode: %s\n", .ColorMode_enum_to_string(msg->color_mode));

	LOG_PRINTK("\tcolor_brightness: %f\n", msg->color_brightness);

	LOG_PRINTK("\tred: %f\n", msg->red);

	LOG_PRINTK("\tgreen: %f\n", msg->green);

	LOG_PRINTK("\tblue: %f\n", msg->blue);

	LOG_PRINTK("\twhite: %f\n", msg->white);

	LOG_PRINTK("\tcolor_temperature: %f\n", msg->color_temperature);

	LOG_PRINTK("\tcold_white: %f\n", msg->cold_white);

	LOG_PRINTK("\twarm_white: %f\n", msg->warm_white);

	LOG_PRINTK("\teffect: %s\n", msg->effect);

	LOG_PRINTK("}\n");
}

static void esphome_LightCommandRequestDump(LightCommandRequest *msg)
{
	LOG_PRINTK("LightCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\thas_state: %s\n", msg->has_state ? "True" : "False");

	LOG_PRINTK("\tstate: %s\n", msg->state ? "True" : "False");

	LOG_PRINTK("\thas_brightness: %s\n", msg->has_brightness ? "True" : "False");

	LOG_PRINTK("\tbrightness: %f\n", msg->brightness);

	LOG_PRINTK("\thas_color_mode: %s\n", msg->has_color_mode ? "True" : "False");

	LOG_PRINTK("\tcolor_mode: %s\n", .ColorMode_enum_to_string(msg->color_mode));

	LOG_PRINTK("\thas_color_brightness: %s\n", msg->has_color_brightness ? "True" : "False");

	LOG_PRINTK("\tcolor_brightness: %f\n", msg->color_brightness);

	LOG_PRINTK("\thas_rgb: %s\n", msg->has_rgb ? "True" : "False");

	LOG_PRINTK("\tred: %f\n", msg->red);

	LOG_PRINTK("\tgreen: %f\n", msg->green);

	LOG_PRINTK("\tblue: %f\n", msg->blue);

	LOG_PRINTK("\thas_white: %s\n", msg->has_white ? "True" : "False");

	LOG_PRINTK("\twhite: %f\n", msg->white);

	LOG_PRINTK("\thas_color_temperature: %s\n", msg->has_color_temperature ? "True" : "False");

	LOG_PRINTK("\tcolor_temperature: %f\n", msg->color_temperature);

	LOG_PRINTK("\thas_cold_white: %s\n", msg->has_cold_white ? "True" : "False");

	LOG_PRINTK("\tcold_white: %f\n", msg->cold_white);

	LOG_PRINTK("\thas_warm_white: %s\n", msg->has_warm_white ? "True" : "False");

	LOG_PRINTK("\twarm_white: %f\n", msg->warm_white);

	LOG_PRINTK("\thas_transition_length: %s\n", msg->has_transition_length ? "True" : "False");

	LOG_PRINTK("\ttransition_length: %u\n", msg->transition_length);

	LOG_PRINTK("\thas_flash_length: %s\n", msg->has_flash_length ? "True" : "False");

	LOG_PRINTK("\tflash_length: %u\n", msg->flash_length);

	LOG_PRINTK("\thas_effect: %s\n", msg->has_effect ? "True" : "False");

	LOG_PRINTK("\teffect: %s\n", msg->effect);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesSensorResponseDump(ListEntitiesSensorResponse *msg)
{
	LOG_PRINTK("ListEntitiesSensorResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tunit_of_measurement: %s\n", msg->unit_of_measurement);

	LOG_PRINTK("\taccuracy_decimals: %d\n", msg->accuracy_decimals);

	LOG_PRINTK("\tforce_update: %s\n", msg->force_update ? "True" : "False");

	LOG_PRINTK("\tdevice_class: %s\n", msg->device_class);

	LOG_PRINTK("\tstate_class: %s\n", .SensorStateClass_enum_to_string(msg->state_class));

	LOG_PRINTK("\tlegacy_last_reset_type: %s\n",
		   .SensorLastResetType_enum_to_string(msg->legacy_last_reset_type));

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("}\n");
}

static void esphome_SensorStateResponseDump(SensorStateResponse *msg)
{
	LOG_PRINTK("SensorStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %f\n", msg->state);

	LOG_PRINTK("\tmissing_state: %s\n", msg->missing_state ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesSwitchResponseDump(ListEntitiesSwitchResponse *msg)
{
	LOG_PRINTK("ListEntitiesSwitchResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tassumed_state: %s\n", msg->assumed_state ? "True" : "False");

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tdevice_class: %s\n", msg->device_class);

	LOG_PRINTK("}\n");
}

static void esphome_SwitchStateResponseDump(SwitchStateResponse *msg)
{
	LOG_PRINTK("SwitchStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", msg->state ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_SwitchCommandRequestDump(SwitchCommandRequest *msg)
{
	LOG_PRINTK("SwitchCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", msg->state ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesTextSensorResponseDump(ListEntitiesTextSensorResponse *msg)
{
	LOG_PRINTK("ListEntitiesTextSensorResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tdevice_class: %s\n", msg->device_class);

	LOG_PRINTK("}\n");
}

static void esphome_TextSensorStateResponseDump(TextSensorStateResponse *msg)
{
	LOG_PRINTK("TextSensorStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", msg->state);

	LOG_PRINTK("\tmissing_state: %s\n", msg->missing_state ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_SubscribeLogsRequestDump(SubscribeLogsRequest *msg)
{
	LOG_PRINTK("SubscribeLogsRequest: {\n");

	LOG_PRINTK("\tlevel: %s\n", .LogLevel_enum_to_string(msg->level));

	LOG_PRINTK("\tdump_config: %s\n", msg->dump_config ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_SubscribeLogsResponseDump(SubscribeLogsResponse *msg)
{
	LOG_PRINTK("SubscribeLogsResponse: {\n");

	LOG_PRINTK("\tlevel: %s\n", .LogLevel_enum_to_string(msg->level));

	LOG_PRINTK("\tmessage: %s\n", msg->message);

	LOG_PRINTK("\tsend_failed: %s\n", msg->send_failed ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_SubscribeHomeassistantServicesRequestDump(void)
{
	LOG_PRINTK("SubscribeHomeassistantServicesRequest: {\n");
	LOG_PRINTK("}\n");
}

static void esphome_HomeassistantServiceResponseDump(HomeassistantServiceResponse *msg)
{
	LOG_PRINTK("HomeassistantServiceResponse: {\n");

	LOG_PRINTK("\tservice: %s\n", msg->service);

	// TODO: data

	// TODO: data_template

	// TODO: variables

	LOG_PRINTK("\tis_event: %s\n", msg->is_event ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_SubscribeHomeAssistantStatesRequestDump(void)
{
	LOG_PRINTK("SubscribeHomeAssistantStatesRequest: {\n");
	LOG_PRINTK("}\n");
}

static void
esphome_SubscribeHomeAssistantStateResponseDump(SubscribeHomeAssistantStateResponse *msg)
{
	LOG_PRINTK("SubscribeHomeAssistantStateResponse: {\n");

	LOG_PRINTK("\tentity_id: %s\n", msg->entity_id);

	LOG_PRINTK("\tattribute: %s\n", msg->attribute);

	LOG_PRINTK("\tonce: %s\n", msg->once ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_HomeAssistantStateResponseDump(HomeAssistantStateResponse *msg)
{
	LOG_PRINTK("HomeAssistantStateResponse: {\n");

	LOG_PRINTK("\tentity_id: %s\n", msg->entity_id);

	LOG_PRINTK("\tstate: %s\n", msg->state);

	LOG_PRINTK("\tattribute: %s\n", msg->attribute);

	LOG_PRINTK("}\n");
}

static void esphome_GetTimeRequestDump(void)
{
	LOG_PRINTK("GetTimeRequest: {\n");
	LOG_PRINTK("}\n");
}

static void esphome_GetTimeResponseDump(GetTimeResponse *msg)
{
	LOG_PRINTK("GetTimeResponse: {\n");

	LOG_PRINTK("\tepoch_seconds: %u\n", msg->epoch_seconds);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesServicesResponseDump(ListEntitiesServicesResponse *msg)
{
	LOG_PRINTK("ListEntitiesServicesResponse: {\n");

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	// TODO: args

	LOG_PRINTK("}\n");
}

static void esphome_ExecuteServiceRequestDump(ExecuteServiceRequest *msg)
{
	LOG_PRINTK("ExecuteServiceRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	// TODO: args

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesCameraResponseDump(ListEntitiesCameraResponse *msg)
{
	LOG_PRINTK("ListEntitiesCameraResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("}\n");
}

static void esphome_CameraImageResponseDump(CameraImageResponse *msg)
{
	LOG_PRINTK("CameraImageResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_HEXDUMP_INF(msg->data.data, msg->data.len, "data:");

	LOG_PRINTK("\tdone: %s\n", msg->done ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_CameraImageRequestDump(CameraImageRequest *msg)
{
	LOG_PRINTK("CameraImageRequest: {\n");

	LOG_PRINTK("\tsingle: %s\n", msg->single ? "True" : "False");

	LOG_PRINTK("\tstream: %s\n", msg->stream ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesClimateResponseDump(ListEntitiesClimateResponse *msg)
{
	LOG_PRINTK("ListEntitiesClimateResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\tsupports_current_temperature: %s\n",
		   msg->supports_current_temperature ? "True" : "False");

	LOG_PRINTK("\tsupports_two_point_target_temperature: %s\n",
		   msg->supports_two_point_target_temperature ? "True" : "False");

	LOG_PRINTK("\tsupported_modes: %s\n", .ClimateMode_enum_to_string(msg->supported_modes));

	LOG_PRINTK("\tvisual_min_temperature: %f\n", msg->visual_min_temperature);

	LOG_PRINTK("\tvisual_max_temperature: %f\n", msg->visual_max_temperature);

	LOG_PRINTK("\tvisual_target_temperature_step: %f\n", msg->visual_target_temperature_step);

	LOG_PRINTK("\tlegacy_supports_away: %s\n", msg->legacy_supports_away ? "True" : "False");

	LOG_PRINTK("\tsupports_action: %s\n", msg->supports_action ? "True" : "False");

	LOG_PRINTK("\tsupported_fan_modes: %s\n",
		   .ClimateFanMode_enum_to_string(msg->supported_fan_modes));

	LOG_PRINTK("\tsupported_swing_modes: %s\n",
		   .ClimateSwingMode_enum_to_string(msg->supported_swing_modes));

	LOG_PRINTK("\tsupported_custom_fan_modes: %s\n", msg->supported_custom_fan_modes);

	LOG_PRINTK("\tsupported_presets: %s\n",
		   .ClimatePreset_enum_to_string(msg->supported_presets));

	LOG_PRINTK("\tsupported_custom_presets: %s\n", msg->supported_custom_presets);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tvisual_current_temperature_step: %f\n", msg->visual_current_temperature_step);

	LOG_PRINTK("\tsupports_current_humidity: %s\n",
		   msg->supports_current_humidity ? "True" : "False");

	LOG_PRINTK("\tsupports_target_humidity: %s\n",
		   msg->supports_target_humidity ? "True" : "False");

	LOG_PRINTK("\tvisual_min_humidity: %f\n", msg->visual_min_humidity);

	LOG_PRINTK("\tvisual_max_humidity: %f\n", msg->visual_max_humidity);

	LOG_PRINTK("}\n");
}

static void esphome_ClimateStateResponseDump(ClimateStateResponse *msg)
{
	LOG_PRINTK("ClimateStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tmode: %s\n", .ClimateMode_enum_to_string(msg->mode));

	LOG_PRINTK("\tcurrent_temperature: %f\n", msg->current_temperature);

	LOG_PRINTK("\ttarget_temperature: %f\n", msg->target_temperature);

	LOG_PRINTK("\ttarget_temperature_low: %f\n", msg->target_temperature_low);

	LOG_PRINTK("\ttarget_temperature_high: %f\n", msg->target_temperature_high);

	LOG_PRINTK("\tunused_legacy_away: %s\n", msg->unused_legacy_away ? "True" : "False");

	LOG_PRINTK("\taction: %s\n", .ClimateAction_enum_to_string(msg->action));

	LOG_PRINTK("\tfan_mode: %s\n", .ClimateFanMode_enum_to_string(msg->fan_mode));

	LOG_PRINTK("\tswing_mode: %s\n", .ClimateSwingMode_enum_to_string(msg->swing_mode));

	LOG_PRINTK("\tcustom_fan_mode: %s\n", msg->custom_fan_mode);

	LOG_PRINTK("\tpreset: %s\n", .ClimatePreset_enum_to_string(msg->preset));

	LOG_PRINTK("\tcustom_preset: %s\n", msg->custom_preset);

	LOG_PRINTK("\tcurrent_humidity: %f\n", msg->current_humidity);

	LOG_PRINTK("\ttarget_humidity: %f\n", msg->target_humidity);

	LOG_PRINTK("}\n");
}

static void esphome_ClimateCommandRequestDump(ClimateCommandRequest *msg)
{
	LOG_PRINTK("ClimateCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\thas_mode: %s\n", msg->has_mode ? "True" : "False");

	LOG_PRINTK("\tmode: %s\n", .ClimateMode_enum_to_string(msg->mode));

	LOG_PRINTK("\thas_target_temperature: %s\n",
		   msg->has_target_temperature ? "True" : "False");

	LOG_PRINTK("\ttarget_temperature: %f\n", msg->target_temperature);

	LOG_PRINTK("\thas_target_temperature_low: %s\n",
		   msg->has_target_temperature_low ? "True" : "False");

	LOG_PRINTK("\ttarget_temperature_low: %f\n", msg->target_temperature_low);

	LOG_PRINTK("\thas_target_temperature_high: %s\n",
		   msg->has_target_temperature_high ? "True" : "False");

	LOG_PRINTK("\ttarget_temperature_high: %f\n", msg->target_temperature_high);

	LOG_PRINTK("\tunused_has_legacy_away: %s\n",
		   msg->unused_has_legacy_away ? "True" : "False");

	LOG_PRINTK("\tunused_legacy_away: %s\n", msg->unused_legacy_away ? "True" : "False");

	LOG_PRINTK("\thas_fan_mode: %s\n", msg->has_fan_mode ? "True" : "False");

	LOG_PRINTK("\tfan_mode: %s\n", .ClimateFanMode_enum_to_string(msg->fan_mode));

	LOG_PRINTK("\thas_swing_mode: %s\n", msg->has_swing_mode ? "True" : "False");

	LOG_PRINTK("\tswing_mode: %s\n", .ClimateSwingMode_enum_to_string(msg->swing_mode));

	LOG_PRINTK("\thas_custom_fan_mode: %s\n", msg->has_custom_fan_mode ? "True" : "False");

	LOG_PRINTK("\tcustom_fan_mode: %s\n", msg->custom_fan_mode);

	LOG_PRINTK("\thas_preset: %s\n", msg->has_preset ? "True" : "False");

	LOG_PRINTK("\tpreset: %s\n", .ClimatePreset_enum_to_string(msg->preset));

	LOG_PRINTK("\thas_custom_preset: %s\n", msg->has_custom_preset ? "True" : "False");

	LOG_PRINTK("\tcustom_preset: %s\n", msg->custom_preset);

	LOG_PRINTK("\thas_target_humidity: %s\n", msg->has_target_humidity ? "True" : "False");

	LOG_PRINTK("\ttarget_humidity: %f\n", msg->target_humidity);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesNumberResponseDump(ListEntitiesNumberResponse *msg)
{
	LOG_PRINTK("ListEntitiesNumberResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tmin_value: %f\n", msg->min_value);

	LOG_PRINTK("\tmax_value: %f\n", msg->max_value);

	LOG_PRINTK("\tstep: %f\n", msg->step);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tunit_of_measurement: %s\n", msg->unit_of_measurement);

	LOG_PRINTK("\tmode: %s\n", .NumberMode_enum_to_string(msg->mode));

	LOG_PRINTK("\tdevice_class: %s\n", msg->device_class);

	LOG_PRINTK("}\n");
}

static void esphome_NumberStateResponseDump(NumberStateResponse *msg)
{
	LOG_PRINTK("NumberStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %f\n", msg->state);

	LOG_PRINTK("\tmissing_state: %s\n", msg->missing_state ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_NumberCommandRequestDump(NumberCommandRequest *msg)
{
	LOG_PRINTK("NumberCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %f\n", msg->state);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesSelectResponseDump(ListEntitiesSelectResponse *msg)
{
	LOG_PRINTK("ListEntitiesSelectResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\toptions: %s\n", msg->options);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("}\n");
}

static void esphome_SelectStateResponseDump(SelectStateResponse *msg)
{
	LOG_PRINTK("SelectStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", msg->state);

	LOG_PRINTK("\tmissing_state: %s\n", msg->missing_state ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_SelectCommandRequestDump(SelectCommandRequest *msg)
{
	LOG_PRINTK("SelectCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", msg->state);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesLockResponseDump(ListEntitiesLockResponse *msg)
{
	LOG_PRINTK("ListEntitiesLockResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tassumed_state: %s\n", msg->assumed_state ? "True" : "False");

	LOG_PRINTK("\tsupports_open: %s\n", msg->supports_open ? "True" : "False");

	LOG_PRINTK("\trequires_code: %s\n", msg->requires_code ? "True" : "False");

	LOG_PRINTK("\tcode_format: %s\n", msg->code_format);

	LOG_PRINTK("}\n");
}

static void esphome_LockStateResponseDump(LockStateResponse *msg)
{
	LOG_PRINTK("LockStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", .LockState_enum_to_string(msg->state));

	LOG_PRINTK("}\n");
}

static void esphome_LockCommandRequestDump(LockCommandRequest *msg)
{
	LOG_PRINTK("LockCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tcommand: %s\n", .LockCommand_enum_to_string(msg->command));

	LOG_PRINTK("\thas_code: %s\n", msg->has_code ? "True" : "False");

	LOG_PRINTK("\tcode: %s\n", msg->code);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesButtonResponseDump(ListEntitiesButtonResponse *msg)
{
	LOG_PRINTK("ListEntitiesButtonResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tdevice_class: %s\n", msg->device_class);

	LOG_PRINTK("}\n");
}

static void esphome_ButtonCommandRequestDump(ButtonCommandRequest *msg)
{
	LOG_PRINTK("ButtonCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesMediaPlayerResponseDump(ListEntitiesMediaPlayerResponse *msg)
{
	LOG_PRINTK("ListEntitiesMediaPlayerResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tsupports_pause: %s\n", msg->supports_pause ? "True" : "False");

	// TODO: supported_formats

	LOG_PRINTK("}\n");
}

static void esphome_MediaPlayerStateResponseDump(MediaPlayerStateResponse *msg)
{
	LOG_PRINTK("MediaPlayerStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", .MediaPlayerState_enum_to_string(msg->state));

	LOG_PRINTK("\tvolume: %f\n", msg->volume);

	LOG_PRINTK("\tmuted: %s\n", msg->muted ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_MediaPlayerCommandRequestDump(MediaPlayerCommandRequest *msg)
{
	LOG_PRINTK("MediaPlayerCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\thas_command: %s\n", msg->has_command ? "True" : "False");

	LOG_PRINTK("\tcommand: %s\n", .MediaPlayerCommand_enum_to_string(msg->command));

	LOG_PRINTK("\thas_volume: %s\n", msg->has_volume ? "True" : "False");

	LOG_PRINTK("\tvolume: %f\n", msg->volume);

	LOG_PRINTK("\thas_media_url: %s\n", msg->has_media_url ? "True" : "False");

	LOG_PRINTK("\tmedia_url: %s\n", msg->media_url);

	LOG_PRINTK("\thas_announcement: %s\n", msg->has_announcement ? "True" : "False");

	LOG_PRINTK("\tannouncement: %s\n", msg->announcement ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_SubscribeBluetoothLEAdvertisementsRequestDump(
	SubscribeBluetoothLEAdvertisementsRequest *msg)
{
	LOG_PRINTK("SubscribeBluetoothLEAdvertisementsRequest: {\n");

	LOG_PRINTK("\tflags: %u\n", msg->flags);

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothLEAdvertisementResponseDump(BluetoothLEAdvertisementResponse *msg)
{
	LOG_PRINTK("BluetoothLEAdvertisementResponse: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\trssi: %d\n", msg->rssi);

	LOG_PRINTK("\tservice_uuids: %s\n", msg->service_uuids);

	// TODO: service_data

	// TODO: manufacturer_data

	LOG_PRINTK("\taddress_type: %u\n", msg->address_type);

	LOG_PRINTK("}\n");
}

static void
esphome_BluetoothLERawAdvertisementsResponseDump(BluetoothLERawAdvertisementsResponse *msg)
{
	LOG_PRINTK("BluetoothLERawAdvertisementsResponse: {\n");

	// TODO: advertisements

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothDeviceRequestDump(BluetoothDeviceRequest *msg)
{
	LOG_PRINTK("BluetoothDeviceRequest: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\trequest_type: %s\n",
		   .BluetoothDeviceRequestType_enum_to_string(msg->request_type));

	LOG_PRINTK("\thas_address_type: %s\n", msg->has_address_type ? "True" : "False");

	LOG_PRINTK("\taddress_type: %u\n", msg->address_type);

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothDeviceConnectionResponseDump(BluetoothDeviceConnectionResponse *msg)
{
	LOG_PRINTK("BluetoothDeviceConnectionResponse: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\tconnected: %s\n", msg->connected ? "True" : "False");

	LOG_PRINTK("\tmtu: %u\n", msg->mtu);

	LOG_PRINTK("\terror: %d\n", msg->error);

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothGATTGetServicesRequestDump(BluetoothGATTGetServicesRequest *msg)
{
	LOG_PRINTK("BluetoothGATTGetServicesRequest: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothGATTGetServicesResponseDump(BluetoothGATTGetServicesResponse *msg)
{
	LOG_PRINTK("BluetoothGATTGetServicesResponse: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	// TODO: services

	LOG_PRINTK("}\n");
}

static void
esphome_BluetoothGATTGetServicesDoneResponseDump(BluetoothGATTGetServicesDoneResponse *msg)
{
	LOG_PRINTK("BluetoothGATTGetServicesDoneResponse: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothGATTReadRequestDump(BluetoothGATTReadRequest *msg)
{
	LOG_PRINTK("BluetoothGATTReadRequest: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\thandle: %u\n", msg->handle);

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothGATTReadResponseDump(BluetoothGATTReadResponse *msg)
{
	LOG_PRINTK("BluetoothGATTReadResponse: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\thandle: %u\n", msg->handle);

	LOG_HEXDUMP_INF(msg->data.data, msg->data.len, "data:");

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothGATTWriteRequestDump(BluetoothGATTWriteRequest *msg)
{
	LOG_PRINTK("BluetoothGATTWriteRequest: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\thandle: %u\n", msg->handle);

	LOG_PRINTK("\tresponse: %s\n", msg->response ? "True" : "False");

	LOG_HEXDUMP_INF(msg->data.data, msg->data.len, "data:");

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothGATTReadDescriptorRequestDump(BluetoothGATTReadDescriptorRequest *msg)
{
	LOG_PRINTK("BluetoothGATTReadDescriptorRequest: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\thandle: %u\n", msg->handle);

	LOG_PRINTK("}\n");
}

static void
esphome_BluetoothGATTWriteDescriptorRequestDump(BluetoothGATTWriteDescriptorRequest *msg)
{
	LOG_PRINTK("BluetoothGATTWriteDescriptorRequest: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\thandle: %u\n", msg->handle);

	LOG_HEXDUMP_INF(msg->data.data, msg->data.len, "data:");

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothGATTNotifyRequestDump(BluetoothGATTNotifyRequest *msg)
{
	LOG_PRINTK("BluetoothGATTNotifyRequest: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\thandle: %u\n", msg->handle);

	LOG_PRINTK("\tenable: %s\n", msg->enable ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothGATTNotifyDataResponseDump(BluetoothGATTNotifyDataResponse *msg)
{
	LOG_PRINTK("BluetoothGATTNotifyDataResponse: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\thandle: %u\n", msg->handle);

	LOG_HEXDUMP_INF(msg->data.data, msg->data.len, "data:");

	LOG_PRINTK("}\n");
}

static void esphome_SubscribeBluetoothConnectionsFreeRequestDump(void)
{
	LOG_PRINTK("SubscribeBluetoothConnectionsFreeRequest: {\n");
	LOG_PRINTK("}\n");
}

static void esphome_BluetoothConnectionsFreeResponseDump(BluetoothConnectionsFreeResponse *msg)
{
	LOG_PRINTK("BluetoothConnectionsFreeResponse: {\n");

	LOG_PRINTK("\tfree: %u\n", msg->free);

	LOG_PRINTK("\tlimit: %u\n", msg->limit);

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothGATTErrorResponseDump(BluetoothGATTErrorResponse *msg)
{
	LOG_PRINTK("BluetoothGATTErrorResponse: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\thandle: %u\n", msg->handle);

	LOG_PRINTK("\terror: %d\n", msg->error);

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothGATTWriteResponseDump(BluetoothGATTWriteResponse *msg)
{
	LOG_PRINTK("BluetoothGATTWriteResponse: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\thandle: %u\n", msg->handle);

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothGATTNotifyResponseDump(BluetoothGATTNotifyResponse *msg)
{
	LOG_PRINTK("BluetoothGATTNotifyResponse: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\thandle: %u\n", msg->handle);

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothDevicePairingResponseDump(BluetoothDevicePairingResponse *msg)
{
	LOG_PRINTK("BluetoothDevicePairingResponse: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\tpaired: %s\n", msg->paired ? "True" : "False");

	LOG_PRINTK("\terror: %d\n", msg->error);

	LOG_PRINTK("}\n");
}

static void esphome_BluetoothDeviceUnpairingResponseDump(BluetoothDeviceUnpairingResponse *msg)
{
	LOG_PRINTK("BluetoothDeviceUnpairingResponse: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\tsuccess: %s\n", msg->success ? "True" : "False");

	LOG_PRINTK("\terror: %d\n", msg->error);

	LOG_PRINTK("}\n");
}

static void esphome_UnsubscribeBluetoothLEAdvertisementsRequestDump(void)
{
	LOG_PRINTK("UnsubscribeBluetoothLEAdvertisementsRequest: {\n");
	LOG_PRINTK("}\n");
}

static void esphome_BluetoothDeviceClearCacheResponseDump(BluetoothDeviceClearCacheResponse *msg)
{
	LOG_PRINTK("BluetoothDeviceClearCacheResponse: {\n");

	LOG_PRINTK("\taddress: %lu\n", msg->address);

	LOG_PRINTK("\tsuccess: %s\n", msg->success ? "True" : "False");

	LOG_PRINTK("\terror: %d\n", msg->error);

	LOG_PRINTK("}\n");
}

static void esphome_SubscribeVoiceAssistantRequestDump(SubscribeVoiceAssistantRequest *msg)
{
	LOG_PRINTK("SubscribeVoiceAssistantRequest: {\n");

	LOG_PRINTK("\tsubscribe: %s\n", msg->subscribe ? "True" : "False");

	LOG_PRINTK("\tflags: %u\n", msg->flags);

	LOG_PRINTK("}\n");
}

static void esphome_VoiceAssistantRequestDump(VoiceAssistantRequest *msg)
{
	LOG_PRINTK("VoiceAssistantRequest: {\n");

	LOG_PRINTK("\tstart: %s\n", msg->start ? "True" : "False");

	LOG_PRINTK("\tconversation_id: %s\n", msg->conversation_id);

	LOG_PRINTK("\tflags: %u\n", msg->flags);

	// TODO: audio_settings

	LOG_PRINTK("\twake_word_phrase: %s\n", msg->wake_word_phrase);

	LOG_PRINTK("}\n");
}

static void esphome_VoiceAssistantResponseDump(VoiceAssistantResponse *msg)
{
	LOG_PRINTK("VoiceAssistantResponse: {\n");

	LOG_PRINTK("\tport: %u\n", msg->port);

	LOG_PRINTK("\terror: %s\n", msg->error ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_VoiceAssistantEventResponseDump(VoiceAssistantEventResponse *msg)
{
	LOG_PRINTK("VoiceAssistantEventResponse: {\n");

	LOG_PRINTK("\tevent_type: %s\n", .VoiceAssistantEvent_enum_to_string(msg->event_type));

	// TODO: data

	LOG_PRINTK("}\n");
}

static void esphome_VoiceAssistantAudioDump(VoiceAssistantAudio *msg)
{
	LOG_PRINTK("VoiceAssistantAudio: {\n");

	LOG_HEXDUMP_INF(msg->data.data, msg->data.len, "data:");

	LOG_PRINTK("\tend: %s\n", msg->end ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_VoiceAssistantTimerEventResponseDump(VoiceAssistantTimerEventResponse *msg)
{
	LOG_PRINTK("VoiceAssistantTimerEventResponse: {\n");

	LOG_PRINTK("\tevent_type: %s\n", .VoiceAssistantTimerEvent_enum_to_string(msg->event_type));

	LOG_PRINTK("\ttimer_id: %s\n", msg->timer_id);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\ttotal_seconds: %u\n", msg->total_seconds);

	LOG_PRINTK("\tseconds_left: %u\n", msg->seconds_left);

	LOG_PRINTK("\tis_active: %s\n", msg->is_active ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_VoiceAssistantAnnounceRequestDump(VoiceAssistantAnnounceRequest *msg)
{
	LOG_PRINTK("VoiceAssistantAnnounceRequest: {\n");

	LOG_PRINTK("\tmedia_id: %s\n", msg->media_id);

	LOG_PRINTK("\ttext: %s\n", msg->text);

	LOG_PRINTK("}\n");
}

static void esphome_VoiceAssistantAnnounceFinishedDump(VoiceAssistantAnnounceFinished *msg)
{
	LOG_PRINTK("VoiceAssistantAnnounceFinished: {\n");

	LOG_PRINTK("\tsuccess: %s\n", msg->success ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_VoiceAssistantConfigurationRequestDump(void)
{
	LOG_PRINTK("VoiceAssistantConfigurationRequest: {\n");
	LOG_PRINTK("}\n");
}

static void
esphome_VoiceAssistantConfigurationResponseDump(VoiceAssistantConfigurationResponse *msg)
{
	LOG_PRINTK("VoiceAssistantConfigurationResponse: {\n");

	// TODO: available_wake_words

	LOG_PRINTK("\tactive_wake_words: %s\n", msg->active_wake_words);

	LOG_PRINTK("\tmax_active_wake_words: %u\n", msg->max_active_wake_words);

	LOG_PRINTK("}\n");
}

static void esphome_VoiceAssistantSetConfigurationDump(VoiceAssistantSetConfiguration *msg)
{
	LOG_PRINTK("VoiceAssistantSetConfiguration: {\n");

	LOG_PRINTK("\tactive_wake_words: %s\n", msg->active_wake_words);

	LOG_PRINTK("}\n");
}

static void
esphome_ListEntitiesAlarmControlPanelResponseDump(ListEntitiesAlarmControlPanelResponse *msg)
{
	LOG_PRINTK("ListEntitiesAlarmControlPanelResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tsupported_features: %u\n", msg->supported_features);

	LOG_PRINTK("\trequires_code: %s\n", msg->requires_code ? "True" : "False");

	LOG_PRINTK("\trequires_code_to_arm: %s\n", msg->requires_code_to_arm ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_AlarmControlPanelStateResponseDump(AlarmControlPanelStateResponse *msg)
{
	LOG_PRINTK("AlarmControlPanelStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", .AlarmControlPanelState_enum_to_string(msg->state));

	LOG_PRINTK("}\n");
}

static void esphome_AlarmControlPanelCommandRequestDump(AlarmControlPanelCommandRequest *msg)
{
	LOG_PRINTK("AlarmControlPanelCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tcommand: %s\n", .AlarmControlPanelStateCommand_enum_to_string(msg->command));

	LOG_PRINTK("\tcode: %s\n", msg->code);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesTextResponseDump(ListEntitiesTextResponse *msg)
{
	LOG_PRINTK("ListEntitiesTextResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tmin_length: %u\n", msg->min_length);

	LOG_PRINTK("\tmax_length: %u\n", msg->max_length);

	LOG_PRINTK("\tpattern: %s\n", msg->pattern);

	LOG_PRINTK("\tmode: %s\n", .TextMode_enum_to_string(msg->mode));

	LOG_PRINTK("}\n");
}

static void esphome_TextStateResponseDump(TextStateResponse *msg)
{
	LOG_PRINTK("TextStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", msg->state);

	LOG_PRINTK("\tmissing_state: %s\n", msg->missing_state ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_TextCommandRequestDump(TextCommandRequest *msg)
{
	LOG_PRINTK("TextCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tstate: %s\n", msg->state);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesDateResponseDump(ListEntitiesDateResponse *msg)
{
	LOG_PRINTK("ListEntitiesDateResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("}\n");
}

static void esphome_DateStateResponseDump(DateStateResponse *msg)
{
	LOG_PRINTK("DateStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tmissing_state: %s\n", msg->missing_state ? "True" : "False");

	LOG_PRINTK("\tyear: %u\n", msg->year);

	LOG_PRINTK("\tmonth: %u\n", msg->month);

	LOG_PRINTK("\tday: %u\n", msg->day);

	LOG_PRINTK("}\n");
}

static void esphome_DateCommandRequestDump(DateCommandRequest *msg)
{
	LOG_PRINTK("DateCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tyear: %u\n", msg->year);

	LOG_PRINTK("\tmonth: %u\n", msg->month);

	LOG_PRINTK("\tday: %u\n", msg->day);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesTimeResponseDump(ListEntitiesTimeResponse *msg)
{
	LOG_PRINTK("ListEntitiesTimeResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("}\n");
}

static void esphome_TimeStateResponseDump(TimeStateResponse *msg)
{
	LOG_PRINTK("TimeStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tmissing_state: %s\n", msg->missing_state ? "True" : "False");

	LOG_PRINTK("\thour: %u\n", msg->hour);

	LOG_PRINTK("\tminute: %u\n", msg->minute);

	LOG_PRINTK("\tsecond: %u\n", msg->second);

	LOG_PRINTK("}\n");
}

static void esphome_TimeCommandRequestDump(TimeCommandRequest *msg)
{
	LOG_PRINTK("TimeCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\thour: %u\n", msg->hour);

	LOG_PRINTK("\tminute: %u\n", msg->minute);

	LOG_PRINTK("\tsecond: %u\n", msg->second);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesEventResponseDump(ListEntitiesEventResponse *msg)
{
	LOG_PRINTK("ListEntitiesEventResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tdevice_class: %s\n", msg->device_class);

	LOG_PRINTK("\tevent_types: %s\n", msg->event_types);

	LOG_PRINTK("}\n");
}

static void esphome_EventResponseDump(EventResponse *msg)
{
	LOG_PRINTK("EventResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tevent_type: %s\n", msg->event_type);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesValveResponseDump(ListEntitiesValveResponse *msg)
{
	LOG_PRINTK("ListEntitiesValveResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tdevice_class: %s\n", msg->device_class);

	LOG_PRINTK("\tassumed_state: %s\n", msg->assumed_state ? "True" : "False");

	LOG_PRINTK("\tsupports_position: %s\n", msg->supports_position ? "True" : "False");

	LOG_PRINTK("\tsupports_stop: %s\n", msg->supports_stop ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_ValveStateResponseDump(ValveStateResponse *msg)
{
	LOG_PRINTK("ValveStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tposition: %f\n", msg->position);

	LOG_PRINTK("\tcurrent_operation: %s\n",
		   .ValveOperation_enum_to_string(msg->current_operation));

	LOG_PRINTK("}\n");
}

static void esphome_ValveCommandRequestDump(ValveCommandRequest *msg)
{
	LOG_PRINTK("ValveCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\thas_position: %s\n", msg->has_position ? "True" : "False");

	LOG_PRINTK("\tposition: %f\n", msg->position);

	LOG_PRINTK("\tstop: %s\n", msg->stop ? "True" : "False");

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesDateTimeResponseDump(ListEntitiesDateTimeResponse *msg)
{
	LOG_PRINTK("ListEntitiesDateTimeResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("}\n");
}

static void esphome_DateTimeStateResponseDump(DateTimeStateResponse *msg)
{
	LOG_PRINTK("DateTimeStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tmissing_state: %s\n", msg->missing_state ? "True" : "False");

	LOG_PRINTK("\tepoch_seconds: %u\n", msg->epoch_seconds);

	LOG_PRINTK("}\n");
}

static void esphome_DateTimeCommandRequestDump(DateTimeCommandRequest *msg)
{
	LOG_PRINTK("DateTimeCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tepoch_seconds: %u\n", msg->epoch_seconds);

	LOG_PRINTK("}\n");
}

static void esphome_ListEntitiesUpdateResponseDump(ListEntitiesUpdateResponse *msg)
{
	LOG_PRINTK("ListEntitiesUpdateResponse: {\n");

	LOG_PRINTK("\tobject_id: %s\n", msg->object_id);

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tname: %s\n", msg->name);

	LOG_PRINTK("\tunique_id: %s\n", msg->unique_id);

	LOG_PRINTK("\ticon: %s\n", msg->icon);

	LOG_PRINTK("\tdisabled_by_default: %s\n", msg->disabled_by_default ? "True" : "False");

	LOG_PRINTK("\tentity_category: %s\n", .EntityCategory_enum_to_string(msg->entity_category));

	LOG_PRINTK("\tdevice_class: %s\n", msg->device_class);

	LOG_PRINTK("}\n");
}

static void esphome_UpdateStateResponseDump(UpdateStateResponse *msg)
{
	LOG_PRINTK("UpdateStateResponse: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tmissing_state: %s\n", msg->missing_state ? "True" : "False");

	LOG_PRINTK("\tin_progress: %s\n", msg->in_progress ? "True" : "False");

	LOG_PRINTK("\thas_progress: %s\n", msg->has_progress ? "True" : "False");

	LOG_PRINTK("\tprogress: %f\n", msg->progress);

	LOG_PRINTK("\tcurrent_version: %s\n", msg->current_version);

	LOG_PRINTK("\tlatest_version: %s\n", msg->latest_version);

	LOG_PRINTK("\ttitle: %s\n", msg->title);

	LOG_PRINTK("\trelease_summary: %s\n", msg->release_summary);

	LOG_PRINTK("\trelease_url: %s\n", msg->release_url);

	LOG_PRINTK("}\n");
}

static void esphome_UpdateCommandRequestDump(UpdateCommandRequest *msg)
{
	LOG_PRINTK("UpdateCommandRequest: {\n");

	LOG_PRINTK("\tkey: %u\n", msg->key);

	LOG_PRINTK("\tcommand: %s\n", .UpdateCommand_enum_to_string(msg->command));

	LOG_PRINTK("}\n");
}

#endif /* HAS_PROTO_MESSAGE_DUMP */

__weak int HelloRequestCb(const struct device *dev, HelloRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int HelloResponseCb(const struct device *dev, HelloResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ConnectRequestCb(const struct device *dev, ConnectRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ConnectResponseCb(const struct device *dev, ConnectResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int DisconnectRequestCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int DisconnectResponseCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int PingRequestCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int PingResponseCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int DeviceInfoRequestCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int DeviceInfoResponseCb(const struct device *dev, DeviceInfoResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesRequestCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int ListEntitiesDoneResponseCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int SubscribeStatesRequestCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int ListEntitiesBinarySensorResponseCb(const struct device *dev,
					      ListEntitiesBinarySensorResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BinarySensorStateResponseCb(const struct device *dev, BinarySensorStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesCoverResponseCb(const struct device *dev, ListEntitiesCoverResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int CoverStateResponseCb(const struct device *dev, CoverStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int CoverCommandRequestCb(const struct device *dev, CoverCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesFanResponseCb(const struct device *dev, ListEntitiesFanResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int FanStateResponseCb(const struct device *dev, FanStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int FanCommandRequestCb(const struct device *dev, FanCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesLightResponseCb(const struct device *dev, ListEntitiesLightResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int LightStateResponseCb(const struct device *dev, LightStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int LightCommandRequestCb(const struct device *dev, LightCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesSensorResponseCb(const struct device *dev, ListEntitiesSensorResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int SensorStateResponseCb(const struct device *dev, SensorStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesSwitchResponseCb(const struct device *dev, ListEntitiesSwitchResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int SwitchStateResponseCb(const struct device *dev, SwitchStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int SwitchCommandRequestCb(const struct device *dev, SwitchCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesTextSensorResponseCb(const struct device *dev,
					    ListEntitiesTextSensorResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int TextSensorStateResponseCb(const struct device *dev, TextSensorStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int SubscribeLogsRequestCb(const struct device *dev, SubscribeLogsRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int SubscribeLogsResponseCb(const struct device *dev, SubscribeLogsResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int SubscribeHomeassistantServicesRequestCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int HomeassistantServiceResponseCb(const struct device *dev,
					  HomeassistantServiceResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int SubscribeHomeAssistantStatesRequestCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int SubscribeHomeAssistantStateResponseCb(const struct device *dev,
						 SubscribeHomeAssistantStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int HomeAssistantStateResponseCb(const struct device *dev, HomeAssistantStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int GetTimeRequestCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int GetTimeResponseCb(const struct device *dev, GetTimeResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesServicesResponseCb(const struct device *dev,
					  ListEntitiesServicesResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ExecuteServiceRequestCb(const struct device *dev, ExecuteServiceRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesCameraResponseCb(const struct device *dev, ListEntitiesCameraResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int CameraImageResponseCb(const struct device *dev, CameraImageResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int CameraImageRequestCb(const struct device *dev, CameraImageRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesClimateResponseCb(const struct device *dev, ListEntitiesClimateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ClimateStateResponseCb(const struct device *dev, ClimateStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ClimateCommandRequestCb(const struct device *dev, ClimateCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesNumberResponseCb(const struct device *dev, ListEntitiesNumberResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int NumberStateResponseCb(const struct device *dev, NumberStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int NumberCommandRequestCb(const struct device *dev, NumberCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesSelectResponseCb(const struct device *dev, ListEntitiesSelectResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int SelectStateResponseCb(const struct device *dev, SelectStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int SelectCommandRequestCb(const struct device *dev, SelectCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesLockResponseCb(const struct device *dev, ListEntitiesLockResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int LockStateResponseCb(const struct device *dev, LockStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int LockCommandRequestCb(const struct device *dev, LockCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesButtonResponseCb(const struct device *dev, ListEntitiesButtonResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ButtonCommandRequestCb(const struct device *dev, ButtonCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesMediaPlayerResponseCb(const struct device *dev,
					     ListEntitiesMediaPlayerResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int MediaPlayerStateResponseCb(const struct device *dev, MediaPlayerStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int MediaPlayerCommandRequestCb(const struct device *dev, MediaPlayerCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int
SubscribeBluetoothLEAdvertisementsRequestCb(const struct device *dev,
					    SubscribeBluetoothLEAdvertisementsRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothLEAdvertisementResponseCb(const struct device *dev,
					      BluetoothLEAdvertisementResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothLERawAdvertisementsResponseCb(const struct device *dev,
						  BluetoothLERawAdvertisementsResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothDeviceRequestCb(const struct device *dev, BluetoothDeviceRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothDeviceConnectionResponseCb(const struct device *dev,
					       BluetoothDeviceConnectionResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTGetServicesRequestCb(const struct device *dev,
					     BluetoothGATTGetServicesRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTGetServicesResponseCb(const struct device *dev,
					      BluetoothGATTGetServicesResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTGetServicesDoneResponseCb(const struct device *dev,
						  BluetoothGATTGetServicesDoneResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTReadRequestCb(const struct device *dev, BluetoothGATTReadRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTReadResponseCb(const struct device *dev, BluetoothGATTReadResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTWriteRequestCb(const struct device *dev, BluetoothGATTWriteRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTReadDescriptorRequestCb(const struct device *dev,
						BluetoothGATTReadDescriptorRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTWriteDescriptorRequestCb(const struct device *dev,
						 BluetoothGATTWriteDescriptorRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTNotifyRequestCb(const struct device *dev, BluetoothGATTNotifyRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTNotifyDataResponseCb(const struct device *dev,
					     BluetoothGATTNotifyDataResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int SubscribeBluetoothConnectionsFreeRequestCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int BluetoothConnectionsFreeResponseCb(const struct device *dev,
					      BluetoothConnectionsFreeResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTErrorResponseCb(const struct device *dev, BluetoothGATTErrorResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTWriteResponseCb(const struct device *dev, BluetoothGATTWriteResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothGATTNotifyResponseCb(const struct device *dev, BluetoothGATTNotifyResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothDevicePairingResponseCb(const struct device *dev,
					    BluetoothDevicePairingResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int BluetoothDeviceUnpairingResponseCb(const struct device *dev,
					      BluetoothDeviceUnpairingResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int UnsubscribeBluetoothLEAdvertisementsRequestCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int BluetoothDeviceClearCacheResponseCb(const struct device *dev,
					       BluetoothDeviceClearCacheResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int SubscribeVoiceAssistantRequestCb(const struct device *dev,
					    SubscribeVoiceAssistantRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int VoiceAssistantRequestCb(const struct device *dev, VoiceAssistantRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int VoiceAssistantResponseCb(const struct device *dev, VoiceAssistantResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int VoiceAssistantEventResponseCb(const struct device *dev, VoiceAssistantEventResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int VoiceAssistantAudioCb(const struct device *dev, VoiceAssistantAudio *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int VoiceAssistantTimerEventResponseCb(const struct device *dev,
					      VoiceAssistantTimerEventResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int VoiceAssistantAnnounceRequestCb(const struct device *dev,
					   VoiceAssistantAnnounceRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int VoiceAssistantAnnounceFinishedCb(const struct device *dev,
					    VoiceAssistantAnnounceFinished *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int VoiceAssistantConfigurationRequestCb(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -1;
}

__weak int VoiceAssistantConfigurationResponseCb(const struct device *dev,
						 VoiceAssistantConfigurationResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int VoiceAssistantSetConfigurationCb(const struct device *dev,
					    VoiceAssistantSetConfiguration *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesAlarmControlPanelResponseCb(const struct device *dev,
						   ListEntitiesAlarmControlPanelResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int AlarmControlPanelStateResponseCb(const struct device *dev,
					    AlarmControlPanelStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int AlarmControlPanelCommandRequestCb(const struct device *dev,
					     AlarmControlPanelCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesTextResponseCb(const struct device *dev, ListEntitiesTextResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int TextStateResponseCb(const struct device *dev, TextStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int TextCommandRequestCb(const struct device *dev, TextCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesDateResponseCb(const struct device *dev, ListEntitiesDateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int DateStateResponseCb(const struct device *dev, DateStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int DateCommandRequestCb(const struct device *dev, DateCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesTimeResponseCb(const struct device *dev, ListEntitiesTimeResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int TimeStateResponseCb(const struct device *dev, TimeStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int TimeCommandRequestCb(const struct device *dev, TimeCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesEventResponseCb(const struct device *dev, ListEntitiesEventResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int EventResponseCb(const struct device *dev, EventResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesValveResponseCb(const struct device *dev, ListEntitiesValveResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ValveStateResponseCb(const struct device *dev, ValveStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ValveCommandRequestCb(const struct device *dev, ValveCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesDateTimeResponseCb(const struct device *dev,
					  ListEntitiesDateTimeResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int DateTimeStateResponseCb(const struct device *dev, DateTimeStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int DateTimeCommandRequestCb(const struct device *dev, DateTimeCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int ListEntitiesUpdateResponseCb(const struct device *dev, ListEntitiesUpdateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int UpdateStateResponseCb(const struct device *dev, UpdateStateResponse *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

__weak int UpdateCommandRequestCb(const struct device *dev, UpdateCommandRequest *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	return -1;
}

static int esphome_HelloRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	HelloRequest *msg;

	msg = hello_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("HelloRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_HelloRequestDump(msg);
#endif
	ret = HelloRequestCb(dev, msg);
	hello_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_HelloResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	HelloResponse *msg;

	msg = hello_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("HelloResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_HelloResponseDump(msg);
#endif
	ret = HelloResponseCb(dev, msg);
	hello_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ConnectRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ConnectRequest *msg;

	msg = connect_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ConnectRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ConnectRequestDump(msg);
#endif
	ret = ConnectRequestCb(dev, msg);
	connect_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ConnectResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ConnectResponse *msg;

	msg = connect_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ConnectResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ConnectResponseDump(msg);
#endif
	ret = ConnectResponseCb(dev, msg);
	connect_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_DisconnectRequestRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DisconnectRequestDump();
#endif
	return DisconnectRequestCb(dev);
}

static int esphome_DisconnectResponseRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DisconnectResponseDump();
#endif
	return DisconnectResponseCb(dev);
}

static int esphome_PingRequestRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_PingRequestDump();
#endif
	return PingRequestCb(dev);
}

static int esphome_PingResponseRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_PingResponseDump();
#endif
	return PingResponseCb(dev);
}

static int esphome_DeviceInfoRequestRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DeviceInfoRequestDump();
#endif
	return DeviceInfoRequestCb(dev);
}

static int esphome_DeviceInfoResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	DeviceInfoResponse *msg;

	msg = device_info_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("DeviceInfoResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DeviceInfoResponseDump(msg);
#endif
	ret = DeviceInfoResponseCb(dev, msg);
	device_info_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesRequestRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesRequestDump();
#endif
	return ListEntitiesRequestCb(dev);
}

static int esphome_ListEntitiesDoneResponseRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesDoneResponseDump();
#endif
	return ListEntitiesDoneResponseCb(dev);
}

static int esphome_SubscribeStatesRequestRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeStatesRequestDump();
#endif
	return SubscribeStatesRequestCb(dev);
}

static int esphome_ListEntitiesBinarySensorResponseRead(const struct device *dev, uint8_t *data,
							size_t len)
{
	int ret;
	ListEntitiesBinarySensorResponse *msg;

	msg = list_entities_binary_sensor_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesBinarySensorResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesBinarySensorResponseDump(msg);
#endif
	ret = ListEntitiesBinarySensorResponseCb(dev, msg);
	list_entities_binary_sensor_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BinarySensorStateResponseRead(const struct device *dev, uint8_t *data,
						 size_t len)
{
	int ret;
	BinarySensorStateResponse *msg;

	msg = binary_sensor_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BinarySensorStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BinarySensorStateResponseDump(msg);
#endif
	ret = BinarySensorStateResponseCb(dev, msg);
	binary_sensor_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesCoverResponseRead(const struct device *dev, uint8_t *data,
						 size_t len)
{
	int ret;
	ListEntitiesCoverResponse *msg;

	msg = list_entities_cover_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesCoverResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesCoverResponseDump(msg);
#endif
	ret = ListEntitiesCoverResponseCb(dev, msg);
	list_entities_cover_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_CoverStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	CoverStateResponse *msg;

	msg = cover_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("CoverStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_CoverStateResponseDump(msg);
#endif
	ret = CoverStateResponseCb(dev, msg);
	cover_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_CoverCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	CoverCommandRequest *msg;

	msg = cover_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("CoverCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_CoverCommandRequestDump(msg);
#endif
	ret = CoverCommandRequestCb(dev, msg);
	cover_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesFanResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ListEntitiesFanResponse *msg;

	msg = list_entities_fan_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesFanResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesFanResponseDump(msg);
#endif
	ret = ListEntitiesFanResponseCb(dev, msg);
	list_entities_fan_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_FanStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	FanStateResponse *msg;

	msg = fan_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("FanStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_FanStateResponseDump(msg);
#endif
	ret = FanStateResponseCb(dev, msg);
	fan_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_FanCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	FanCommandRequest *msg;

	msg = fan_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("FanCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_FanCommandRequestDump(msg);
#endif
	ret = FanCommandRequestCb(dev, msg);
	fan_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesLightResponseRead(const struct device *dev, uint8_t *data,
						 size_t len)
{
	int ret;
	ListEntitiesLightResponse *msg;

	msg = list_entities_light_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesLightResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesLightResponseDump(msg);
#endif
	ret = ListEntitiesLightResponseCb(dev, msg);
	list_entities_light_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_LightStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	LightStateResponse *msg;

	msg = light_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("LightStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_LightStateResponseDump(msg);
#endif
	ret = LightStateResponseCb(dev, msg);
	light_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_LightCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	LightCommandRequest *msg;

	msg = light_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("LightCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_LightCommandRequestDump(msg);
#endif
	ret = LightCommandRequestCb(dev, msg);
	light_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesSensorResponseRead(const struct device *dev, uint8_t *data,
						  size_t len)
{
	int ret;
	ListEntitiesSensorResponse *msg;

	msg = list_entities_sensor_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesSensorResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesSensorResponseDump(msg);
#endif
	ret = ListEntitiesSensorResponseCb(dev, msg);
	list_entities_sensor_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_SensorStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	SensorStateResponse *msg;

	msg = sensor_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("SensorStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SensorStateResponseDump(msg);
#endif
	ret = SensorStateResponseCb(dev, msg);
	sensor_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesSwitchResponseRead(const struct device *dev, uint8_t *data,
						  size_t len)
{
	int ret;
	ListEntitiesSwitchResponse *msg;

	msg = list_entities_switch_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesSwitchResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesSwitchResponseDump(msg);
#endif
	ret = ListEntitiesSwitchResponseCb(dev, msg);
	list_entities_switch_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_SwitchStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	SwitchStateResponse *msg;

	msg = switch_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("SwitchStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SwitchStateResponseDump(msg);
#endif
	ret = SwitchStateResponseCb(dev, msg);
	switch_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_SwitchCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	SwitchCommandRequest *msg;

	msg = switch_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("SwitchCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SwitchCommandRequestDump(msg);
#endif
	ret = SwitchCommandRequestCb(dev, msg);
	switch_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesTextSensorResponseRead(const struct device *dev, uint8_t *data,
						      size_t len)
{
	int ret;
	ListEntitiesTextSensorResponse *msg;

	msg = list_entities_text_sensor_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesTextSensorResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesTextSensorResponseDump(msg);
#endif
	ret = ListEntitiesTextSensorResponseCb(dev, msg);
	list_entities_text_sensor_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_TextSensorStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	TextSensorStateResponse *msg;

	msg = text_sensor_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("TextSensorStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_TextSensorStateResponseDump(msg);
#endif
	ret = TextSensorStateResponseCb(dev, msg);
	text_sensor_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_SubscribeLogsRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	SubscribeLogsRequest *msg;

	msg = subscribe_logs_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("SubscribeLogsRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeLogsRequestDump(msg);
#endif
	ret = SubscribeLogsRequestCb(dev, msg);
	subscribe_logs_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_SubscribeLogsResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	SubscribeLogsResponse *msg;

	msg = subscribe_logs_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("SubscribeLogsResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeLogsResponseDump(msg);
#endif
	ret = SubscribeLogsResponseCb(dev, msg);
	subscribe_logs_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_SubscribeHomeassistantServicesRequestRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeHomeassistantServicesRequestDump();
#endif
	return SubscribeHomeassistantServicesRequestCb(dev);
}

static int esphome_HomeassistantServiceResponseRead(const struct device *dev, uint8_t *data,
						    size_t len)
{
	int ret;
	HomeassistantServiceResponse *msg;

	msg = homeassistant_service_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("HomeassistantServiceResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_HomeassistantServiceResponseDump(msg);
#endif
	ret = HomeassistantServiceResponseCb(dev, msg);
	homeassistant_service_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_SubscribeHomeAssistantStatesRequestRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeHomeAssistantStatesRequestDump();
#endif
	return SubscribeHomeAssistantStatesRequestCb(dev);
}

static int esphome_SubscribeHomeAssistantStateResponseRead(const struct device *dev, uint8_t *data,
							   size_t len)
{
	int ret;
	SubscribeHomeAssistantStateResponse *msg;

	msg = subscribe_home_assistant_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("SubscribeHomeAssistantStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeHomeAssistantStateResponseDump(msg);
#endif
	ret = SubscribeHomeAssistantStateResponseCb(dev, msg);
	subscribe_home_assistant_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_HomeAssistantStateResponseRead(const struct device *dev, uint8_t *data,
						  size_t len)
{
	int ret;
	HomeAssistantStateResponse *msg;

	msg = home_assistant_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("HomeAssistantStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_HomeAssistantStateResponseDump(msg);
#endif
	ret = HomeAssistantStateResponseCb(dev, msg);
	home_assistant_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_GetTimeRequestRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_GetTimeRequestDump();
#endif
	return GetTimeRequestCb(dev);
}

static int esphome_GetTimeResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	GetTimeResponse *msg;

	msg = get_time_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("GetTimeResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_GetTimeResponseDump(msg);
#endif
	ret = GetTimeResponseCb(dev, msg);
	get_time_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesServicesResponseRead(const struct device *dev, uint8_t *data,
						    size_t len)
{
	int ret;
	ListEntitiesServicesResponse *msg;

	msg = list_entities_services_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesServicesResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesServicesResponseDump(msg);
#endif
	ret = ListEntitiesServicesResponseCb(dev, msg);
	list_entities_services_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ExecuteServiceRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ExecuteServiceRequest *msg;

	msg = execute_service_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ExecuteServiceRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ExecuteServiceRequestDump(msg);
#endif
	ret = ExecuteServiceRequestCb(dev, msg);
	execute_service_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesCameraResponseRead(const struct device *dev, uint8_t *data,
						  size_t len)
{
	int ret;
	ListEntitiesCameraResponse *msg;

	msg = list_entities_camera_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesCameraResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesCameraResponseDump(msg);
#endif
	ret = ListEntitiesCameraResponseCb(dev, msg);
	list_entities_camera_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_CameraImageResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	CameraImageResponse *msg;

	msg = camera_image_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("CameraImageResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_CameraImageResponseDump(msg);
#endif
	ret = CameraImageResponseCb(dev, msg);
	camera_image_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_CameraImageRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	CameraImageRequest *msg;

	msg = camera_image_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("CameraImageRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_CameraImageRequestDump(msg);
#endif
	ret = CameraImageRequestCb(dev, msg);
	camera_image_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesClimateResponseRead(const struct device *dev, uint8_t *data,
						   size_t len)
{
	int ret;
	ListEntitiesClimateResponse *msg;

	msg = list_entities_climate_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesClimateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesClimateResponseDump(msg);
#endif
	ret = ListEntitiesClimateResponseCb(dev, msg);
	list_entities_climate_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ClimateStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ClimateStateResponse *msg;

	msg = climate_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ClimateStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ClimateStateResponseDump(msg);
#endif
	ret = ClimateStateResponseCb(dev, msg);
	climate_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ClimateCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ClimateCommandRequest *msg;

	msg = climate_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ClimateCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ClimateCommandRequestDump(msg);
#endif
	ret = ClimateCommandRequestCb(dev, msg);
	climate_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesNumberResponseRead(const struct device *dev, uint8_t *data,
						  size_t len)
{
	int ret;
	ListEntitiesNumberResponse *msg;

	msg = list_entities_number_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesNumberResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesNumberResponseDump(msg);
#endif
	ret = ListEntitiesNumberResponseCb(dev, msg);
	list_entities_number_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_NumberStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	NumberStateResponse *msg;

	msg = number_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("NumberStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_NumberStateResponseDump(msg);
#endif
	ret = NumberStateResponseCb(dev, msg);
	number_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_NumberCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	NumberCommandRequest *msg;

	msg = number_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("NumberCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_NumberCommandRequestDump(msg);
#endif
	ret = NumberCommandRequestCb(dev, msg);
	number_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesSelectResponseRead(const struct device *dev, uint8_t *data,
						  size_t len)
{
	int ret;
	ListEntitiesSelectResponse *msg;

	msg = list_entities_select_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesSelectResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesSelectResponseDump(msg);
#endif
	ret = ListEntitiesSelectResponseCb(dev, msg);
	list_entities_select_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_SelectStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	SelectStateResponse *msg;

	msg = select_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("SelectStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SelectStateResponseDump(msg);
#endif
	ret = SelectStateResponseCb(dev, msg);
	select_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_SelectCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	SelectCommandRequest *msg;

	msg = select_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("SelectCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SelectCommandRequestDump(msg);
#endif
	ret = SelectCommandRequestCb(dev, msg);
	select_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesLockResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ListEntitiesLockResponse *msg;

	msg = list_entities_lock_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesLockResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesLockResponseDump(msg);
#endif
	ret = ListEntitiesLockResponseCb(dev, msg);
	list_entities_lock_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_LockStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	LockStateResponse *msg;

	msg = lock_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("LockStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_LockStateResponseDump(msg);
#endif
	ret = LockStateResponseCb(dev, msg);
	lock_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_LockCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	LockCommandRequest *msg;

	msg = lock_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("LockCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_LockCommandRequestDump(msg);
#endif
	ret = LockCommandRequestCb(dev, msg);
	lock_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesButtonResponseRead(const struct device *dev, uint8_t *data,
						  size_t len)
{
	int ret;
	ListEntitiesButtonResponse *msg;

	msg = list_entities_button_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesButtonResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesButtonResponseDump(msg);
#endif
	ret = ListEntitiesButtonResponseCb(dev, msg);
	list_entities_button_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ButtonCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ButtonCommandRequest *msg;

	msg = button_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ButtonCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ButtonCommandRequestDump(msg);
#endif
	ret = ButtonCommandRequestCb(dev, msg);
	button_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesMediaPlayerResponseRead(const struct device *dev, uint8_t *data,
						       size_t len)
{
	int ret;
	ListEntitiesMediaPlayerResponse *msg;

	msg = list_entities_media_player_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesMediaPlayerResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesMediaPlayerResponseDump(msg);
#endif
	ret = ListEntitiesMediaPlayerResponseCb(dev, msg);
	list_entities_media_player_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_MediaPlayerStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	MediaPlayerStateResponse *msg;

	msg = media_player_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("MediaPlayerStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_MediaPlayerStateResponseDump(msg);
#endif
	ret = MediaPlayerStateResponseCb(dev, msg);
	media_player_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_MediaPlayerCommandRequestRead(const struct device *dev, uint8_t *data,
						 size_t len)
{
	int ret;
	MediaPlayerCommandRequest *msg;

	msg = media_player_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("MediaPlayerCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_MediaPlayerCommandRequestDump(msg);
#endif
	ret = MediaPlayerCommandRequestCb(dev, msg);
	media_player_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_SubscribeBluetoothLEAdvertisementsRequestRead(const struct device *dev,
								 uint8_t *data, size_t len)
{
	int ret;
	SubscribeBluetoothLEAdvertisementsRequest *msg;

	msg = subscribe_bluetooth_leadvertisements_request__unpack(&esphome_pb_allocator, len,
								   data);
	if (!msg) {
		LOG_ERR("SubscribeBluetoothLEAdvertisementsRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeBluetoothLEAdvertisementsRequestDump(msg);
#endif
	ret = SubscribeBluetoothLEAdvertisementsRequestCb(dev, msg);
	subscribe_bluetooth_leadvertisements_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothLEAdvertisementResponseRead(const struct device *dev, uint8_t *data,
							size_t len)
{
	int ret;
	BluetoothLEAdvertisementResponse *msg;

	msg = bluetooth_leadvertisement_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothLEAdvertisementResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothLEAdvertisementResponseDump(msg);
#endif
	ret = BluetoothLEAdvertisementResponseCb(dev, msg);
	bluetooth_leadvertisement_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothLERawAdvertisementsResponseRead(const struct device *dev, uint8_t *data,
							    size_t len)
{
	int ret;
	BluetoothLERawAdvertisementsResponse *msg;

	msg = bluetooth_leraw_advertisements_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothLERawAdvertisementsResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothLERawAdvertisementsResponseDump(msg);
#endif
	ret = BluetoothLERawAdvertisementsResponseCb(dev, msg);
	bluetooth_leraw_advertisements_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothDeviceRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	BluetoothDeviceRequest *msg;

	msg = bluetooth_device_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothDeviceRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothDeviceRequestDump(msg);
#endif
	ret = BluetoothDeviceRequestCb(dev, msg);
	bluetooth_device_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothDeviceConnectionResponseRead(const struct device *dev, uint8_t *data,
							 size_t len)
{
	int ret;
	BluetoothDeviceConnectionResponse *msg;

	msg = bluetooth_device_connection_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothDeviceConnectionResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothDeviceConnectionResponseDump(msg);
#endif
	ret = BluetoothDeviceConnectionResponseCb(dev, msg);
	bluetooth_device_connection_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTGetServicesRequestRead(const struct device *dev, uint8_t *data,
						       size_t len)
{
	int ret;
	BluetoothGATTGetServicesRequest *msg;

	msg = bluetooth_gattget_services_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTGetServicesRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTGetServicesRequestDump(msg);
#endif
	ret = BluetoothGATTGetServicesRequestCb(dev, msg);
	bluetooth_gattget_services_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTGetServicesResponseRead(const struct device *dev, uint8_t *data,
							size_t len)
{
	int ret;
	BluetoothGATTGetServicesResponse *msg;

	msg = bluetooth_gattget_services_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTGetServicesResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTGetServicesResponseDump(msg);
#endif
	ret = BluetoothGATTGetServicesResponseCb(dev, msg);
	bluetooth_gattget_services_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTGetServicesDoneResponseRead(const struct device *dev, uint8_t *data,
							    size_t len)
{
	int ret;
	BluetoothGATTGetServicesDoneResponse *msg;

	msg = bluetooth_gattget_services_done_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTGetServicesDoneResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTGetServicesDoneResponseDump(msg);
#endif
	ret = BluetoothGATTGetServicesDoneResponseCb(dev, msg);
	bluetooth_gattget_services_done_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTReadRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	BluetoothGATTReadRequest *msg;

	msg = bluetooth_gattread_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTReadRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTReadRequestDump(msg);
#endif
	ret = BluetoothGATTReadRequestCb(dev, msg);
	bluetooth_gattread_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTReadResponseRead(const struct device *dev, uint8_t *data,
						 size_t len)
{
	int ret;
	BluetoothGATTReadResponse *msg;

	msg = bluetooth_gattread_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTReadResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTReadResponseDump(msg);
#endif
	ret = BluetoothGATTReadResponseCb(dev, msg);
	bluetooth_gattread_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTWriteRequestRead(const struct device *dev, uint8_t *data,
						 size_t len)
{
	int ret;
	BluetoothGATTWriteRequest *msg;

	msg = bluetooth_gattwrite_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTWriteRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTWriteRequestDump(msg);
#endif
	ret = BluetoothGATTWriteRequestCb(dev, msg);
	bluetooth_gattwrite_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTReadDescriptorRequestRead(const struct device *dev, uint8_t *data,
							  size_t len)
{
	int ret;
	BluetoothGATTReadDescriptorRequest *msg;

	msg = bluetooth_gattread_descriptor_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTReadDescriptorRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTReadDescriptorRequestDump(msg);
#endif
	ret = BluetoothGATTReadDescriptorRequestCb(dev, msg);
	bluetooth_gattread_descriptor_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTWriteDescriptorRequestRead(const struct device *dev, uint8_t *data,
							   size_t len)
{
	int ret;
	BluetoothGATTWriteDescriptorRequest *msg;

	msg = bluetooth_gattwrite_descriptor_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTWriteDescriptorRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTWriteDescriptorRequestDump(msg);
#endif
	ret = BluetoothGATTWriteDescriptorRequestCb(dev, msg);
	bluetooth_gattwrite_descriptor_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTNotifyRequestRead(const struct device *dev, uint8_t *data,
						  size_t len)
{
	int ret;
	BluetoothGATTNotifyRequest *msg;

	msg = bluetooth_gattnotify_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTNotifyRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTNotifyRequestDump(msg);
#endif
	ret = BluetoothGATTNotifyRequestCb(dev, msg);
	bluetooth_gattnotify_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTNotifyDataResponseRead(const struct device *dev, uint8_t *data,
						       size_t len)
{
	int ret;
	BluetoothGATTNotifyDataResponse *msg;

	msg = bluetooth_gattnotify_data_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTNotifyDataResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTNotifyDataResponseDump(msg);
#endif
	ret = BluetoothGATTNotifyDataResponseCb(dev, msg);
	bluetooth_gattnotify_data_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_SubscribeBluetoothConnectionsFreeRequestRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeBluetoothConnectionsFreeRequestDump();
#endif
	return SubscribeBluetoothConnectionsFreeRequestCb(dev);
}

static int esphome_BluetoothConnectionsFreeResponseRead(const struct device *dev, uint8_t *data,
							size_t len)
{
	int ret;
	BluetoothConnectionsFreeResponse *msg;

	msg = bluetooth_connections_free_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothConnectionsFreeResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothConnectionsFreeResponseDump(msg);
#endif
	ret = BluetoothConnectionsFreeResponseCb(dev, msg);
	bluetooth_connections_free_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTErrorResponseRead(const struct device *dev, uint8_t *data,
						  size_t len)
{
	int ret;
	BluetoothGATTErrorResponse *msg;

	msg = bluetooth_gatterror_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTErrorResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTErrorResponseDump(msg);
#endif
	ret = BluetoothGATTErrorResponseCb(dev, msg);
	bluetooth_gatterror_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTWriteResponseRead(const struct device *dev, uint8_t *data,
						  size_t len)
{
	int ret;
	BluetoothGATTWriteResponse *msg;

	msg = bluetooth_gattwrite_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTWriteResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTWriteResponseDump(msg);
#endif
	ret = BluetoothGATTWriteResponseCb(dev, msg);
	bluetooth_gattwrite_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothGATTNotifyResponseRead(const struct device *dev, uint8_t *data,
						   size_t len)
{
	int ret;
	BluetoothGATTNotifyResponse *msg;

	msg = bluetooth_gattnotify_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothGATTNotifyResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTNotifyResponseDump(msg);
#endif
	ret = BluetoothGATTNotifyResponseCb(dev, msg);
	bluetooth_gattnotify_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothDevicePairingResponseRead(const struct device *dev, uint8_t *data,
						      size_t len)
{
	int ret;
	BluetoothDevicePairingResponse *msg;

	msg = bluetooth_device_pairing_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothDevicePairingResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothDevicePairingResponseDump(msg);
#endif
	ret = BluetoothDevicePairingResponseCb(dev, msg);
	bluetooth_device_pairing_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_BluetoothDeviceUnpairingResponseRead(const struct device *dev, uint8_t *data,
							size_t len)
{
	int ret;
	BluetoothDeviceUnpairingResponse *msg;

	msg = bluetooth_device_unpairing_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothDeviceUnpairingResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothDeviceUnpairingResponseDump(msg);
#endif
	ret = BluetoothDeviceUnpairingResponseCb(dev, msg);
	bluetooth_device_unpairing_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_UnsubscribeBluetoothLEAdvertisementsRequestRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_UnsubscribeBluetoothLEAdvertisementsRequestDump();
#endif
	return UnsubscribeBluetoothLEAdvertisementsRequestCb(dev);
}

static int esphome_BluetoothDeviceClearCacheResponseRead(const struct device *dev, uint8_t *data,
							 size_t len)
{
	int ret;
	BluetoothDeviceClearCacheResponse *msg;

	msg = bluetooth_device_clear_cache_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("BluetoothDeviceClearCacheResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothDeviceClearCacheResponseDump(msg);
#endif
	ret = BluetoothDeviceClearCacheResponseCb(dev, msg);
	bluetooth_device_clear_cache_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_SubscribeVoiceAssistantRequestRead(const struct device *dev, uint8_t *data,
						      size_t len)
{
	int ret;
	SubscribeVoiceAssistantRequest *msg;

	msg = subscribe_voice_assistant_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("SubscribeVoiceAssistantRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeVoiceAssistantRequestDump(msg);
#endif
	ret = SubscribeVoiceAssistantRequestCb(dev, msg);
	subscribe_voice_assistant_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_VoiceAssistantRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	VoiceAssistantRequest *msg;

	msg = voice_assistant_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("VoiceAssistantRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantRequestDump(msg);
#endif
	ret = VoiceAssistantRequestCb(dev, msg);
	voice_assistant_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_VoiceAssistantResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	VoiceAssistantResponse *msg;

	msg = voice_assistant_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("VoiceAssistantResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantResponseDump(msg);
#endif
	ret = VoiceAssistantResponseCb(dev, msg);
	voice_assistant_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_VoiceAssistantEventResponseRead(const struct device *dev, uint8_t *data,
						   size_t len)
{
	int ret;
	VoiceAssistantEventResponse *msg;

	msg = voice_assistant_event_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("VoiceAssistantEventResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantEventResponseDump(msg);
#endif
	ret = VoiceAssistantEventResponseCb(dev, msg);
	voice_assistant_event_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_VoiceAssistantAudioRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	VoiceAssistantAudio *msg;

	msg = voice_assistant_audio__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("VoiceAssistantAudio: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantAudioDump(msg);
#endif
	ret = VoiceAssistantAudioCb(dev, msg);
	voice_assistant_audio__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_VoiceAssistantTimerEventResponseRead(const struct device *dev, uint8_t *data,
							size_t len)
{
	int ret;
	VoiceAssistantTimerEventResponse *msg;

	msg = voice_assistant_timer_event_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("VoiceAssistantTimerEventResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantTimerEventResponseDump(msg);
#endif
	ret = VoiceAssistantTimerEventResponseCb(dev, msg);
	voice_assistant_timer_event_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_VoiceAssistantAnnounceRequestRead(const struct device *dev, uint8_t *data,
						     size_t len)
{
	int ret;
	VoiceAssistantAnnounceRequest *msg;

	msg = voice_assistant_announce_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("VoiceAssistantAnnounceRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantAnnounceRequestDump(msg);
#endif
	ret = VoiceAssistantAnnounceRequestCb(dev, msg);
	voice_assistant_announce_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_VoiceAssistantAnnounceFinishedRead(const struct device *dev, uint8_t *data,
						      size_t len)
{
	int ret;
	VoiceAssistantAnnounceFinished *msg;

	msg = voice_assistant_announce_finished__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("VoiceAssistantAnnounceFinished: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantAnnounceFinishedDump(msg);
#endif
	ret = VoiceAssistantAnnounceFinishedCb(dev, msg);
	voice_assistant_announce_finished__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_VoiceAssistantConfigurationRequestRead(const struct device *dev)
{
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantConfigurationRequestDump();
#endif
	return VoiceAssistantConfigurationRequestCb(dev);
}

static int esphome_VoiceAssistantConfigurationResponseRead(const struct device *dev, uint8_t *data,
							   size_t len)
{
	int ret;
	VoiceAssistantConfigurationResponse *msg;

	msg = voice_assistant_configuration_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("VoiceAssistantConfigurationResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantConfigurationResponseDump(msg);
#endif
	ret = VoiceAssistantConfigurationResponseCb(dev, msg);
	voice_assistant_configuration_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_VoiceAssistantSetConfigurationRead(const struct device *dev, uint8_t *data,
						      size_t len)
{
	int ret;
	VoiceAssistantSetConfiguration *msg;

	msg = voice_assistant_set_configuration__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("VoiceAssistantSetConfiguration: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantSetConfigurationDump(msg);
#endif
	ret = VoiceAssistantSetConfigurationCb(dev, msg);
	voice_assistant_set_configuration__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesAlarmControlPanelResponseRead(const struct device *dev,
							     uint8_t *data, size_t len)
{
	int ret;
	ListEntitiesAlarmControlPanelResponse *msg;

	msg = list_entities_alarm_control_panel_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesAlarmControlPanelResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesAlarmControlPanelResponseDump(msg);
#endif
	ret = ListEntitiesAlarmControlPanelResponseCb(dev, msg);
	list_entities_alarm_control_panel_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_AlarmControlPanelStateResponseRead(const struct device *dev, uint8_t *data,
						      size_t len)
{
	int ret;
	AlarmControlPanelStateResponse *msg;

	msg = alarm_control_panel_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("AlarmControlPanelStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_AlarmControlPanelStateResponseDump(msg);
#endif
	ret = AlarmControlPanelStateResponseCb(dev, msg);
	alarm_control_panel_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_AlarmControlPanelCommandRequestRead(const struct device *dev, uint8_t *data,
						       size_t len)
{
	int ret;
	AlarmControlPanelCommandRequest *msg;

	msg = alarm_control_panel_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("AlarmControlPanelCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_AlarmControlPanelCommandRequestDump(msg);
#endif
	ret = AlarmControlPanelCommandRequestCb(dev, msg);
	alarm_control_panel_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesTextResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ListEntitiesTextResponse *msg;

	msg = list_entities_text_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesTextResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesTextResponseDump(msg);
#endif
	ret = ListEntitiesTextResponseCb(dev, msg);
	list_entities_text_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_TextStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	TextStateResponse *msg;

	msg = text_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("TextStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_TextStateResponseDump(msg);
#endif
	ret = TextStateResponseCb(dev, msg);
	text_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_TextCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	TextCommandRequest *msg;

	msg = text_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("TextCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_TextCommandRequestDump(msg);
#endif
	ret = TextCommandRequestCb(dev, msg);
	text_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesDateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ListEntitiesDateResponse *msg;

	msg = list_entities_date_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesDateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesDateResponseDump(msg);
#endif
	ret = ListEntitiesDateResponseCb(dev, msg);
	list_entities_date_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_DateStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	DateStateResponse *msg;

	msg = date_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("DateStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DateStateResponseDump(msg);
#endif
	ret = DateStateResponseCb(dev, msg);
	date_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_DateCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	DateCommandRequest *msg;

	msg = date_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("DateCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DateCommandRequestDump(msg);
#endif
	ret = DateCommandRequestCb(dev, msg);
	date_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesTimeResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ListEntitiesTimeResponse *msg;

	msg = list_entities_time_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesTimeResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesTimeResponseDump(msg);
#endif
	ret = ListEntitiesTimeResponseCb(dev, msg);
	list_entities_time_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_TimeStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	TimeStateResponse *msg;

	msg = time_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("TimeStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_TimeStateResponseDump(msg);
#endif
	ret = TimeStateResponseCb(dev, msg);
	time_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_TimeCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	TimeCommandRequest *msg;

	msg = time_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("TimeCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_TimeCommandRequestDump(msg);
#endif
	ret = TimeCommandRequestCb(dev, msg);
	time_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesEventResponseRead(const struct device *dev, uint8_t *data,
						 size_t len)
{
	int ret;
	ListEntitiesEventResponse *msg;

	msg = list_entities_event_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesEventResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesEventResponseDump(msg);
#endif
	ret = ListEntitiesEventResponseCb(dev, msg);
	list_entities_event_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_EventResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	EventResponse *msg;

	msg = event_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("EventResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_EventResponseDump(msg);
#endif
	ret = EventResponseCb(dev, msg);
	event_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesValveResponseRead(const struct device *dev, uint8_t *data,
						 size_t len)
{
	int ret;
	ListEntitiesValveResponse *msg;

	msg = list_entities_valve_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesValveResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesValveResponseDump(msg);
#endif
	ret = ListEntitiesValveResponseCb(dev, msg);
	list_entities_valve_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ValveStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ValveStateResponse *msg;

	msg = valve_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ValveStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ValveStateResponseDump(msg);
#endif
	ret = ValveStateResponseCb(dev, msg);
	valve_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ValveCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	ValveCommandRequest *msg;

	msg = valve_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ValveCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ValveCommandRequestDump(msg);
#endif
	ret = ValveCommandRequestCb(dev, msg);
	valve_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesDateTimeResponseRead(const struct device *dev, uint8_t *data,
						    size_t len)
{
	int ret;
	ListEntitiesDateTimeResponse *msg;

	msg = list_entities_date_time_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesDateTimeResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesDateTimeResponseDump(msg);
#endif
	ret = ListEntitiesDateTimeResponseCb(dev, msg);
	list_entities_date_time_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_DateTimeStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	DateTimeStateResponse *msg;

	msg = date_time_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("DateTimeStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DateTimeStateResponseDump(msg);
#endif
	ret = DateTimeStateResponseCb(dev, msg);
	date_time_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_DateTimeCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	DateTimeCommandRequest *msg;

	msg = date_time_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("DateTimeCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DateTimeCommandRequestDump(msg);
#endif
	ret = DateTimeCommandRequestCb(dev, msg);
	date_time_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_ListEntitiesUpdateResponseRead(const struct device *dev, uint8_t *data,
						  size_t len)
{
	int ret;
	ListEntitiesUpdateResponse *msg;

	msg = list_entities_update_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("ListEntitiesUpdateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesUpdateResponseDump(msg);
#endif
	ret = ListEntitiesUpdateResponseCb(dev, msg);
	list_entities_update_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_UpdateStateResponseRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	UpdateStateResponse *msg;

	msg = update_state_response__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("UpdateStateResponse: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_UpdateStateResponseDump(msg);
#endif
	ret = UpdateStateResponseCb(dev, msg);
	update_state_response__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

static int esphome_UpdateCommandRequestRead(const struct device *dev, uint8_t *data, size_t len)
{
	int ret;
	UpdateCommandRequest *msg;

	msg = update_command_request__unpack(&esphome_pb_allocator, len, data);
	if (!msg) {
		LOG_ERR("UpdateCommandRequest: Decode failed\n");
		return -EIO;
	}
#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_UpdateCommandRequestDump(msg);
#endif
	ret = UpdateCommandRequestCb(dev, msg);
	update_command_request__free_unpacked(msg, &esphome_pb_allocator);
	return ret;
}

int HelloRequestWrite(const struct device *dev, HelloRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_HelloRequestDump(msg);
#endif
	len = hello_request__get_packed_size(msg);
	hdr_len = esphome_header_size(1, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(1, len, out);
	hello_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int HelloResponseWrite(const struct device *dev, HelloResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_HelloResponseDump(msg);
#endif
	len = hello_response__get_packed_size(msg);
	hdr_len = esphome_header_size(2, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(2, len, out);
	hello_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ConnectRequestWrite(const struct device *dev, ConnectRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ConnectRequestDump(msg);
#endif
	len = connect_request__get_packed_size(msg);
	hdr_len = esphome_header_size(3, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(3, len, out);
	connect_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ConnectResponseWrite(const struct device *dev, ConnectResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ConnectResponseDump(msg);
#endif
	len = connect_response__get_packed_size(msg);
	hdr_len = esphome_header_size(4, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(4, len, out);
	connect_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int DisconnectRequestWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DisconnectRequestDump();
#endif

	hdr_len = esphome_header_size(5, 0);
	esphome_encode_header(5, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int DisconnectResponseWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DisconnectResponseDump();
#endif

	hdr_len = esphome_header_size(6, 0);
	esphome_encode_header(6, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int PingRequestWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_PingRequestDump();
#endif

	hdr_len = esphome_header_size(7, 0);
	esphome_encode_header(7, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int PingResponseWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_PingResponseDump();
#endif

	hdr_len = esphome_header_size(8, 0);
	esphome_encode_header(8, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int DeviceInfoRequestWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DeviceInfoRequestDump();
#endif

	hdr_len = esphome_header_size(9, 0);
	esphome_encode_header(9, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int DeviceInfoResponseWrite(const struct device *dev, DeviceInfoResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DeviceInfoResponseDump(msg);
#endif
	len = device_info_response__get_packed_size(msg);
	hdr_len = esphome_header_size(10, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(10, len, out);
	device_info_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesRequestWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesRequestDump();
#endif

	hdr_len = esphome_header_size(11, 0);
	esphome_encode_header(11, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int ListEntitiesDoneResponseWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesDoneResponseDump();
#endif

	hdr_len = esphome_header_size(19, 0);
	esphome_encode_header(19, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int SubscribeStatesRequestWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeStatesRequestDump();
#endif

	hdr_len = esphome_header_size(20, 0);
	esphome_encode_header(20, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int ListEntitiesBinarySensorResponseWrite(const struct device *dev,
					  ListEntitiesBinarySensorResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesBinarySensorResponseDump(msg);
#endif
	len = list_entities_binary_sensor_response__get_packed_size(msg);
	hdr_len = esphome_header_size(12, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(12, len, out);
	list_entities_binary_sensor_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BinarySensorStateResponseWrite(const struct device *dev, BinarySensorStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BinarySensorStateResponseDump(msg);
#endif
	len = binary_sensor_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(21, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(21, len, out);
	binary_sensor_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesCoverResponseWrite(const struct device *dev, ListEntitiesCoverResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesCoverResponseDump(msg);
#endif
	len = list_entities_cover_response__get_packed_size(msg);
	hdr_len = esphome_header_size(13, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(13, len, out);
	list_entities_cover_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int CoverStateResponseWrite(const struct device *dev, CoverStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_CoverStateResponseDump(msg);
#endif
	len = cover_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(22, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(22, len, out);
	cover_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int CoverCommandRequestWrite(const struct device *dev, CoverCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_CoverCommandRequestDump(msg);
#endif
	len = cover_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(30, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(30, len, out);
	cover_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesFanResponseWrite(const struct device *dev, ListEntitiesFanResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesFanResponseDump(msg);
#endif
	len = list_entities_fan_response__get_packed_size(msg);
	hdr_len = esphome_header_size(14, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(14, len, out);
	list_entities_fan_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int FanStateResponseWrite(const struct device *dev, FanStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_FanStateResponseDump(msg);
#endif
	len = fan_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(23, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(23, len, out);
	fan_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int FanCommandRequestWrite(const struct device *dev, FanCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_FanCommandRequestDump(msg);
#endif
	len = fan_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(31, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(31, len, out);
	fan_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesLightResponseWrite(const struct device *dev, ListEntitiesLightResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesLightResponseDump(msg);
#endif
	len = list_entities_light_response__get_packed_size(msg);
	hdr_len = esphome_header_size(15, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(15, len, out);
	list_entities_light_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int LightStateResponseWrite(const struct device *dev, LightStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_LightStateResponseDump(msg);
#endif
	len = light_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(24, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(24, len, out);
	light_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int LightCommandRequestWrite(const struct device *dev, LightCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_LightCommandRequestDump(msg);
#endif
	len = light_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(32, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(32, len, out);
	light_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesSensorResponseWrite(const struct device *dev, ListEntitiesSensorResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesSensorResponseDump(msg);
#endif
	len = list_entities_sensor_response__get_packed_size(msg);
	hdr_len = esphome_header_size(16, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(16, len, out);
	list_entities_sensor_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int SensorStateResponseWrite(const struct device *dev, SensorStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SensorStateResponseDump(msg);
#endif
	len = sensor_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(25, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(25, len, out);
	sensor_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesSwitchResponseWrite(const struct device *dev, ListEntitiesSwitchResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesSwitchResponseDump(msg);
#endif
	len = list_entities_switch_response__get_packed_size(msg);
	hdr_len = esphome_header_size(17, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(17, len, out);
	list_entities_switch_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int SwitchStateResponseWrite(const struct device *dev, SwitchStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SwitchStateResponseDump(msg);
#endif
	len = switch_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(26, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(26, len, out);
	switch_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int SwitchCommandRequestWrite(const struct device *dev, SwitchCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SwitchCommandRequestDump(msg);
#endif
	len = switch_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(33, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(33, len, out);
	switch_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesTextSensorResponseWrite(const struct device *dev,
					ListEntitiesTextSensorResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesTextSensorResponseDump(msg);
#endif
	len = list_entities_text_sensor_response__get_packed_size(msg);
	hdr_len = esphome_header_size(18, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(18, len, out);
	list_entities_text_sensor_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int TextSensorStateResponseWrite(const struct device *dev, TextSensorStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_TextSensorStateResponseDump(msg);
#endif
	len = text_sensor_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(27, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(27, len, out);
	text_sensor_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int SubscribeLogsRequestWrite(const struct device *dev, SubscribeLogsRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeLogsRequestDump(msg);
#endif
	len = subscribe_logs_request__get_packed_size(msg);
	hdr_len = esphome_header_size(28, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(28, len, out);
	subscribe_logs_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int SubscribeLogsResponseWrite(const struct device *dev, SubscribeLogsResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeLogsResponseDump(msg);
#endif
	len = subscribe_logs_response__get_packed_size(msg);
	hdr_len = esphome_header_size(29, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(29, len, out);
	subscribe_logs_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int SubscribeHomeassistantServicesRequestWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeHomeassistantServicesRequestDump();
#endif

	hdr_len = esphome_header_size(34, 0);
	esphome_encode_header(34, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int HomeassistantServiceResponseWrite(const struct device *dev, HomeassistantServiceResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_HomeassistantServiceResponseDump(msg);
#endif
	len = homeassistant_service_response__get_packed_size(msg);
	hdr_len = esphome_header_size(35, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(35, len, out);
	homeassistant_service_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int SubscribeHomeAssistantStatesRequestWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeHomeAssistantStatesRequestDump();
#endif

	hdr_len = esphome_header_size(38, 0);
	esphome_encode_header(38, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int SubscribeHomeAssistantStateResponseWrite(const struct device *dev,
					     SubscribeHomeAssistantStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeHomeAssistantStateResponseDump(msg);
#endif
	len = subscribe_home_assistant_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(39, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(39, len, out);
	subscribe_home_assistant_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int HomeAssistantStateResponseWrite(const struct device *dev, HomeAssistantStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_HomeAssistantStateResponseDump(msg);
#endif
	len = home_assistant_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(40, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(40, len, out);
	home_assistant_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int GetTimeRequestWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_GetTimeRequestDump();
#endif

	hdr_len = esphome_header_size(36, 0);
	esphome_encode_header(36, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int GetTimeResponseWrite(const struct device *dev, GetTimeResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_GetTimeResponseDump(msg);
#endif
	len = get_time_response__get_packed_size(msg);
	hdr_len = esphome_header_size(37, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(37, len, out);
	get_time_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesServicesResponseWrite(const struct device *dev, ListEntitiesServicesResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesServicesResponseDump(msg);
#endif
	len = list_entities_services_response__get_packed_size(msg);
	hdr_len = esphome_header_size(41, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(41, len, out);
	list_entities_services_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ExecuteServiceRequestWrite(const struct device *dev, ExecuteServiceRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ExecuteServiceRequestDump(msg);
#endif
	len = execute_service_request__get_packed_size(msg);
	hdr_len = esphome_header_size(42, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(42, len, out);
	execute_service_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesCameraResponseWrite(const struct device *dev, ListEntitiesCameraResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesCameraResponseDump(msg);
#endif
	len = list_entities_camera_response__get_packed_size(msg);
	hdr_len = esphome_header_size(43, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(43, len, out);
	list_entities_camera_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int CameraImageResponseWrite(const struct device *dev, CameraImageResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_CameraImageResponseDump(msg);
#endif
	len = camera_image_response__get_packed_size(msg);
	hdr_len = esphome_header_size(44, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(44, len, out);
	camera_image_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int CameraImageRequestWrite(const struct device *dev, CameraImageRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_CameraImageRequestDump(msg);
#endif
	len = camera_image_request__get_packed_size(msg);
	hdr_len = esphome_header_size(45, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(45, len, out);
	camera_image_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesClimateResponseWrite(const struct device *dev, ListEntitiesClimateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesClimateResponseDump(msg);
#endif
	len = list_entities_climate_response__get_packed_size(msg);
	hdr_len = esphome_header_size(46, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(46, len, out);
	list_entities_climate_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ClimateStateResponseWrite(const struct device *dev, ClimateStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ClimateStateResponseDump(msg);
#endif
	len = climate_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(47, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(47, len, out);
	climate_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ClimateCommandRequestWrite(const struct device *dev, ClimateCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ClimateCommandRequestDump(msg);
#endif
	len = climate_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(48, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(48, len, out);
	climate_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesNumberResponseWrite(const struct device *dev, ListEntitiesNumberResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesNumberResponseDump(msg);
#endif
	len = list_entities_number_response__get_packed_size(msg);
	hdr_len = esphome_header_size(49, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(49, len, out);
	list_entities_number_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int NumberStateResponseWrite(const struct device *dev, NumberStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_NumberStateResponseDump(msg);
#endif
	len = number_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(50, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(50, len, out);
	number_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int NumberCommandRequestWrite(const struct device *dev, NumberCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_NumberCommandRequestDump(msg);
#endif
	len = number_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(51, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(51, len, out);
	number_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesSelectResponseWrite(const struct device *dev, ListEntitiesSelectResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesSelectResponseDump(msg);
#endif
	len = list_entities_select_response__get_packed_size(msg);
	hdr_len = esphome_header_size(52, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(52, len, out);
	list_entities_select_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int SelectStateResponseWrite(const struct device *dev, SelectStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SelectStateResponseDump(msg);
#endif
	len = select_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(53, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(53, len, out);
	select_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int SelectCommandRequestWrite(const struct device *dev, SelectCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SelectCommandRequestDump(msg);
#endif
	len = select_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(54, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(54, len, out);
	select_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesLockResponseWrite(const struct device *dev, ListEntitiesLockResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesLockResponseDump(msg);
#endif
	len = list_entities_lock_response__get_packed_size(msg);
	hdr_len = esphome_header_size(58, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(58, len, out);
	list_entities_lock_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int LockStateResponseWrite(const struct device *dev, LockStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_LockStateResponseDump(msg);
#endif
	len = lock_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(59, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(59, len, out);
	lock_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int LockCommandRequestWrite(const struct device *dev, LockCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_LockCommandRequestDump(msg);
#endif
	len = lock_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(60, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(60, len, out);
	lock_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesButtonResponseWrite(const struct device *dev, ListEntitiesButtonResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesButtonResponseDump(msg);
#endif
	len = list_entities_button_response__get_packed_size(msg);
	hdr_len = esphome_header_size(61, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(61, len, out);
	list_entities_button_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ButtonCommandRequestWrite(const struct device *dev, ButtonCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ButtonCommandRequestDump(msg);
#endif
	len = button_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(62, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(62, len, out);
	button_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesMediaPlayerResponseWrite(const struct device *dev,
					 ListEntitiesMediaPlayerResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesMediaPlayerResponseDump(msg);
#endif
	len = list_entities_media_player_response__get_packed_size(msg);
	hdr_len = esphome_header_size(63, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(63, len, out);
	list_entities_media_player_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int MediaPlayerStateResponseWrite(const struct device *dev, MediaPlayerStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_MediaPlayerStateResponseDump(msg);
#endif
	len = media_player_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(64, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(64, len, out);
	media_player_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int MediaPlayerCommandRequestWrite(const struct device *dev, MediaPlayerCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_MediaPlayerCommandRequestDump(msg);
#endif
	len = media_player_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(65, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(65, len, out);
	media_player_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int SubscribeBluetoothLEAdvertisementsRequestWrite(const struct device *dev,
						   SubscribeBluetoothLEAdvertisementsRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeBluetoothLEAdvertisementsRequestDump(msg);
#endif
	len = subscribe_bluetooth_leadvertisements_request__get_packed_size(msg);
	hdr_len = esphome_header_size(66, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(66, len, out);
	subscribe_bluetooth_leadvertisements_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothLEAdvertisementResponseWrite(const struct device *dev,
					  BluetoothLEAdvertisementResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothLEAdvertisementResponseDump(msg);
#endif
	len = bluetooth_leadvertisement_response__get_packed_size(msg);
	hdr_len = esphome_header_size(67, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(67, len, out);
	bluetooth_leadvertisement_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothLERawAdvertisementsResponseWrite(const struct device *dev,
					      BluetoothLERawAdvertisementsResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothLERawAdvertisementsResponseDump(msg);
#endif
	len = bluetooth_leraw_advertisements_response__get_packed_size(msg);
	hdr_len = esphome_header_size(93, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(93, len, out);
	bluetooth_leraw_advertisements_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothDeviceRequestWrite(const struct device *dev, BluetoothDeviceRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothDeviceRequestDump(msg);
#endif
	len = bluetooth_device_request__get_packed_size(msg);
	hdr_len = esphome_header_size(68, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(68, len, out);
	bluetooth_device_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothDeviceConnectionResponseWrite(const struct device *dev,
					   BluetoothDeviceConnectionResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothDeviceConnectionResponseDump(msg);
#endif
	len = bluetooth_device_connection_response__get_packed_size(msg);
	hdr_len = esphome_header_size(69, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(69, len, out);
	bluetooth_device_connection_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTGetServicesRequestWrite(const struct device *dev,
					 BluetoothGATTGetServicesRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTGetServicesRequestDump(msg);
#endif
	len = bluetooth_gattget_services_request__get_packed_size(msg);
	hdr_len = esphome_header_size(70, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(70, len, out);
	bluetooth_gattget_services_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTGetServicesResponseWrite(const struct device *dev,
					  BluetoothGATTGetServicesResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTGetServicesResponseDump(msg);
#endif
	len = bluetooth_gattget_services_response__get_packed_size(msg);
	hdr_len = esphome_header_size(71, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(71, len, out);
	bluetooth_gattget_services_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTGetServicesDoneResponseWrite(const struct device *dev,
					      BluetoothGATTGetServicesDoneResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTGetServicesDoneResponseDump(msg);
#endif
	len = bluetooth_gattget_services_done_response__get_packed_size(msg);
	hdr_len = esphome_header_size(72, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(72, len, out);
	bluetooth_gattget_services_done_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTReadRequestWrite(const struct device *dev, BluetoothGATTReadRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTReadRequestDump(msg);
#endif
	len = bluetooth_gattread_request__get_packed_size(msg);
	hdr_len = esphome_header_size(73, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(73, len, out);
	bluetooth_gattread_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTReadResponseWrite(const struct device *dev, BluetoothGATTReadResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTReadResponseDump(msg);
#endif
	len = bluetooth_gattread_response__get_packed_size(msg);
	hdr_len = esphome_header_size(74, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(74, len, out);
	bluetooth_gattread_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTWriteRequestWrite(const struct device *dev, BluetoothGATTWriteRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTWriteRequestDump(msg);
#endif
	len = bluetooth_gattwrite_request__get_packed_size(msg);
	hdr_len = esphome_header_size(75, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(75, len, out);
	bluetooth_gattwrite_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTReadDescriptorRequestWrite(const struct device *dev,
					    BluetoothGATTReadDescriptorRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTReadDescriptorRequestDump(msg);
#endif
	len = bluetooth_gattread_descriptor_request__get_packed_size(msg);
	hdr_len = esphome_header_size(76, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(76, len, out);
	bluetooth_gattread_descriptor_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTWriteDescriptorRequestWrite(const struct device *dev,
					     BluetoothGATTWriteDescriptorRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTWriteDescriptorRequestDump(msg);
#endif
	len = bluetooth_gattwrite_descriptor_request__get_packed_size(msg);
	hdr_len = esphome_header_size(77, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(77, len, out);
	bluetooth_gattwrite_descriptor_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTNotifyRequestWrite(const struct device *dev, BluetoothGATTNotifyRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTNotifyRequestDump(msg);
#endif
	len = bluetooth_gattnotify_request__get_packed_size(msg);
	hdr_len = esphome_header_size(78, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(78, len, out);
	bluetooth_gattnotify_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTNotifyDataResponseWrite(const struct device *dev,
					 BluetoothGATTNotifyDataResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTNotifyDataResponseDump(msg);
#endif
	len = bluetooth_gattnotify_data_response__get_packed_size(msg);
	hdr_len = esphome_header_size(79, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(79, len, out);
	bluetooth_gattnotify_data_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int SubscribeBluetoothConnectionsFreeRequestWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeBluetoothConnectionsFreeRequestDump();
#endif

	hdr_len = esphome_header_size(80, 0);
	esphome_encode_header(80, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int BluetoothConnectionsFreeResponseWrite(const struct device *dev,
					  BluetoothConnectionsFreeResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothConnectionsFreeResponseDump(msg);
#endif
	len = bluetooth_connections_free_response__get_packed_size(msg);
	hdr_len = esphome_header_size(81, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(81, len, out);
	bluetooth_connections_free_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTErrorResponseWrite(const struct device *dev, BluetoothGATTErrorResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTErrorResponseDump(msg);
#endif
	len = bluetooth_gatterror_response__get_packed_size(msg);
	hdr_len = esphome_header_size(82, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(82, len, out);
	bluetooth_gatterror_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTWriteResponseWrite(const struct device *dev, BluetoothGATTWriteResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTWriteResponseDump(msg);
#endif
	len = bluetooth_gattwrite_response__get_packed_size(msg);
	hdr_len = esphome_header_size(83, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(83, len, out);
	bluetooth_gattwrite_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothGATTNotifyResponseWrite(const struct device *dev, BluetoothGATTNotifyResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothGATTNotifyResponseDump(msg);
#endif
	len = bluetooth_gattnotify_response__get_packed_size(msg);
	hdr_len = esphome_header_size(84, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(84, len, out);
	bluetooth_gattnotify_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothDevicePairingResponseWrite(const struct device *dev,
					BluetoothDevicePairingResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothDevicePairingResponseDump(msg);
#endif
	len = bluetooth_device_pairing_response__get_packed_size(msg);
	hdr_len = esphome_header_size(85, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(85, len, out);
	bluetooth_device_pairing_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int BluetoothDeviceUnpairingResponseWrite(const struct device *dev,
					  BluetoothDeviceUnpairingResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothDeviceUnpairingResponseDump(msg);
#endif
	len = bluetooth_device_unpairing_response__get_packed_size(msg);
	hdr_len = esphome_header_size(86, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(86, len, out);
	bluetooth_device_unpairing_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int UnsubscribeBluetoothLEAdvertisementsRequestWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_UnsubscribeBluetoothLEAdvertisementsRequestDump();
#endif

	hdr_len = esphome_header_size(87, 0);
	esphome_encode_header(87, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int BluetoothDeviceClearCacheResponseWrite(const struct device *dev,
					   BluetoothDeviceClearCacheResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_BluetoothDeviceClearCacheResponseDump(msg);
#endif
	len = bluetooth_device_clear_cache_response__get_packed_size(msg);
	hdr_len = esphome_header_size(88, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(88, len, out);
	bluetooth_device_clear_cache_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int SubscribeVoiceAssistantRequestWrite(const struct device *dev,
					SubscribeVoiceAssistantRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_SubscribeVoiceAssistantRequestDump(msg);
#endif
	len = subscribe_voice_assistant_request__get_packed_size(msg);
	hdr_len = esphome_header_size(89, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(89, len, out);
	subscribe_voice_assistant_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int VoiceAssistantRequestWrite(const struct device *dev, VoiceAssistantRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantRequestDump(msg);
#endif
	len = voice_assistant_request__get_packed_size(msg);
	hdr_len = esphome_header_size(90, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(90, len, out);
	voice_assistant_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int VoiceAssistantResponseWrite(const struct device *dev, VoiceAssistantResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantResponseDump(msg);
#endif
	len = voice_assistant_response__get_packed_size(msg);
	hdr_len = esphome_header_size(91, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(91, len, out);
	voice_assistant_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int VoiceAssistantEventResponseWrite(const struct device *dev, VoiceAssistantEventResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantEventResponseDump(msg);
#endif
	len = voice_assistant_event_response__get_packed_size(msg);
	hdr_len = esphome_header_size(92, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(92, len, out);
	voice_assistant_event_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int VoiceAssistantAudioWrite(const struct device *dev, VoiceAssistantAudio *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantAudioDump(msg);
#endif
	len = voice_assistant_audio__get_packed_size(msg);
	hdr_len = esphome_header_size(106, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(106, len, out);
	voice_assistant_audio__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int VoiceAssistantTimerEventResponseWrite(const struct device *dev,
					  VoiceAssistantTimerEventResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantTimerEventResponseDump(msg);
#endif
	len = voice_assistant_timer_event_response__get_packed_size(msg);
	hdr_len = esphome_header_size(115, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(115, len, out);
	voice_assistant_timer_event_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int VoiceAssistantAnnounceRequestWrite(const struct device *dev, VoiceAssistantAnnounceRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantAnnounceRequestDump(msg);
#endif
	len = voice_assistant_announce_request__get_packed_size(msg);
	hdr_len = esphome_header_size(119, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(119, len, out);
	voice_assistant_announce_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int VoiceAssistantAnnounceFinishedWrite(const struct device *dev,
					VoiceAssistantAnnounceFinished *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantAnnounceFinishedDump(msg);
#endif
	len = voice_assistant_announce_finished__get_packed_size(msg);
	hdr_len = esphome_header_size(120, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(120, len, out);
	voice_assistant_announce_finished__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int VoiceAssistantConfigurationRequestWrite(const struct device *dev)
{
	size_t hdr_len;
	uint8_t out[8];

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantConfigurationRequestDump();
#endif

	hdr_len = esphome_header_size(121, 0);
	esphome_encode_header(121, 0, out);
	return esphome_rpc_send(dev, out, hdr_len);
}

int VoiceAssistantConfigurationResponseWrite(const struct device *dev,
					     VoiceAssistantConfigurationResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantConfigurationResponseDump(msg);
#endif
	len = voice_assistant_configuration_response__get_packed_size(msg);
	hdr_len = esphome_header_size(122, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(122, len, out);
	voice_assistant_configuration_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int VoiceAssistantSetConfigurationWrite(const struct device *dev,
					VoiceAssistantSetConfiguration *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_VoiceAssistantSetConfigurationDump(msg);
#endif
	len = voice_assistant_set_configuration__get_packed_size(msg);
	hdr_len = esphome_header_size(123, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(123, len, out);
	voice_assistant_set_configuration__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesAlarmControlPanelResponseWrite(const struct device *dev,
					       ListEntitiesAlarmControlPanelResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesAlarmControlPanelResponseDump(msg);
#endif
	len = list_entities_alarm_control_panel_response__get_packed_size(msg);
	hdr_len = esphome_header_size(94, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(94, len, out);
	list_entities_alarm_control_panel_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int AlarmControlPanelStateResponseWrite(const struct device *dev,
					AlarmControlPanelStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_AlarmControlPanelStateResponseDump(msg);
#endif
	len = alarm_control_panel_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(95, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(95, len, out);
	alarm_control_panel_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int AlarmControlPanelCommandRequestWrite(const struct device *dev,
					 AlarmControlPanelCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_AlarmControlPanelCommandRequestDump(msg);
#endif
	len = alarm_control_panel_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(96, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(96, len, out);
	alarm_control_panel_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesTextResponseWrite(const struct device *dev, ListEntitiesTextResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesTextResponseDump(msg);
#endif
	len = list_entities_text_response__get_packed_size(msg);
	hdr_len = esphome_header_size(97, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(97, len, out);
	list_entities_text_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int TextStateResponseWrite(const struct device *dev, TextStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_TextStateResponseDump(msg);
#endif
	len = text_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(98, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(98, len, out);
	text_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int TextCommandRequestWrite(const struct device *dev, TextCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_TextCommandRequestDump(msg);
#endif
	len = text_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(99, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(99, len, out);
	text_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesDateResponseWrite(const struct device *dev, ListEntitiesDateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesDateResponseDump(msg);
#endif
	len = list_entities_date_response__get_packed_size(msg);
	hdr_len = esphome_header_size(100, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(100, len, out);
	list_entities_date_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int DateStateResponseWrite(const struct device *dev, DateStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DateStateResponseDump(msg);
#endif
	len = date_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(101, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(101, len, out);
	date_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int DateCommandRequestWrite(const struct device *dev, DateCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DateCommandRequestDump(msg);
#endif
	len = date_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(102, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(102, len, out);
	date_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesTimeResponseWrite(const struct device *dev, ListEntitiesTimeResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesTimeResponseDump(msg);
#endif
	len = list_entities_time_response__get_packed_size(msg);
	hdr_len = esphome_header_size(103, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(103, len, out);
	list_entities_time_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int TimeStateResponseWrite(const struct device *dev, TimeStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_TimeStateResponseDump(msg);
#endif
	len = time_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(104, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(104, len, out);
	time_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int TimeCommandRequestWrite(const struct device *dev, TimeCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_TimeCommandRequestDump(msg);
#endif
	len = time_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(105, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(105, len, out);
	time_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesEventResponseWrite(const struct device *dev, ListEntitiesEventResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesEventResponseDump(msg);
#endif
	len = list_entities_event_response__get_packed_size(msg);
	hdr_len = esphome_header_size(107, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(107, len, out);
	list_entities_event_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int EventResponseWrite(const struct device *dev, EventResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_EventResponseDump(msg);
#endif
	len = event_response__get_packed_size(msg);
	hdr_len = esphome_header_size(108, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(108, len, out);
	event_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesValveResponseWrite(const struct device *dev, ListEntitiesValveResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesValveResponseDump(msg);
#endif
	len = list_entities_valve_response__get_packed_size(msg);
	hdr_len = esphome_header_size(109, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(109, len, out);
	list_entities_valve_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ValveStateResponseWrite(const struct device *dev, ValveStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ValveStateResponseDump(msg);
#endif
	len = valve_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(110, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(110, len, out);
	valve_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ValveCommandRequestWrite(const struct device *dev, ValveCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ValveCommandRequestDump(msg);
#endif
	len = valve_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(111, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(111, len, out);
	valve_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesDateTimeResponseWrite(const struct device *dev, ListEntitiesDateTimeResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesDateTimeResponseDump(msg);
#endif
	len = list_entities_date_time_response__get_packed_size(msg);
	hdr_len = esphome_header_size(112, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(112, len, out);
	list_entities_date_time_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int DateTimeStateResponseWrite(const struct device *dev, DateTimeStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DateTimeStateResponseDump(msg);
#endif
	len = date_time_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(113, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(113, len, out);
	date_time_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int DateTimeCommandRequestWrite(const struct device *dev, DateTimeCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_DateTimeCommandRequestDump(msg);
#endif
	len = date_time_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(114, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(114, len, out);
	date_time_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int ListEntitiesUpdateResponseWrite(const struct device *dev, ListEntitiesUpdateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_ListEntitiesUpdateResponseDump(msg);
#endif
	len = list_entities_update_response__get_packed_size(msg);
	hdr_len = esphome_header_size(116, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(116, len, out);
	list_entities_update_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int UpdateStateResponseWrite(const struct device *dev, UpdateStateResponse *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_UpdateStateResponseDump(msg);
#endif
	len = update_state_response__get_packed_size(msg);
	hdr_len = esphome_header_size(117, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(117, len, out);
	update_state_response__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

int UpdateCommandRequestWrite(const struct device *dev, UpdateCommandRequest *msg)
{
	int ret;
	size_t len;
	size_t hdr_len;
	uint8_t *out;

#ifdef HAS_PROTO_MESSAGE_DUMP
	esphome_UpdateCommandRequestDump(msg);
#endif
	len = update_command_request__get_packed_size(msg);
	hdr_len = esphome_header_size(118, len);
	out = esphome_pb_allocator.alloc(NULL, len + hdr_len);
	if (!out) {
		return -ENOMEM;
	}
	esphome_encode_header(118, len, out);
	update_command_request__pack(msg, out + hdr_len);
	ret = esphome_rpc_send(dev, out, len + hdr_len);
	esphome_pb_allocator.free(NULL, out);
	return ret;
}

static void *zephyr_alloc(void *allocator_data, size_t size)
{
	return k_malloc(size);
}

static void zephyr_free(void *allocator_data, void *pointer)
{
	k_free(pointer);
}

static int varint_encode(uint64_t val, uint8_t *out)
{
	int i = 0;

	if (val <= 0x7F) {
		if (out) {
			*out = (uint8_t)val;
		}
		return 1;
	}
	while (val) {
		uint8_t temp = val & 0x7F;
		val >>= 7;
		if (val) {
			if (out) {
				out[i] = (temp | 0x80);
			}
		} else {
			if (out) {
				out[i] = temp;
			}
		}
		i++;
	}
	return i;
}

static int read_decode_varint(int fd, uint64_t *value)
{
	uint64_t result = 0;
	uint8_t bitpos = 0;
	uint8_t val;
	int ret;

	do {
		ret = zsock_recv(fd, &val, 1, ZSOCK_MSG_WAITALL);
		if (ret == 0) {
			return -EOF;
		}
		if (bitpos >= 63 && (val & 0xFE) != 0) {
			return -EOVERFLOW;
		}
		result |= (uint64_t)(val & 0x7F) << (uint64_t)bitpos;
		bitpos += 7;
	} while (val & 0x80);

	*value = result;
	return 0;
}

static int esphome_header_size(uint32_t rpc_id, size_t len)
{
	int header_size = 1;

	header_size += varint_encode(len, NULL);
	header_size += varint_encode(rpc_id, NULL);
	return header_size;
}

static int esphome_encode_header(uint32_t rpc_id, size_t len, uint8_t *out)
{
	int header_size = 1;

	out[0] = 0x00;
	header_size += varint_encode(len, out + header_size);
	header_size += varint_encode(rpc_id, out + header_size);

	return 0;
}

static int esphome_rpc_send(const struct device *dev, void *out, size_t len)
{
	struct esphome_rpc_data *rpc_data = dev->data;

	zsock_send(rpc_data->socket, out, len, 0);

	return 0;
}

static int esphome_read_header(int fd, uint32_t *rpc_id, uint32_t *len)
{
	uint64_t val;
	uint8_t byte;
	int ret;

	do {
		ret = zsock_recv(fd, &byte, 1, ZSOCK_MSG_WAITALL);
	} while (ret == 0);

	if (ret < 0) {
		return ret;
	}

	if (byte != 0x00) {
		return -EIO;
	}

	ret = read_decode_varint(fd, &val);
	if (ret) {
		return ret;
	}
	*len = (uint32_t)val;

	ret = read_decode_varint(fd, &val);
	if (ret) {
		return ret;
	}
	*rpc_id = (uint32_t)val;

	return 0;
}

static int esphome_read_request(const struct device *dev)
{
	struct esphome_rpc_data *rpc_data = dev->data;
	int ret;
	uint32_t msg_id;
	size_t len;
	uint8_t *data = NULL;

	LOG_DBG("Waiting for message");
	ret = esphome_read_header(rpc_data->socket, &msg_id, &len);
	if (ret) {
		LOG_ERR("Failed to read message header");
		return ret;
	}

	LOG_DBG("Reading message");
	if (len) {
		data = k_malloc(len);
		if (!data) {
			LOG_ERR("Failed to allocate message buffer");
			return -ENOMEM;
		}

		/* TODO: loop until we receive all the data */
		ret = zsock_recv(rpc_data->socket, data, len, ZSOCK_MSG_WAITALL);
		if (ret != len) {
			LOG_ERR("Failed to read message data");
			return -EIO;
		}
	}

	LOG_DBG("Handling message id %d", msg_id);
	switch (msg_id) {

	case 1:

		return esphome_HelloRequestRead(dev, data, len);

	case 2:

		return esphome_HelloResponseRead(dev, data, len);

	case 3:

		return esphome_ConnectRequestRead(dev, data, len);

	case 4:

		return esphome_ConnectResponseRead(dev, data, len);

	case 5:

		return esphome_DisconnectRequestRead(dev);

	case 6:

		return esphome_DisconnectResponseRead(dev);

	case 7:

		return esphome_PingRequestRead(dev);

	case 8:

		return esphome_PingResponseRead(dev);

	case 9:

		return esphome_DeviceInfoRequestRead(dev);

	case 10:

		return esphome_DeviceInfoResponseRead(dev, data, len);

	case 11:

		return esphome_ListEntitiesRequestRead(dev);

	case 19:

		return esphome_ListEntitiesDoneResponseRead(dev);

	case 20:

		return esphome_SubscribeStatesRequestRead(dev);

	case 12:

		return esphome_ListEntitiesBinarySensorResponseRead(dev, data, len);

	case 21:

		return esphome_BinarySensorStateResponseRead(dev, data, len);

	case 13:

		return esphome_ListEntitiesCoverResponseRead(dev, data, len);

	case 22:

		return esphome_CoverStateResponseRead(dev, data, len);

	case 30:

		return esphome_CoverCommandRequestRead(dev, data, len);

	case 14:

		return esphome_ListEntitiesFanResponseRead(dev, data, len);

	case 23:

		return esphome_FanStateResponseRead(dev, data, len);

	case 31:

		return esphome_FanCommandRequestRead(dev, data, len);

	case 15:

		return esphome_ListEntitiesLightResponseRead(dev, data, len);

	case 24:

		return esphome_LightStateResponseRead(dev, data, len);

	case 32:

		return esphome_LightCommandRequestRead(dev, data, len);

	case 16:

		return esphome_ListEntitiesSensorResponseRead(dev, data, len);

	case 25:

		return esphome_SensorStateResponseRead(dev, data, len);

	case 17:

		return esphome_ListEntitiesSwitchResponseRead(dev, data, len);

	case 26:

		return esphome_SwitchStateResponseRead(dev, data, len);

	case 33:

		return esphome_SwitchCommandRequestRead(dev, data, len);

	case 18:

		return esphome_ListEntitiesTextSensorResponseRead(dev, data, len);

	case 27:

		return esphome_TextSensorStateResponseRead(dev, data, len);

	case 28:

		return esphome_SubscribeLogsRequestRead(dev, data, len);

	case 29:

		return esphome_SubscribeLogsResponseRead(dev, data, len);

	case 34:

		return esphome_SubscribeHomeassistantServicesRequestRead(dev);

	case 35:

		return esphome_HomeassistantServiceResponseRead(dev, data, len);

	case 38:

		return esphome_SubscribeHomeAssistantStatesRequestRead(dev);

	case 39:

		return esphome_SubscribeHomeAssistantStateResponseRead(dev, data, len);

	case 40:

		return esphome_HomeAssistantStateResponseRead(dev, data, len);

	case 36:

		return esphome_GetTimeRequestRead(dev);

	case 37:

		return esphome_GetTimeResponseRead(dev, data, len);

	case 41:

		return esphome_ListEntitiesServicesResponseRead(dev, data, len);

	case 42:

		return esphome_ExecuteServiceRequestRead(dev, data, len);

	case 43:

		return esphome_ListEntitiesCameraResponseRead(dev, data, len);

	case 44:

		return esphome_CameraImageResponseRead(dev, data, len);

	case 45:

		return esphome_CameraImageRequestRead(dev, data, len);

	case 46:

		return esphome_ListEntitiesClimateResponseRead(dev, data, len);

	case 47:

		return esphome_ClimateStateResponseRead(dev, data, len);

	case 48:

		return esphome_ClimateCommandRequestRead(dev, data, len);

	case 49:

		return esphome_ListEntitiesNumberResponseRead(dev, data, len);

	case 50:

		return esphome_NumberStateResponseRead(dev, data, len);

	case 51:

		return esphome_NumberCommandRequestRead(dev, data, len);

	case 52:

		return esphome_ListEntitiesSelectResponseRead(dev, data, len);

	case 53:

		return esphome_SelectStateResponseRead(dev, data, len);

	case 54:

		return esphome_SelectCommandRequestRead(dev, data, len);

	case 58:

		return esphome_ListEntitiesLockResponseRead(dev, data, len);

	case 59:

		return esphome_LockStateResponseRead(dev, data, len);

	case 60:

		return esphome_LockCommandRequestRead(dev, data, len);

	case 61:

		return esphome_ListEntitiesButtonResponseRead(dev, data, len);

	case 62:

		return esphome_ButtonCommandRequestRead(dev, data, len);

	case 63:

		return esphome_ListEntitiesMediaPlayerResponseRead(dev, data, len);

	case 64:

		return esphome_MediaPlayerStateResponseRead(dev, data, len);

	case 65:

		return esphome_MediaPlayerCommandRequestRead(dev, data, len);

	case 66:

		return esphome_SubscribeBluetoothLEAdvertisementsRequestRead(dev, data, len);

	case 67:

		return esphome_BluetoothLEAdvertisementResponseRead(dev, data, len);

	case 93:

		return esphome_BluetoothLERawAdvertisementsResponseRead(dev, data, len);

	case 68:

		return esphome_BluetoothDeviceRequestRead(dev, data, len);

	case 69:

		return esphome_BluetoothDeviceConnectionResponseRead(dev, data, len);

	case 70:

		return esphome_BluetoothGATTGetServicesRequestRead(dev, data, len);

	case 71:

		return esphome_BluetoothGATTGetServicesResponseRead(dev, data, len);

	case 72:

		return esphome_BluetoothGATTGetServicesDoneResponseRead(dev, data, len);

	case 73:

		return esphome_BluetoothGATTReadRequestRead(dev, data, len);

	case 74:

		return esphome_BluetoothGATTReadResponseRead(dev, data, len);

	case 75:

		return esphome_BluetoothGATTWriteRequestRead(dev, data, len);

	case 76:

		return esphome_BluetoothGATTReadDescriptorRequestRead(dev, data, len);

	case 77:

		return esphome_BluetoothGATTWriteDescriptorRequestRead(dev, data, len);

	case 78:

		return esphome_BluetoothGATTNotifyRequestRead(dev, data, len);

	case 79:

		return esphome_BluetoothGATTNotifyDataResponseRead(dev, data, len);

	case 80:

		return esphome_SubscribeBluetoothConnectionsFreeRequestRead(dev);

	case 81:

		return esphome_BluetoothConnectionsFreeResponseRead(dev, data, len);

	case 82:

		return esphome_BluetoothGATTErrorResponseRead(dev, data, len);

	case 83:

		return esphome_BluetoothGATTWriteResponseRead(dev, data, len);

	case 84:

		return esphome_BluetoothGATTNotifyResponseRead(dev, data, len);

	case 85:

		return esphome_BluetoothDevicePairingResponseRead(dev, data, len);

	case 86:

		return esphome_BluetoothDeviceUnpairingResponseRead(dev, data, len);

	case 87:

		return esphome_UnsubscribeBluetoothLEAdvertisementsRequestRead(dev);

	case 88:

		return esphome_BluetoothDeviceClearCacheResponseRead(dev, data, len);

	case 89:

		return esphome_SubscribeVoiceAssistantRequestRead(dev, data, len);

	case 90:

		return esphome_VoiceAssistantRequestRead(dev, data, len);

	case 91:

		return esphome_VoiceAssistantResponseRead(dev, data, len);

	case 92:

		return esphome_VoiceAssistantEventResponseRead(dev, data, len);

	case 106:

		return esphome_VoiceAssistantAudioRead(dev, data, len);

	case 115:

		return esphome_VoiceAssistantTimerEventResponseRead(dev, data, len);

	case 119:

		return esphome_VoiceAssistantAnnounceRequestRead(dev, data, len);

	case 120:

		return esphome_VoiceAssistantAnnounceFinishedRead(dev, data, len);

	case 121:

		return esphome_VoiceAssistantConfigurationRequestRead(dev);

	case 122:

		return esphome_VoiceAssistantConfigurationResponseRead(dev, data, len);

	case 123:

		return esphome_VoiceAssistantSetConfigurationRead(dev, data, len);

	case 94:

		return esphome_ListEntitiesAlarmControlPanelResponseRead(dev, data, len);

	case 95:

		return esphome_AlarmControlPanelStateResponseRead(dev, data, len);

	case 96:

		return esphome_AlarmControlPanelCommandRequestRead(dev, data, len);

	case 97:

		return esphome_ListEntitiesTextResponseRead(dev, data, len);

	case 98:

		return esphome_TextStateResponseRead(dev, data, len);

	case 99:

		return esphome_TextCommandRequestRead(dev, data, len);

	case 100:

		return esphome_ListEntitiesDateResponseRead(dev, data, len);

	case 101:

		return esphome_DateStateResponseRead(dev, data, len);

	case 102:

		return esphome_DateCommandRequestRead(dev, data, len);

	case 103:

		return esphome_ListEntitiesTimeResponseRead(dev, data, len);

	case 104:

		return esphome_TimeStateResponseRead(dev, data, len);

	case 105:

		return esphome_TimeCommandRequestRead(dev, data, len);

	case 107:

		return esphome_ListEntitiesEventResponseRead(dev, data, len);

	case 108:

		return esphome_EventResponseRead(dev, data, len);

	case 109:

		return esphome_ListEntitiesValveResponseRead(dev, data, len);

	case 110:

		return esphome_ValveStateResponseRead(dev, data, len);

	case 111:

		return esphome_ValveCommandRequestRead(dev, data, len);

	case 112:

		return esphome_ListEntitiesDateTimeResponseRead(dev, data, len);

	case 113:

		return esphome_DateTimeStateResponseRead(dev, data, len);

	case 114:

		return esphome_DateTimeCommandRequestRead(dev, data, len);

	case 116:

		return esphome_ListEntitiesUpdateResponseRead(dev, data, len);

	case 117:

		return esphome_UpdateStateResponseRead(dev, data, len);

	case 118:

		return esphome_UpdateCommandRequestRead(dev, data, len);

	default:
		LOG_ERR("Unsupported message id %d", msg_id);
		return -ENOTSUP;
	}

	return 0;
}

int esphome_rpc_service(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	struct esphome_rpc_data *rpc_data = dev->data;
	int port = (int)arg2;

	int opt;
	socklen_t optlen = sizeof(int);
	int server_fd, r, ret;
	static struct sockaddr server_addr;
	char addrstr[INET6_ADDRSTRLEN];

	void *addrp;
	uint16_t *portp;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_sin6(&server_addr)->sin6_family = AF_INET6;
		net_sin6(&server_addr)->sin6_addr = in6addr_any;
		net_sin6(&server_addr)->sin6_port = sys_cpu_to_be16(port);
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_sin(&server_addr)->sin_family = AF_INET;
		net_sin(&server_addr)->sin_addr.s_addr = htonl(INADDR_ANY);
		net_sin(&server_addr)->sin_port = sys_cpu_to_be16(port);
	} else {
		__ASSERT(false, "Neither IPv6 nor IPv4 are enabled");
	}

	r = zsock_socket(server_addr.sa_family, SOCK_STREAM, 0);
	if (r == -1) {
		LOG_DBG("socket() failed (%d)", errno);
		return errno;
	}

	server_fd = r;
	LOG_DBG("server_fd is %d", server_fd);

	ret = zsock_getsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, &optlen);
	if (ret == 0) {
		if (opt) {
			LOG_INF("IPV6_V6ONLY option is on, turning it off.\n");

			opt = 0;
			ret = zsock_setsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, optlen);
			if (ret < 0) {
				LOG_WRN("Cannot turn off IPV6_V6ONLY option\n");
			} else {
				LOG_INF("Sharing same socket between IPv6 and IPv4\n");
			}
		}
	}

	r = zsock_bind(server_fd, &server_addr, sizeof(server_addr));
	if (r == -1) {
		LOG_DBG("bind() failed (%d)", errno);
		zsock_close(server_fd);
		return errno;
	}

	if (server_addr.sa_family == AF_INET6) {
		addrp = &net_sin6(&server_addr)->sin6_addr;
		portp = &net_sin6(&server_addr)->sin6_port;
	} else {
		addrp = &net_sin(&server_addr)->sin_addr;
		portp = &net_sin(&server_addr)->sin_port;
	}

	zsock_inet_ntop(server_addr.sa_family, addrp, addrstr, sizeof(addrstr));
	LOG_DBG("bound to [%s]:%u", addrstr, ntohs(*portp));

	r = zsock_listen(server_fd, 1);
	if (r == -1) {
		LOG_DBG("listen() failed (%d)", errno);
		zsock_close(server_fd);
		return errno;
	}

	LOG_INF("ESPHOME server waits for a connection on "
		"port %d...\n",
		port);

	while (1) {
		struct sockaddr_in6 client_addr;
		socklen_t client_addr_len = sizeof(client_addr);

		rpc_data->socket =
			zsock_accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
		if (rpc_data->socket == -1) {
			LOG_DBG("accept() failed (%d)", errno);
			continue;
		}

		zsock_inet_ntop(server_addr.sa_family, addrp, addrstr, sizeof(addrstr));
		LOG_DBG("accepted connection from [%s]:%u", addrstr, ntohs(*portp));

		while (1) {
			ret = esphome_read_request(dev);
			if (!ret) {
				continue;
			} else {
				goto error;
			}
		}

	error:
		zsock_close(rpc_data->socket);
		LOG_INF("Connection from %s closed\n", addrstr);
	}

	return 0;
}
