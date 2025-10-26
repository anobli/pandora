#include <zephyr/dfu/mcuboot.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>

#include <zephyr/dfu/flash_img.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/sys/reboot.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ESPHomeOTA);

#include "esphome_ota.h"

#define USE_OTA_VERSION 2
#define OTA_BLOCK_SIZE  8192

int esphome_ota_read_magic(int socket)
{
	uint8_t error_code = 0;
	char magic_bytes[sizeof(MAGIC_BYTES)];
	int ret;

	ret = zsock_recv(socket, magic_bytes, sizeof(MAGIC_BYTES), ZSOCK_MSG_WAITALL);
	if (ret != sizeof(MAGIC_BYTES)) {
		return -EIO;
	}

	if (memcmp(MAGIC_BYTES, magic_bytes, sizeof(MAGIC_BYTES))) {
		error_code = OTA_RESPONSE_ERROR_MAGIC;
		goto error;
	}

	return 0;

error:
	zsock_send(socket, &error_code, 1, 0);
	return -EIO;
}

int esphome_ota_send_version(int socket)
{
	char buf[2] = {OTA_RESPONSE_OK, USE_OTA_VERSION};
	int ret;

	ret = zsock_send(socket, buf, sizeof(buf), 0);
	if (ret != sizeof(buf)) {
		return -EIO;
	}

	return 0;
}

int esphome_ota_read_features(int socket, uint8_t *ota_features)
{
	char buf[1];
	int ret;

	ret = zsock_recv(socket, buf, sizeof(buf), ZSOCK_MSG_WAITALL);
	if (ret != sizeof(buf)) {
		return -EIO;
	}

	*ota_features = buf[0];

	/* Send ack */
	buf[0] = OTA_RESPONSE_HEADER_OK;
	ret = zsock_send(socket, buf, sizeof(buf), 0);
	if (ret != sizeof(buf)) {
		return -EIO;
	}

	return 0;
}

int esphome_ota_send_auth_ok(int socket)
{
	char buf[1];
	int ret;

	/* Send ack */
	buf[0] = OTA_RESPONSE_AUTH_OK;
	ret = zsock_send(socket, buf, sizeof(buf), 0);
	if (ret != sizeof(buf)) {
		return -EIO;
	}

	return 0;
}

int esphome_ota_read_size(int socket, size_t *ota_size)
{
	char buf[4];
	int ret;

	ret = zsock_recv(socket, buf, sizeof(buf), ZSOCK_MSG_WAITALL);
	if (ret != sizeof(buf)) {
		return -EIO;
	}

	*ota_size = 0;
	for (uint8_t i = 0; i < 4; i++) {
		*ota_size <<= 8;
		*ota_size |= (buf[i]) & 0xff;
	}

	return 0;
}

int esphome_ota_send_prepare_ok(int socket)
{
	char buf[1];
	int ret;

	/* Send ack */
	buf[0] = OTA_RESPONSE_UPDATE_PREPARE_OK;
	ret = zsock_send(socket, buf, sizeof(buf), 0);
	if (ret != sizeof(buf)) {
		return -EIO;
	}

	return 0;
}

int esphome_ota_read_md5(int socket, char *md5_buf, int size)
{
	uint8_t error_code = 0;
	int ret;

	ret = zsock_recv(socket, md5_buf, size - 1, ZSOCK_MSG_WAITALL);
	if (ret != (size - 1)) {
		goto error;
	}

	md5_buf[size - 1] = '\0';

	/* Send ack */
	md5_buf[0] = OTA_RESPONSE_BIN_MD5_OK;
	ret = zsock_send(socket, md5_buf, 1, 0);
	if (ret != 1) {
		return -EIO;
	}

	return 0;

error:
	zsock_send(socket, &error_code, 1, 0);
	return -EIO;
}

int esphome_ota_read_data(int socket, char *buf, int size)
{
	uint8_t error_code = 0;
	int ret;

	ret = zsock_recv(socket, buf, size, ZSOCK_MSG_WAITALL);
	if (ret != size) {
		goto error;
	}

	return 0;

error:
	zsock_send(socket, &error_code, 1, 0);
	return -EIO;
}

int esphome_ota_send_data_ack(int socket)
{
	char buf[1] = {OTA_RESPONSE_OK};
	int ret;

	ret = zsock_send(socket, buf, sizeof(buf), 0);
	if (ret != sizeof(buf)) {
		return -EIO;
	}

	return 0;
}

int esphome_ota_read_ack(int socket)
{
	char buf[1];
	int ret;

	ret = zsock_recv(socket, buf, 1, ZSOCK_MSG_WAITALL);
	if (ret != 1) {
		return -EIO;
	}

	if (buf[0] != OTA_RESPONSE_OK) {
		return -EINVAL;
	}

	return 0;
}

int esphome_ota_run(int socket, struct flash_img_context *ctx)
{
	size_t ota_size;
	uint8_t ota_features;
	char buf[1024];
	size_t total = 0;
	size_t size_acknowledged = 0;

	int ret;

	ret = esphome_ota_read_magic(socket);
	if (ret) {
		LOG_ERR("Invalid magic value");
		goto error;
	}

	ret = esphome_ota_send_version(socket);
	if (ret) {
		LOG_ERR("Failed to send version");
		goto error;
	}

	ret = esphome_ota_read_features(socket, &ota_features);
	if (ret) {
		LOG_ERR("Failed to read ota features");
		goto error;
	}

	/* TODO: read password and check password */
	ret = esphome_ota_send_auth_ok(socket);
	if (ret) {
		LOG_ERR("Failed to send auth ok");
		goto error;
	}

	ret = esphome_ota_read_size(socket, &ota_size);
	if (ret) {
		LOG_ERR("Failed to read ota size");
		goto error;
	}

	/* TODO: check memory size */

	ret = esphome_ota_send_prepare_ok(socket);
	if (ret) {
		goto error;
	}

	ret = esphome_ota_read_md5(socket, buf, 32 + 1);
	if (ret) {
		goto error;
	}

	while (total < ota_size) {
		size_t requested = MIN(sizeof(buf), ota_size - total);
		ret = esphome_ota_read_data(socket, buf, requested);
		if (ret) {
			goto error;
		}

		/* TODO: write data to flash */
		bool last = (ota_size - total) <= sizeof(buf) ? true : false;
		if (flash_img_buffered_write(ctx, buf, requested, last) != 0) {
			goto error;
		}

		total += requested;
		while (size_acknowledged + OTA_BLOCK_SIZE <= total ||
		       (total == ota_size && size_acknowledged < ota_size)) {
			buf[0] = OTA_RESPONSE_CHUNK_OK;
			ret = zsock_send(socket, buf, 1, 0);
			if (ret != 1) {
				goto error;
			}
			size_acknowledged += OTA_BLOCK_SIZE;
		}
	}

	buf[0] = OTA_RESPONSE_RECEIVE_OK;
	ret = zsock_send(socket, buf, 1, 0);
	if (ret != 1) {
		goto error;
	}

	/* TODO: check firmware md5, flash write ok, etc */
	boot_request_upgrade(1);

	buf[0] = OTA_RESPONSE_UPDATE_END_OK;
	ret = zsock_send(socket, buf, 1, 0);
	if (ret != 1) {
		goto error;
	}

	ret = esphome_ota_read_ack(socket);
	if (ret) {
		goto error;
	}

	LOG_INF("Rebooting ...");
	sys_reboot(SYS_REBOOT_WARM);

	/* We are not supposed to reach this point */
	return -ENOTSUP;

error:
	return ret;
}

int esphome_ota_service(void *arg1, void *arg2, void *arg3)
{
	int port = 8266;

	int opt;
	socklen_t optlen = sizeof(int);
	int ret;

	int server_fd;
	socklen_t len;
	void *addrp;
	uint16_t *portp;
	struct sockaddr client_addr;
	char addrstr[INET6_ADDRSTRLEN];

	static struct sockaddr server_addr;

	if (!boot_is_img_confirmed()) {
		boot_write_img_confirmed();
	}

	struct flash_img_context ctx;
	ret = flash_img_init(&ctx);
	if (ret != 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_sin6(&server_addr)->sin6_family = AF_INET6;
		net_sin6(&server_addr)->sin6_addr = in6addr_any;
		net_sin6(&server_addr)->sin6_port = sys_cpu_to_be16(port);
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_sin(&server_addr)->sin_family = AF_INET;
		net_sin(&server_addr)->sin_addr.s_addr = htonl(INADDR_ANY);
		net_sin(&server_addr)->sin_port = sys_cpu_to_be16(port);
	} else {
		__ASSERT(false, "Neither IPv6 nor IPv4 are enabled");
	}

	server_fd = zsock_socket(server_addr.sa_family, SOCK_STREAM, 0);
	if (server_fd < 0) {
		LOG_DBG("socket() failed (%d)", errno);
		return errno;
	}

	LOG_DBG("server_fd is %d", server_fd);

	ret = zsock_getsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, &optlen);
	if (ret == 0) {
		if (opt) {
			LOG_INF("IPV6_V6ONLY option is on, turning it off.\n");

			opt = 0;
			ret = zsock_setsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, optlen);
			if (ret < 0) {
				LOG_WRN("Cannot turn off IPV6_V6ONLY option\n");
			} else {
				LOG_INF("Sharing same socket between IPv6 and IPv4\n");
			}
		}
	}

	ret = zsock_bind(server_fd, &server_addr, sizeof(server_addr));
	if (ret < 0) {
		LOG_DBG("bind() failed (%d)", errno);
		zsock_close(server_fd);
		return errno;
	}

	if (server_addr.sa_family == AF_INET6) {
		addrp = &net_sin6(&server_addr)->sin6_addr;
		portp = &net_sin6(&server_addr)->sin6_port;
	} else {
		addrp = &net_sin(&server_addr)->sin_addr;
		portp = &net_sin(&server_addr)->sin_port;
	}

	zsock_inet_ntop(server_addr.sa_family, addrp, addrstr, sizeof(addrstr));
	LOG_DBG("bound to [%s]:%u", addrstr, ntohs(*portp));

	ret = zsock_listen(server_fd, 1);
	if (ret < 0) {
		LOG_DBG("listen() failed (%d)", errno);
		zsock_close(server_fd);
		return errno;
	}

	LOG_INF("OTA server waits for a connection on port %d...\n", port);

	while (1) {
		int socket;

		len = sizeof(client_addr);
		socket = zsock_accept(server_fd, (struct sockaddr *)&client_addr, &len);
		if (socket < 0) {
			LOG_DBG("accept() failed (%d)", errno);
			continue;
		}

		zsock_inet_ntop(server_addr.sa_family, addrp, addrstr, sizeof(addrstr));
		LOG_DBG("accepted connection from [%s]:%u", addrstr, ntohs(*portp));

		ret = esphome_ota_run(socket, &ctx);
		if (ret) {
			LOG_ERR("Downloading and flashing OTA failed!");
		}

		zsock_close(socket);
		LOG_INF("Connection from %s closed\n", addrstr);
	}
	return 0;
}

#define ESPHOME_STACK_SIZE (4096)

K_THREAD_DEFINE(esphome_ota_tid, ESPHOME_STACK_SIZE, esphome_ota_service, NULL, NULL, NULL,
		0 /* todo: set priority */, 0, 0);