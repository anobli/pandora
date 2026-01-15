
/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 Alexandre Bailon
 */

#include <stdio.h>
#include <zephyr/shell/shell.h>

#include <pandora/components/component.h>
#include <pandora/components/switch.h>

static int cmd_switch_list(const struct shell *sh, size_t argc, char **argv)
{
	STRUCT_SECTION_FOREACH(pandora_component_item, component) {
		if (component->type != PANDORA_COMPONENT_SWITCH) {
			continue;
		}

		shell_print(sh, "%s", component->dev->name);
	}

	return 0;
}

static int cmd_switch_state(const struct shell *sh, size_t argc, char **argv)
{
	int state = 0;
	int ret;

	STRUCT_SECTION_FOREACH(pandora_component_item, component) {
		if (component->type != PANDORA_COMPONENT_SWITCH) {
			continue;
		}

		if (strcmp(component->dev->name, argv[1])) {
			continue;
		}

		if (argc > 2) {
			if (sscanf(argv[2], "%d", &state) != 1) {
				if (!strcmp(argv[2], "ON")) {
					state = 1;
				} else if (!strcmp(argv[2], "OFF")) {
					state = 0;
				} else {
					/* TODO: Print help */
					return -EINVAL;
				}
			}

			ret = pandora_switch_set_state(component->dev, state);
			if (ret) {
				shell_error(sh, "Failed to set switch state");
				return ret;
			}
		}

		ret = pandora_switch_get_state(component->dev, &state);
		if (ret) {
			shell_error(sh, "Failed to get switch state");
			return ret;
		}
		shell_print(sh, "%s", state ? "ON" : "OFF");
		return 0;
	}

	shell_error(sh, "Invalid switch name");
	return -EINVAL;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_switch,
			       SHELL_CMD(list, NULL, "List switch devices", cmd_switch_list),
			       SHELL_CMD_ARG(state, NULL, "Get or set switch state",
					     cmd_switch_state, 2, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((pandora), switch, &sub_switch, "Pandora switch commandes\n", NULL, 0, 0);
