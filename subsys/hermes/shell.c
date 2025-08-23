#include <zephyr/shell/shell.h>

#include <hermes/settings.h>

static int cmd_settings_save_all(const struct shell *sh, size_t argc, char **argv)
{
	return hermes_settings_save_all();
}

static int cmd_settings_erase_all(const struct shell *sh, size_t argc, char **argv)
{
	return hermes_settings_erase_all();
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_settings,
			       SHELL_CMD(save, NULL, "Save all settings", cmd_settings_save_all),
			       SHELL_CMD(reset, NULL, "Erase all settings", cmd_settings_erase_all),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(hermes, &sub_settings, "Hermes commands", NULL);
