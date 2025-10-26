/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ESPHOME_H__
#define __ESPHOME_H__

#include <stdint.h>

struct esphome_config {
	const char *name;
	const char *friendly_name;
	const char *server_info;
	uint32_t api_version_major;
	uint32_t api_version_minor;
	const char *compilation_time;
	const char *project_name;
	const char *project_version;
	const char *model;
	const char *manufacturer;

	const char *password;

	int port;
};

struct esphome_data {
	int socket;
};

#endif /* __ESPHOME__ */
