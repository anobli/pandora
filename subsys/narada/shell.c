#include <zephyr/shell/shell.h>
#include <zephyr/settings/settings.h>

#include <narada/narada.h>
#include <narada/settings.h>

static int narada_shell_ip(const struct shell *sh, size_t argc, char *argv[])
{
	struct narada_device *dev = NARADA_CHOSEN_DEVICE;
	int ret;

	if (argc > 1) {
		ret = narada_client_set_ipv4(&dev->client, argv[1], NARADA_PORT);
		//		if (!ret) {
		//			settings_save_one("narada/ip", argv[1], strlen(argv[1]) +
		// 1);
		//		}
	} else {
		char ip_str[INET_ADDRSTRLEN];
		uint16_t port;

		ret = narada_client_get_ipv4(&dev->client, ip_str, &port);
		if (ret) {
			shell_print(sh, "IPv4 has not been assigned to client");
			return ret;
		}
		shell_print(sh, "%s", ip_str);
	}

	return ret;
}

static int narada_shell_ipv6(const struct shell *sh, size_t argc, char *argv[])
{
	struct narada_device *dev = NARADA_CHOSEN_DEVICE;
	int ret;

	if (argc > 1) {
		ret = narada_client_set_ipv6(&dev->client, argv[1], NARADA_PORT);
		if (ret == 0) {
			settings_save_one("narada/ip6", argv[1], strlen(argv[1]) + 1);
		}
	} else {
		char ip_str[INET6_ADDRSTRLEN];
		uint16_t port;

		ret = narada_client_get_ipv6(&dev->client, ip_str, &port);
		if (ret) {
			shell_print(sh, "IPv6 has not been assigned to client");
			return ret;
		}

		shell_print(sh, "%s", ip_str);
	}

	return ret;
}

static int narada_shell_settings_load(const struct shell *sh, size_t argc, char *argv[])
{
	return narada_settings_load();
}

static int narada_shell_settings_save(const struct shell *sh, size_t argc, char *argv[])
{
	return narada_settings_save();
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_settings,
			       SHELL_CMD(load, NULL, "Initialize client from settings",
					 narada_shell_settings_load),
			       SHELL_CMD(save, NULL, "Save client settings",
					 narada_shell_settings_save),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_narada, SHELL_CMD_ARG(ipv4, NULL, "Get or set the client IPv4", narada_shell_ip, 1, 1),
	SHELL_CMD_ARG(ipv6, NULL, "Get or set the client IPv6", narada_shell_ipv6, 1, 1),
	SHELL_CMD(settings, &sub_settings, "Settings commands", NULL), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(narada, &sub_narada, "Narada commands", NULL);
