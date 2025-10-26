/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ESPHOME_SAMPLE);

int ha_btn_1_on_push_cb(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_INF("on_push: ha-btn-1\n");

	return 0;
}

int ha_btn_2_on_push_cb(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_INF("on_push: ha-btn-2\n");

	return 0;
}

int main(void)
{
	return 0;
}
