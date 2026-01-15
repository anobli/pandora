
/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 Alexandre Bailon
 */

#include <zephyr/shell/shell.h>

SHELL_SUBCMD_SET_CREATE(pandora_commands, (pandora));

SHELL_CMD_REGISTER(pandora, &pandora_commands, "Pandora commands", NULL);
