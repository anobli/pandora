/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ESPHome, CONFIG_ESPHOME_LOG_LEVEL);

#include <esphome/components/entity.h>

uint32_t fnv1_hash(const char *str)
{
	uint32_t hash = 2166136261UL;
	unsigned int i;

	for (i = 0; i < (strlen(str) + 1); i++) {
		hash *= 16777619UL;
		hash ^= str[i];
	}
	return hash;
}
int esphome_entity_init(const struct device *api_dev)
{
	STRUCT_SECTION_FOREACH(esphome_entity, entity) {
		entity->data->key = fnv1_hash(entity->config->object_id);
		entity->data->api_dev = api_dev;
	}
	return 0;
}

int _string_copy_safe(char *dest, const char *src, size_t len)
{
	size_t src_len = src ? strlen(src) : 0;
	int ret = 0;

	if (!src || !src_len) {
		dest[0] = '\0';
		return -ENODATA;
	}

	strncpy(dest, src, len);
	if (len <= src_len) {
		ret = -ENOSPC;
	}
	dest[len - 1] = '\0';

	return ret;
}

const struct device *find_device_entity_by_key(uint32_t key)
{
	STRUCT_SECTION_FOREACH(esphome_entity, entity) {
		if (entity->data->key == key) {
			return entity->dev;
		}
	}

	LOG_WRN("No device found matching key %d\n", key);
	return NULL;
}

char *esphome_build_unique_id(const char *base_name, char *buffer, int len)
{
	struct net_if *iface;
	struct net_linkaddr *link_addr;

	// Get the default network interface
	iface = net_if_get_default();
	if (!iface) {
		LOG_ERR("No network interface found\n");
		return (char *)base_name;
	}

	// Get the link layer address (MAC address)
	link_addr = net_if_get_link_addr(iface);
	if (!link_addr) {
		LOG_ERR("No MAC address available\n");
		return (char *)base_name;
	}

	// Get the last 3 bytes of the MAC address
	uint8_t mac_byte1 = link_addr->addr[link_addr->len - 3];
	uint8_t mac_byte2 = link_addr->addr[link_addr->len - 2];
	uint8_t mac_byte3 = link_addr->addr[link_addr->len - 1];

	snprintf(buffer, len, "%02x%02x%02x-%s", mac_byte1, mac_byte2, mac_byte3, base_name);

	return buffer;
}