
/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 Alexandre Bailon
 */

#include <stdio.h>
#include <zephyr/shell/shell.h>

#include <pandora/components/component.h>
#include <pandora/components/binary_sensor.h>

static const struct shell *g_shell;
PANDORA_CB(binary_sensor, on_state, shell_on_state_cb, NULL)
{
	shell_print(g_shell, "binary_sensor.on_state: %s is %s", dev->name, state ? "ON" : "OFF");
}

static int cmd_binary_sensor_list(const struct shell *sh, size_t argc, char **argv)
{
	STRUCT_SECTION_FOREACH(pandora_component_item, component) {
		if (component->type != PANDORA_COMPONENT_BINARY_SENSOR) {
			continue;
		}

		shell_print(sh, "%s", component->dev->name);
	}

	return 0;
}

static int cmd_binary_sensor_state(const struct shell *sh, size_t argc, char **argv)
{
	int state = 0;
	int ret;

	STRUCT_SECTION_FOREACH(pandora_component_item, component) {
		if (component->type != PANDORA_COMPONENT_BINARY_SENSOR) {
			continue;
		}

		if (strcmp(component->dev->name, argv[1])) {
			continue;
		}

		ret = pandora_binary_sensor_get_state(component->dev, &state);
		if (ret) {
			shell_error(sh, "Failed to get binary_sensor state");
			return ret;
		}
		shell_print(sh, "%s", state ? "ON" : "OFF");
		return 0;
	}

	shell_error(sh, "Invalid binary_sensor name");
	return -EINVAL;
}

static int cmd_binary_sensor_on_state(const struct shell *sh, size_t argc, char **argv)
{
	int enable = 0;

	if (sscanf(argv[1], "%d", &enable) != 1) {
		if (!strcmp(argv[1], "ON")) {
			enable = 1;
		} else if (!strcmp(argv[1], "OFF")) {
			enable = 0;
		} else {
			/* TODO: Print help */
			return -EINVAL;
		}
	}

	/* TODO: Found a better place to do this */
	g_shell = sh;
	if (enable) {
		pandora_cb_enable(binary_sensor, on_state, &shell_on_state_cb);
	} else {
		pandora_cb_disable(binary_sensor, on_state, &shell_on_state_cb);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_binary_sensor,
			       SHELL_CMD(list, NULL, "List binary_sensor devices",
					 cmd_binary_sensor_list),
			       SHELL_CMD_ARG(state, NULL, "Get binary_sensor state",
					     cmd_binary_sensor_state, 2, 0),
			       SHELL_CMD_ARG(on_state, NULL, "Enable or disable on_state",
					     cmd_binary_sensor_on_state, 2, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((pandora), binary_sensor, &sub_binary_sensor, "Pandora binary_sensor commandes\n",
		 NULL, 0, 0);
