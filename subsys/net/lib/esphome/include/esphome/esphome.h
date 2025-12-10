/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ESPHOME_H__
#define __ESPHOME_H__

#include <stdint.h>

#define ESPHOME_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nabucasa_esphome)

struct esphome_config {
	const char *name;
	const char *friendly_name;
	const char *compilation_time;
	const char *project_name;
	const char *project_version;
	const char *model;
	const char *manufacturer;
};

struct esphome_data {
	int socket;
};

const struct esphome_config *esphome_get_config(void);

#endif /* __ESPHOME__ */
