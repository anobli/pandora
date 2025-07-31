#include <errno.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(narada, CONFIG_NARADA_LOG_LEVEL);

#include <narada/narada.h>

int narada_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	struct narada_device *dev = NARADA_CHOSEN_DEVICE;
	const char *next;
	int ret;

	if (settings_name_steq(name, "ip", &next) && !next) {
		char ip[INET6_ADDRSTRLEN];

		if (len >= INET6_ADDRSTRLEN) {
			return -EINVAL;
		}

		ret = read_cb(cb_arg, &ip, len);
		if (ret < 0) {
			return ret;
		}
		ip[len] = '\0';

		return narada_client_set_ipv4(&dev->client, ip, 5683);
	}

	if (settings_name_steq(name, "ip6", &next) && !next) {
		char ip6[INET6_ADDRSTRLEN];

		if (len > INET6_ADDRSTRLEN) {
			return -EINVAL;
		}

		ret = read_cb(cb_arg, &ip6, len);
		if (ret < 0) {
			return ret;
		}
		ip6[len] = '\0';

		return narada_client_set_ipv6(&dev->client, ip6, 5683);
	}

	return -ENOENT;
}

static int narada_handle_export(int (*storage_func)(const char *name, const void *value,
						    size_t val_len))
{

	struct narada_device *dev = NARADA_CHOSEN_DEVICE;
	int ret;

	if (dev->client.sa.sa_family == AF_INET) {
		char ip_str[INET_ADDRSTRLEN];
		uint16_t port;

		ret = narada_client_get_ipv4(&dev->client, ip_str, &port);
		if (ret) {
			return ret;
		}

		ret = storage_func("narada/ip", ip_str, strlen(ip_str) + 1);
		if (ret) {
			LOG_ERR("Failed to save ipv4");
			return ret;
		}
	}

	if (dev->client.sa.sa_family == AF_INET6) {
		char ip_str[INET6_ADDRSTRLEN];
		uint16_t port;

		ret = narada_client_get_ipv6(&dev->client, ip_str, &port);
		if (ret) {
			return ret;
		}

		ret = storage_func("narada/ip6", ip_str, strlen(ip_str) + 1);
		if (ret) {
			LOG_ERR("Failed to save ipv6");
			return ret;
		}
	}

	return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(narada, "narada", NULL, narada_handle_set, NULL,
			       narada_handle_export);

int narada_settings_load(void)
{
	int ret;

	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Settings initialization failed (err %d)", ret);
		return ret;
	}

	return settings_load();
}

int narada_settings_save(void)
{
	return settings_save();
}
