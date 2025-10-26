
/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_ESPHOME_CLIENT_RPC_H__
#define __ZEPHYR_ESPHOME_CLIENT_RPC_H__

#include <stdbool.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include "api.pb-c.h"

struct esphome_rpc_data {
	int socket;
};

extern ProtobufCAllocator esphome_pb_allocator;

int HelloRequestCb(const struct device *dev, HelloRequest *msg);
int HelloRequestWrite(const struct device *dev, HelloRequest *msg);

int HelloResponseCb(const struct device *dev, HelloResponse *msg);
int HelloResponseWrite(const struct device *dev, HelloResponse *msg);

int ConnectRequestCb(const struct device *dev, ConnectRequest *msg);
int ConnectRequestWrite(const struct device *dev, ConnectRequest *msg);

int ConnectResponseCb(const struct device *dev, ConnectResponse *msg);
int ConnectResponseWrite(const struct device *dev, ConnectResponse *msg);

int DisconnectRequestCb(const struct device *dev);
int DisconnectRequestWrite(const struct device *dev);

int DisconnectResponseCb(const struct device *dev);
int DisconnectResponseWrite(const struct device *dev);

int PingRequestCb(const struct device *dev);
int PingRequestWrite(const struct device *dev);

int PingResponseCb(const struct device *dev);
int PingResponseWrite(const struct device *dev);

int DeviceInfoRequestCb(const struct device *dev);
int DeviceInfoRequestWrite(const struct device *dev);

int DeviceInfoResponseCb(const struct device *dev, DeviceInfoResponse *msg);
int DeviceInfoResponseWrite(const struct device *dev, DeviceInfoResponse *msg);

int ListEntitiesRequestCb(const struct device *dev);
int ListEntitiesRequestWrite(const struct device *dev);

int ListEntitiesDoneResponseCb(const struct device *dev);
int ListEntitiesDoneResponseWrite(const struct device *dev);

int SubscribeStatesRequestCb(const struct device *dev);
int SubscribeStatesRequestWrite(const struct device *dev);

int ListEntitiesBinarySensorResponseCb(const struct device *dev,
				       ListEntitiesBinarySensorResponse *msg);
int ListEntitiesBinarySensorResponseWrite(const struct device *dev,
					  ListEntitiesBinarySensorResponse *msg);

int BinarySensorStateResponseCb(const struct device *dev, BinarySensorStateResponse *msg);
int BinarySensorStateResponseWrite(const struct device *dev, BinarySensorStateResponse *msg);

int ListEntitiesCoverResponseCb(const struct device *dev, ListEntitiesCoverResponse *msg);
int ListEntitiesCoverResponseWrite(const struct device *dev, ListEntitiesCoverResponse *msg);

int CoverStateResponseCb(const struct device *dev, CoverStateResponse *msg);
int CoverStateResponseWrite(const struct device *dev, CoverStateResponse *msg);

int CoverCommandRequestCb(const struct device *dev, CoverCommandRequest *msg);
int CoverCommandRequestWrite(const struct device *dev, CoverCommandRequest *msg);

int ListEntitiesFanResponseCb(const struct device *dev, ListEntitiesFanResponse *msg);
int ListEntitiesFanResponseWrite(const struct device *dev, ListEntitiesFanResponse *msg);

int FanStateResponseCb(const struct device *dev, FanStateResponse *msg);
int FanStateResponseWrite(const struct device *dev, FanStateResponse *msg);

int FanCommandRequestCb(const struct device *dev, FanCommandRequest *msg);
int FanCommandRequestWrite(const struct device *dev, FanCommandRequest *msg);

int ListEntitiesLightResponseCb(const struct device *dev, ListEntitiesLightResponse *msg);
int ListEntitiesLightResponseWrite(const struct device *dev, ListEntitiesLightResponse *msg);

int LightStateResponseCb(const struct device *dev, LightStateResponse *msg);
int LightStateResponseWrite(const struct device *dev, LightStateResponse *msg);

int LightCommandRequestCb(const struct device *dev, LightCommandRequest *msg);
int LightCommandRequestWrite(const struct device *dev, LightCommandRequest *msg);

int ListEntitiesSensorResponseCb(const struct device *dev, ListEntitiesSensorResponse *msg);
int ListEntitiesSensorResponseWrite(const struct device *dev, ListEntitiesSensorResponse *msg);

int SensorStateResponseCb(const struct device *dev, SensorStateResponse *msg);
int SensorStateResponseWrite(const struct device *dev, SensorStateResponse *msg);

int ListEntitiesSwitchResponseCb(const struct device *dev, ListEntitiesSwitchResponse *msg);
int ListEntitiesSwitchResponseWrite(const struct device *dev, ListEntitiesSwitchResponse *msg);

int SwitchStateResponseCb(const struct device *dev, SwitchStateResponse *msg);
int SwitchStateResponseWrite(const struct device *dev, SwitchStateResponse *msg);

int SwitchCommandRequestCb(const struct device *dev, SwitchCommandRequest *msg);
int SwitchCommandRequestWrite(const struct device *dev, SwitchCommandRequest *msg);

int ListEntitiesTextSensorResponseCb(const struct device *dev, ListEntitiesTextSensorResponse *msg);
int ListEntitiesTextSensorResponseWrite(const struct device *dev,
					ListEntitiesTextSensorResponse *msg);

int TextSensorStateResponseCb(const struct device *dev, TextSensorStateResponse *msg);
int TextSensorStateResponseWrite(const struct device *dev, TextSensorStateResponse *msg);

int SubscribeLogsRequestCb(const struct device *dev, SubscribeLogsRequest *msg);
int SubscribeLogsRequestWrite(const struct device *dev, SubscribeLogsRequest *msg);

int SubscribeLogsResponseCb(const struct device *dev, SubscribeLogsResponse *msg);
int SubscribeLogsResponseWrite(const struct device *dev, SubscribeLogsResponse *msg);

int SubscribeHomeassistantServicesRequestCb(const struct device *dev);
int SubscribeHomeassistantServicesRequestWrite(const struct device *dev);

int HomeassistantServiceResponseCb(const struct device *dev, HomeassistantServiceResponse *msg);
int HomeassistantServiceResponseWrite(const struct device *dev, HomeassistantServiceResponse *msg);

int SubscribeHomeAssistantStatesRequestCb(const struct device *dev);
int SubscribeHomeAssistantStatesRequestWrite(const struct device *dev);

int SubscribeHomeAssistantStateResponseCb(const struct device *dev,
					  SubscribeHomeAssistantStateResponse *msg);
int SubscribeHomeAssistantStateResponseWrite(const struct device *dev,
					     SubscribeHomeAssistantStateResponse *msg);

int HomeAssistantStateResponseCb(const struct device *dev, HomeAssistantStateResponse *msg);
int HomeAssistantStateResponseWrite(const struct device *dev, HomeAssistantStateResponse *msg);

int GetTimeRequestCb(const struct device *dev);
int GetTimeRequestWrite(const struct device *dev);

int GetTimeResponseCb(const struct device *dev, GetTimeResponse *msg);
int GetTimeResponseWrite(const struct device *dev, GetTimeResponse *msg);

int ListEntitiesServicesResponseCb(const struct device *dev, ListEntitiesServicesResponse *msg);
int ListEntitiesServicesResponseWrite(const struct device *dev, ListEntitiesServicesResponse *msg);

int ExecuteServiceRequestCb(const struct device *dev, ExecuteServiceRequest *msg);
int ExecuteServiceRequestWrite(const struct device *dev, ExecuteServiceRequest *msg);

int ListEntitiesCameraResponseCb(const struct device *dev, ListEntitiesCameraResponse *msg);
int ListEntitiesCameraResponseWrite(const struct device *dev, ListEntitiesCameraResponse *msg);

int CameraImageResponseCb(const struct device *dev, CameraImageResponse *msg);
int CameraImageResponseWrite(const struct device *dev, CameraImageResponse *msg);

int CameraImageRequestCb(const struct device *dev, CameraImageRequest *msg);
int CameraImageRequestWrite(const struct device *dev, CameraImageRequest *msg);

int ListEntitiesClimateResponseCb(const struct device *dev, ListEntitiesClimateResponse *msg);
int ListEntitiesClimateResponseWrite(const struct device *dev, ListEntitiesClimateResponse *msg);

int ClimateStateResponseCb(const struct device *dev, ClimateStateResponse *msg);
int ClimateStateResponseWrite(const struct device *dev, ClimateStateResponse *msg);

int ClimateCommandRequestCb(const struct device *dev, ClimateCommandRequest *msg);
int ClimateCommandRequestWrite(const struct device *dev, ClimateCommandRequest *msg);

int ListEntitiesNumberResponseCb(const struct device *dev, ListEntitiesNumberResponse *msg);
int ListEntitiesNumberResponseWrite(const struct device *dev, ListEntitiesNumberResponse *msg);

int NumberStateResponseCb(const struct device *dev, NumberStateResponse *msg);
int NumberStateResponseWrite(const struct device *dev, NumberStateResponse *msg);

int NumberCommandRequestCb(const struct device *dev, NumberCommandRequest *msg);
int NumberCommandRequestWrite(const struct device *dev, NumberCommandRequest *msg);

int ListEntitiesSelectResponseCb(const struct device *dev, ListEntitiesSelectResponse *msg);
int ListEntitiesSelectResponseWrite(const struct device *dev, ListEntitiesSelectResponse *msg);

int SelectStateResponseCb(const struct device *dev, SelectStateResponse *msg);
int SelectStateResponseWrite(const struct device *dev, SelectStateResponse *msg);

int SelectCommandRequestCb(const struct device *dev, SelectCommandRequest *msg);
int SelectCommandRequestWrite(const struct device *dev, SelectCommandRequest *msg);

int ListEntitiesLockResponseCb(const struct device *dev, ListEntitiesLockResponse *msg);
int ListEntitiesLockResponseWrite(const struct device *dev, ListEntitiesLockResponse *msg);

int LockStateResponseCb(const struct device *dev, LockStateResponse *msg);
int LockStateResponseWrite(const struct device *dev, LockStateResponse *msg);

int LockCommandRequestCb(const struct device *dev, LockCommandRequest *msg);
int LockCommandRequestWrite(const struct device *dev, LockCommandRequest *msg);

int ListEntitiesButtonResponseCb(const struct device *dev, ListEntitiesButtonResponse *msg);
int ListEntitiesButtonResponseWrite(const struct device *dev, ListEntitiesButtonResponse *msg);

int ButtonCommandRequestCb(const struct device *dev, ButtonCommandRequest *msg);
int ButtonCommandRequestWrite(const struct device *dev, ButtonCommandRequest *msg);

int ListEntitiesMediaPlayerResponseCb(const struct device *dev,
				      ListEntitiesMediaPlayerResponse *msg);
int ListEntitiesMediaPlayerResponseWrite(const struct device *dev,
					 ListEntitiesMediaPlayerResponse *msg);

int MediaPlayerStateResponseCb(const struct device *dev, MediaPlayerStateResponse *msg);
int MediaPlayerStateResponseWrite(const struct device *dev, MediaPlayerStateResponse *msg);

int MediaPlayerCommandRequestCb(const struct device *dev, MediaPlayerCommandRequest *msg);
int MediaPlayerCommandRequestWrite(const struct device *dev, MediaPlayerCommandRequest *msg);

int SubscribeBluetoothLEAdvertisementsRequestCb(const struct device *dev,
						SubscribeBluetoothLEAdvertisementsRequest *msg);
int SubscribeBluetoothLEAdvertisementsRequestWrite(const struct device *dev,
						   SubscribeBluetoothLEAdvertisementsRequest *msg);

int BluetoothLEAdvertisementResponseCb(const struct device *dev,
				       BluetoothLEAdvertisementResponse *msg);
int BluetoothLEAdvertisementResponseWrite(const struct device *dev,
					  BluetoothLEAdvertisementResponse *msg);

int BluetoothLERawAdvertisementsResponseCb(const struct device *dev,
					   BluetoothLERawAdvertisementsResponse *msg);
int BluetoothLERawAdvertisementsResponseWrite(const struct device *dev,
					      BluetoothLERawAdvertisementsResponse *msg);

int BluetoothDeviceRequestCb(const struct device *dev, BluetoothDeviceRequest *msg);
int BluetoothDeviceRequestWrite(const struct device *dev, BluetoothDeviceRequest *msg);

int BluetoothDeviceConnectionResponseCb(const struct device *dev,
					BluetoothDeviceConnectionResponse *msg);
int BluetoothDeviceConnectionResponseWrite(const struct device *dev,
					   BluetoothDeviceConnectionResponse *msg);

int BluetoothGATTGetServicesRequestCb(const struct device *dev,
				      BluetoothGATTGetServicesRequest *msg);
int BluetoothGATTGetServicesRequestWrite(const struct device *dev,
					 BluetoothGATTGetServicesRequest *msg);

int BluetoothGATTGetServicesResponseCb(const struct device *dev,
				       BluetoothGATTGetServicesResponse *msg);
int BluetoothGATTGetServicesResponseWrite(const struct device *dev,
					  BluetoothGATTGetServicesResponse *msg);

int BluetoothGATTGetServicesDoneResponseCb(const struct device *dev,
					   BluetoothGATTGetServicesDoneResponse *msg);
int BluetoothGATTGetServicesDoneResponseWrite(const struct device *dev,
					      BluetoothGATTGetServicesDoneResponse *msg);

int BluetoothGATTReadRequestCb(const struct device *dev, BluetoothGATTReadRequest *msg);
int BluetoothGATTReadRequestWrite(const struct device *dev, BluetoothGATTReadRequest *msg);

int BluetoothGATTReadResponseCb(const struct device *dev, BluetoothGATTReadResponse *msg);
int BluetoothGATTReadResponseWrite(const struct device *dev, BluetoothGATTReadResponse *msg);

int BluetoothGATTWriteRequestCb(const struct device *dev, BluetoothGATTWriteRequest *msg);
int BluetoothGATTWriteRequestWrite(const struct device *dev, BluetoothGATTWriteRequest *msg);

int BluetoothGATTReadDescriptorRequestCb(const struct device *dev,
					 BluetoothGATTReadDescriptorRequest *msg);
int BluetoothGATTReadDescriptorRequestWrite(const struct device *dev,
					    BluetoothGATTReadDescriptorRequest *msg);

int BluetoothGATTWriteDescriptorRequestCb(const struct device *dev,
					  BluetoothGATTWriteDescriptorRequest *msg);
int BluetoothGATTWriteDescriptorRequestWrite(const struct device *dev,
					     BluetoothGATTWriteDescriptorRequest *msg);

int BluetoothGATTNotifyRequestCb(const struct device *dev, BluetoothGATTNotifyRequest *msg);
int BluetoothGATTNotifyRequestWrite(const struct device *dev, BluetoothGATTNotifyRequest *msg);

int BluetoothGATTNotifyDataResponseCb(const struct device *dev,
				      BluetoothGATTNotifyDataResponse *msg);
int BluetoothGATTNotifyDataResponseWrite(const struct device *dev,
					 BluetoothGATTNotifyDataResponse *msg);

int SubscribeBluetoothConnectionsFreeRequestCb(const struct device *dev);
int SubscribeBluetoothConnectionsFreeRequestWrite(const struct device *dev);

int BluetoothConnectionsFreeResponseCb(const struct device *dev,
				       BluetoothConnectionsFreeResponse *msg);
int BluetoothConnectionsFreeResponseWrite(const struct device *dev,
					  BluetoothConnectionsFreeResponse *msg);

int BluetoothGATTErrorResponseCb(const struct device *dev, BluetoothGATTErrorResponse *msg);
int BluetoothGATTErrorResponseWrite(const struct device *dev, BluetoothGATTErrorResponse *msg);

int BluetoothGATTWriteResponseCb(const struct device *dev, BluetoothGATTWriteResponse *msg);
int BluetoothGATTWriteResponseWrite(const struct device *dev, BluetoothGATTWriteResponse *msg);

int BluetoothGATTNotifyResponseCb(const struct device *dev, BluetoothGATTNotifyResponse *msg);
int BluetoothGATTNotifyResponseWrite(const struct device *dev, BluetoothGATTNotifyResponse *msg);

int BluetoothDevicePairingResponseCb(const struct device *dev, BluetoothDevicePairingResponse *msg);
int BluetoothDevicePairingResponseWrite(const struct device *dev,
					BluetoothDevicePairingResponse *msg);

int BluetoothDeviceUnpairingResponseCb(const struct device *dev,
				       BluetoothDeviceUnpairingResponse *msg);
int BluetoothDeviceUnpairingResponseWrite(const struct device *dev,
					  BluetoothDeviceUnpairingResponse *msg);

int UnsubscribeBluetoothLEAdvertisementsRequestCb(const struct device *dev);
int UnsubscribeBluetoothLEAdvertisementsRequestWrite(const struct device *dev);

int BluetoothDeviceClearCacheResponseCb(const struct device *dev,
					BluetoothDeviceClearCacheResponse *msg);
int BluetoothDeviceClearCacheResponseWrite(const struct device *dev,
					   BluetoothDeviceClearCacheResponse *msg);

int SubscribeVoiceAssistantRequestCb(const struct device *dev, SubscribeVoiceAssistantRequest *msg);
int SubscribeVoiceAssistantRequestWrite(const struct device *dev,
					SubscribeVoiceAssistantRequest *msg);

int VoiceAssistantRequestCb(const struct device *dev, VoiceAssistantRequest *msg);
int VoiceAssistantRequestWrite(const struct device *dev, VoiceAssistantRequest *msg);

int VoiceAssistantResponseCb(const struct device *dev, VoiceAssistantResponse *msg);
int VoiceAssistantResponseWrite(const struct device *dev, VoiceAssistantResponse *msg);

int VoiceAssistantEventResponseCb(const struct device *dev, VoiceAssistantEventResponse *msg);
int VoiceAssistantEventResponseWrite(const struct device *dev, VoiceAssistantEventResponse *msg);

int VoiceAssistantAudioCb(const struct device *dev, VoiceAssistantAudio *msg);
int VoiceAssistantAudioWrite(const struct device *dev, VoiceAssistantAudio *msg);

int VoiceAssistantTimerEventResponseCb(const struct device *dev,
				       VoiceAssistantTimerEventResponse *msg);
int VoiceAssistantTimerEventResponseWrite(const struct device *dev,
					  VoiceAssistantTimerEventResponse *msg);

int VoiceAssistantAnnounceRequestCb(const struct device *dev, VoiceAssistantAnnounceRequest *msg);
int VoiceAssistantAnnounceRequestWrite(const struct device *dev,
				       VoiceAssistantAnnounceRequest *msg);

int VoiceAssistantAnnounceFinishedCb(const struct device *dev, VoiceAssistantAnnounceFinished *msg);
int VoiceAssistantAnnounceFinishedWrite(const struct device *dev,
					VoiceAssistantAnnounceFinished *msg);

int VoiceAssistantConfigurationRequestCb(const struct device *dev);
int VoiceAssistantConfigurationRequestWrite(const struct device *dev);

int VoiceAssistantConfigurationResponseCb(const struct device *dev,
					  VoiceAssistantConfigurationResponse *msg);
int VoiceAssistantConfigurationResponseWrite(const struct device *dev,
					     VoiceAssistantConfigurationResponse *msg);

int VoiceAssistantSetConfigurationCb(const struct device *dev, VoiceAssistantSetConfiguration *msg);
int VoiceAssistantSetConfigurationWrite(const struct device *dev,
					VoiceAssistantSetConfiguration *msg);

int ListEntitiesAlarmControlPanelResponseCb(const struct device *dev,
					    ListEntitiesAlarmControlPanelResponse *msg);
int ListEntitiesAlarmControlPanelResponseWrite(const struct device *dev,
					       ListEntitiesAlarmControlPanelResponse *msg);

int AlarmControlPanelStateResponseCb(const struct device *dev, AlarmControlPanelStateResponse *msg);
int AlarmControlPanelStateResponseWrite(const struct device *dev,
					AlarmControlPanelStateResponse *msg);

int AlarmControlPanelCommandRequestCb(const struct device *dev,
				      AlarmControlPanelCommandRequest *msg);
int AlarmControlPanelCommandRequestWrite(const struct device *dev,
					 AlarmControlPanelCommandRequest *msg);

int ListEntitiesTextResponseCb(const struct device *dev, ListEntitiesTextResponse *msg);
int ListEntitiesTextResponseWrite(const struct device *dev, ListEntitiesTextResponse *msg);

int TextStateResponseCb(const struct device *dev, TextStateResponse *msg);
int TextStateResponseWrite(const struct device *dev, TextStateResponse *msg);

int TextCommandRequestCb(const struct device *dev, TextCommandRequest *msg);
int TextCommandRequestWrite(const struct device *dev, TextCommandRequest *msg);

int ListEntitiesDateResponseCb(const struct device *dev, ListEntitiesDateResponse *msg);
int ListEntitiesDateResponseWrite(const struct device *dev, ListEntitiesDateResponse *msg);

int DateStateResponseCb(const struct device *dev, DateStateResponse *msg);
int DateStateResponseWrite(const struct device *dev, DateStateResponse *msg);

int DateCommandRequestCb(const struct device *dev, DateCommandRequest *msg);
int DateCommandRequestWrite(const struct device *dev, DateCommandRequest *msg);

int ListEntitiesTimeResponseCb(const struct device *dev, ListEntitiesTimeResponse *msg);
int ListEntitiesTimeResponseWrite(const struct device *dev, ListEntitiesTimeResponse *msg);

int TimeStateResponseCb(const struct device *dev, TimeStateResponse *msg);
int TimeStateResponseWrite(const struct device *dev, TimeStateResponse *msg);

int TimeCommandRequestCb(const struct device *dev, TimeCommandRequest *msg);
int TimeCommandRequestWrite(const struct device *dev, TimeCommandRequest *msg);

int ListEntitiesEventResponseCb(const struct device *dev, ListEntitiesEventResponse *msg);
int ListEntitiesEventResponseWrite(const struct device *dev, ListEntitiesEventResponse *msg);

int EventResponseCb(const struct device *dev, EventResponse *msg);
int EventResponseWrite(const struct device *dev, EventResponse *msg);

int ListEntitiesValveResponseCb(const struct device *dev, ListEntitiesValveResponse *msg);
int ListEntitiesValveResponseWrite(const struct device *dev, ListEntitiesValveResponse *msg);

int ValveStateResponseCb(const struct device *dev, ValveStateResponse *msg);
int ValveStateResponseWrite(const struct device *dev, ValveStateResponse *msg);

int ValveCommandRequestCb(const struct device *dev, ValveCommandRequest *msg);
int ValveCommandRequestWrite(const struct device *dev, ValveCommandRequest *msg);

int ListEntitiesDateTimeResponseCb(const struct device *dev, ListEntitiesDateTimeResponse *msg);
int ListEntitiesDateTimeResponseWrite(const struct device *dev, ListEntitiesDateTimeResponse *msg);

int DateTimeStateResponseCb(const struct device *dev, DateTimeStateResponse *msg);
int DateTimeStateResponseWrite(const struct device *dev, DateTimeStateResponse *msg);

int DateTimeCommandRequestCb(const struct device *dev, DateTimeCommandRequest *msg);
int DateTimeCommandRequestWrite(const struct device *dev, DateTimeCommandRequest *msg);

int ListEntitiesUpdateResponseCb(const struct device *dev, ListEntitiesUpdateResponse *msg);
int ListEntitiesUpdateResponseWrite(const struct device *dev, ListEntitiesUpdateResponse *msg);

int UpdateStateResponseCb(const struct device *dev, UpdateStateResponse *msg);
int UpdateStateResponseWrite(const struct device *dev, UpdateStateResponse *msg);

int UpdateCommandRequestCb(const struct device *dev, UpdateCommandRequest *msg);
int UpdateCommandRequestWrite(const struct device *dev, UpdateCommandRequest *msg);

int esphome_rpc_service(void *arg1, void *arg2, void *arg3);

#endif /* __ZEPHYR_ESPHOME_CLIENT_RPC_H__ */