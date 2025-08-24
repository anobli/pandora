/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>

#include <zephyr/drivers/light.h>

static const struct device *pandora_light_get_dev_by_name(const struct shell *sh, const char *name)
{
	const struct device *dev_list;
	size_t n = z_device_get_all_static(&dev_list);

	for (size_t i = 0; i < n; i++) {
		if (!DEVICE_API_IS(pandora_light, (&dev_list[i]))) {
			continue;
		}
		if (!strcmp(dev_list[i].name, name)) {
			return &dev_list[i];
		}
	}

	shell_print(sh, "Failed to found %s", name);
	return NULL;
}

static int cmd_light_list(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev_list;
	size_t n = z_device_get_all_static(&dev_list);

	for (size_t i = 0; i < n; i++) {
		if (!DEVICE_API_IS(pandora_light, (&dev_list[i]))) {
			continue;
		}
		shell_print(sh, "%s", dev_list[i].name);
	}

	return 0;
}

static int cmd_light_state(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret = 0;

	dev = pandora_light_get_dev_by_name(sh, argv[1]);
	if (!dev) {
		return -ENODEV;
	}

	if (argc == 2) {
		uint8_t state;

		pandora_light_get_state(dev, &state);
		shell_print(sh, "%s", state ? "ON" : "OFF");
	} else {
		if (!strcmp(argv[2], "ON") || !strcmp(argv[2], "1")) {
			ret = pandora_light_set_state(dev, 1);
		} else {
			ret = pandora_light_set_state(dev, 0);
		}
		if (ret) {
			shell_print(sh, "Failed to set state");
		}
	}

	return ret;
}

static int cmd_light_brightness(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret = 0;

	dev = pandora_light_get_dev_by_name(sh, argv[1]);
	if (!dev) {
		return -ENODEV;
	}

	if (argc == 2) {
		uint8_t brightness;

		pandora_light_get_brightness(dev, &brightness);
		shell_print(sh, "%d", brightness);
	} else {
		ret = pandora_light_set_brightness(dev, atoi(argv[2]));
		if (ret) {
			shell_print(sh, "Failed to set brightness");
		}
	}

	return ret;
}

static int cmd_light_color(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t color;
	int ret = 0;

	dev = pandora_light_get_dev_by_name(sh, argv[1]);
	if (!dev) {
		return -ENODEV;
	}

	if (argc == 2) {

		pandora_light_get_color(dev, &color);
		shell_print(sh, "%d", color);
	} else {
		sscanf(argv[2], "%x", &color);
		ret = pandora_light_set_color(dev, color);
		if (ret) {
			shell_print(sh, "Failed to set color");
		}
	}

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_light, SHELL_CMD(list, NULL, "List light devices", cmd_light_list),
	SHELL_CMD_ARG(state, NULL, "Get or set light state", cmd_light_state, 2, 1),
	SHELL_CMD_ARG(brightness, NULL, "Get or set light brightness", cmd_light_brightness, 2, 1),
	SHELL_CMD_ARG(color, NULL, "Get or set light color", cmd_light_color, 2, 1),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(light, &sub_light, "Hermes light commands", NULL);
