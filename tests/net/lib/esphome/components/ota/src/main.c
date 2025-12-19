/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esphome_ota.h>

#ifndef CONFIG_ZTEST
int main(void)
{
	return 0;
}
#else

#include <stdarg.h>

#include <zephyr/ztest.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/flash_img.h>

#include <esphome_ota.h>

/* Structure to hold multiple recv buffers */
struct recv_buffer_entry {
	const uint8_t *data;
	size_t len;
};

/* Test state for simulating socket I/O */
static struct {
	/* Single buffer mode (legacy) */
	uint8_t recv_buffer[1024];
	size_t recv_len;
	size_t recv_pos;

	/* Multi-buffer mode */
	struct recv_buffer_entry *recv_buffers;
	size_t recv_buffer_count;
	size_t recv_buffer_index;
	size_t recv_buffer_pos;

	uint8_t send_buffer[8192];
	size_t send_len;

	int recv_error;
	int send_error;
} test_socket_state;

/* Test implementation of recv - reads from test buffer(s) */
static int test_recv(int socket, void *data, size_t len, int flags)
{
	if (test_socket_state.recv_error) {
		return test_socket_state.recv_error;
	}

	/* Multi-buffer mode */
	if (test_socket_state.recv_buffers != NULL) {
		/* Check if we've exhausted all buffers */
		if (test_socket_state.recv_buffer_index >= test_socket_state.recv_buffer_count) {
			return 0; /* No more data */
		}

		struct recv_buffer_entry *current =
			&test_socket_state.recv_buffers[test_socket_state.recv_buffer_index];
		size_t available = current->len - test_socket_state.recv_buffer_pos;
		size_t to_copy = (len < available) ? len : available;

		if (to_copy == 0) {
			/* Move to next buffer */
			test_socket_state.recv_buffer_index++;
			test_socket_state.recv_buffer_pos = 0;
			/* Recursively try the next buffer */
			return test_recv(socket, data, len, flags);
		}

		memcpy(data, &current->data[test_socket_state.recv_buffer_pos], to_copy);
		test_socket_state.recv_buffer_pos += to_copy;

		/* If current buffer is exhausted, move to next */
		if (test_socket_state.recv_buffer_pos >= current->len) {
			test_socket_state.recv_buffer_index++;
			test_socket_state.recv_buffer_pos = 0;
		}

		return to_copy;
	}

	/* Single buffer mode (legacy) */
	size_t available = test_socket_state.recv_len - test_socket_state.recv_pos;
	size_t to_copy = (len < available) ? len : available;

	if (to_copy == 0) {
		return 0;
	}

	memcpy(data, &test_socket_state.recv_buffer[test_socket_state.recv_pos], to_copy);
	test_socket_state.recv_pos += to_copy;

	return to_copy;
}

/* Test implementation of send - writes to test buffer */
static int test_send(int socket, void *data, size_t len, int flags)
{
	if (test_socket_state.send_error) {
		return test_socket_state.send_error;
	}

	size_t available = sizeof(test_socket_state.send_buffer) - test_socket_state.send_len;
	size_t to_copy = (len < available) ? len : available;

	if (to_copy == 0) {
		return -ENOMEM;
	}

	memcpy(&test_socket_state.send_buffer[test_socket_state.send_len], data, to_copy);
	test_socket_state.send_len += to_copy;

	return to_copy;
}

/* Helper to reset test state */
static void reset_test_socket_state(void)
{
	memset(&test_socket_state, 0, sizeof(test_socket_state));
}

/* Helper to prepare data in recv buffer (single buffer mode) */
static void prepare_recv_data(const uint8_t *data, size_t len)
{
	memcpy(test_socket_state.recv_buffer, data, len);
	test_socket_state.recv_len = len;
	test_socket_state.recv_pos = 0;
}

/* Test fixture */
struct esphome_ota_tests_fixture {
	int socket_fd;
};

static void *ota_tests_setup(void)
{
	static struct esphome_ota_tests_fixture fixture;

	/* Set up test function pointers */
	esphome_ota_test_recv = test_recv;
	esphome_ota_test_send = test_send;

	fixture.socket_fd = 1; /* Fake socket descriptor */

	return &fixture;
}

static void ota_tests_before(void *fixture)
{
	/* Reset test state before each test */
	reset_test_socket_state();
}

ZTEST_SUITE(esphome_ota_tests, NULL, ota_tests_setup, ota_tests_before, NULL, NULL);

/*
 * Test: Invalid magic bytes should be rejected
 */
ZTEST_F(esphome_ota_tests, test_read_magic_invalid)
{
	int ret;
	uint8_t invalid_magic[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	/* Prepare invalid magic bytes */
	prepare_recv_data(invalid_magic, sizeof(invalid_magic));

	/* Call function under test */
	ret = esphome_ota_read_magic(fixture->socket_fd);

	/* Verify failure */
	zassert_equal(ret, -EIO, "Invalid magic should return -EIO, got %d", ret);

	/* Verify error response was sent */
	zassert_equal(test_socket_state.send_len, 1,
		      "Error response should be sent for invalid magic");
	zassert_equal(test_socket_state.send_buffer[0], OTA_RESPONSE_ERROR_MAGIC,
		      "Should send OTA_RESPONSE_ERROR_MAGIC (0x%02x), got 0x%02x",
		      OTA_RESPONSE_ERROR_MAGIC, test_socket_state.send_buffer[0]);
}

/*
 * Test: Partial magic bytes (short read) should fail
 */
ZTEST_F(esphome_ota_tests, test_read_magic_short)
{
	int ret;
	uint8_t partial_magic[] = {0x6C, 0x26, 0xF7}; /* Only 3 bytes */

	/* Prepare partial magic bytes */
	prepare_recv_data(partial_magic, sizeof(partial_magic));

	/* Call function under test */
	ret = esphome_ota_read_magic(fixture->socket_fd);

	/* Verify failure */
	zassert_equal(ret, -EIO, "Partial magic should return -EIO, got %d", ret);
}

/*
 * Test: Socket recv error should be propagated
 */
ZTEST_F(esphome_ota_tests, test_read_magic_recv_error)
{
	int ret;

	/* Simulate recv error */
	test_socket_state.recv_error = -ECONNRESET;

	/* Call function under test */
	ret = esphome_ota_read_magic(fixture->socket_fd);

	/* Verify error propagation */
	zassert_equal(ret, -EIO, "Socket error should result in -EIO, got %d", ret);
}

/*
 * =============================================================================
 * Tests for esphome_ota_send_version()
 * =============================================================================
 */

/*
 * Test: Version should be sent correctly
 */
ZTEST_F(esphome_ota_tests, test_send_version_valid)
{
	int ret;

	/* Call function under test */
	ret = esphome_ota_send_version(fixture->socket_fd);

	/* Verify success */
	zassert_equal(ret, 0, "Send version should return 0, got %d", ret);

	/* Verify sent data: OTA_RESPONSE_OK (0x00) + USE_OTA_VERSION (2) */
	zassert_equal(test_socket_state.send_len, 2, "Should send 2 bytes, sent %zu",
		      test_socket_state.send_len);
	zassert_equal(test_socket_state.send_buffer[0], OTA_RESPONSE_OK,
		      "First byte should be OTA_RESPONSE_OK (0x%02x), got 0x%02x", OTA_RESPONSE_OK,
		      test_socket_state.send_buffer[0]);
	zassert_equal(test_socket_state.send_buffer[1], 2,
		      "Second byte should be version 2, got %d", test_socket_state.send_buffer[1]);
}

/*
 * Test: Send failure should be handled
 */
ZTEST_F(esphome_ota_tests, test_send_version_send_error)
{
	int ret;

	/* Simulate send error */
	test_socket_state.send_error = -ECONNRESET;

	/* Call function under test */
	ret = esphome_ota_send_version(fixture->socket_fd);

	/* Verify error */
	zassert_equal(ret, -EIO, "Send error should return -EIO, got %d", ret);
}

/*
 * =============================================================================
 * Tests for esphome_ota_read_features()
 * =============================================================================
 */

/*
 * Test: Features should be read and acknowledged
 */
ZTEST_F(esphome_ota_tests, test_read_features_valid)
{
	int ret;
	uint8_t features = 0xFF; /* Initialize to invalid value */
	uint8_t test_features = 0x42;

	/* Prepare features byte */
	prepare_recv_data(&test_features, sizeof(test_features));

	/* Call function under test */
	ret = esphome_ota_read_features(fixture->socket_fd, &features);

	/* Verify success */
	zassert_equal(ret, 0, "Read features should return 0, got %d", ret);

	/* Verify features were read correctly */
	zassert_equal(features, test_features, "Features should be 0x%02x, got 0x%02x",
		      test_features, features);

	/* Verify acknowledgment was sent */
	zassert_equal(test_socket_state.send_len, 1, "Should send 1 byte ack, sent %zu",
		      test_socket_state.send_len);
	zassert_equal(test_socket_state.send_buffer[0], OTA_RESPONSE_HEADER_OK,
		      "Should send OTA_RESPONSE_HEADER_OK (0x%02x), got 0x%02x",
		      OTA_RESPONSE_HEADER_OK, test_socket_state.send_buffer[0]);
}

/*
 * Test: Zero features value
 */
ZTEST_F(esphome_ota_tests, test_read_features_zero)
{
	int ret;
	uint8_t features = 0xFF;
	uint8_t test_features = 0x00;

	prepare_recv_data(&test_features, sizeof(test_features));
	ret = esphome_ota_read_features(fixture->socket_fd, &features);

	zassert_equal(ret, 0, "Should handle zero features, got %d", ret);
	zassert_equal(features, 0x00, "Features should be 0x00, got 0x%02x", features);
}

/*
 * Test: Maximum features value
 */
ZTEST_F(esphome_ota_tests, test_read_features_max)
{
	int ret;
	uint8_t features = 0x00;
	uint8_t test_features = 0xFF;

	prepare_recv_data(&test_features, sizeof(test_features));
	ret = esphome_ota_read_features(fixture->socket_fd, &features);

	zassert_equal(ret, 0, "Should handle max features, got %d", ret);
	zassert_equal(features, 0xFF, "Features should be 0xFF, got 0x%02x", features);
}

/*
 * Test: Recv error should be propagated
 */
ZTEST_F(esphome_ota_tests, test_read_features_recv_error)
{
	int ret;
	uint8_t features = 0;

	/* Simulate recv error */
	test_socket_state.recv_error = -ECONNRESET;

	ret = esphome_ota_read_features(fixture->socket_fd, &features);

	zassert_equal(ret, -EIO, "Recv error should return -EIO, got %d", ret);
}

/*
 * Test: Send error after successful recv
 */
ZTEST_F(esphome_ota_tests, test_read_features_send_error)
{
	int ret;
	uint8_t features = 0;
	uint8_t test_features = 0x42;

	prepare_recv_data(&test_features, sizeof(test_features));

	/* Simulate send error (will occur when trying to send ack) */
	test_socket_state.send_error = -EPIPE;

	ret = esphome_ota_read_features(fixture->socket_fd, &features);

	/* Features should be read even though send failed */
	zassert_equal(features, test_features, "Features should be read before send error");
	zassert_equal(ret, -EIO, "Send error should return -EIO, got %d", ret);
}

/*
 * =============================================================================
 * Tests for esphome_ota_send_auth_ok()
 * =============================================================================
 */

/*
 * Test: Auth OK should be sent correctly
 */
ZTEST_F(esphome_ota_tests, test_send_auth_ok_valid)
{
	int ret;

	/* Call function under test */
	ret = esphome_ota_send_auth_ok(fixture->socket_fd);

	/* Verify success */
	zassert_equal(ret, 0, "Send auth OK should return 0, got %d", ret);

	/* Verify sent data */
	zassert_equal(test_socket_state.send_len, 1, "Should send 1 byte, sent %zu",
		      test_socket_state.send_len);
	zassert_equal(test_socket_state.send_buffer[0], OTA_RESPONSE_AUTH_OK,
		      "Should send OTA_RESPONSE_AUTH_OK (0x%02x), got 0x%02x", OTA_RESPONSE_AUTH_OK,
		      test_socket_state.send_buffer[0]);
}

/*
 * Test: Send error should be handled
 */
ZTEST_F(esphome_ota_tests, test_send_auth_ok_send_error)
{
	int ret;

	/* Simulate send error */
	test_socket_state.send_error = -ECONNRESET;

	ret = esphome_ota_send_auth_ok(fixture->socket_fd);

	zassert_equal(ret, -EIO, "Send error should return -EIO, got %d", ret);
}

/*
 * =============================================================================
 * Tests for esphome_ota_read_size()
 * =============================================================================
 */

/*
 * Test: Size should be parsed correctly (big-endian)
 */
ZTEST_F(esphome_ota_tests, test_read_size_valid)
{
	int ret;
	size_t size = 0;
	/* Big-endian representation of 0x12345678 */
	uint8_t size_data[] = {0x12, 0x34, 0x56, 0x78};

	prepare_recv_data(size_data, sizeof(size_data));

	ret = esphome_ota_read_size(fixture->socket_fd, &size);

	zassert_equal(ret, 0, "Read size should return 0, got %d", ret);
	zassert_equal(size, 0x12345678, "Size should be 0x12345678, got 0x%08zx", size);
}

/*
 * Test: Zero size
 */
ZTEST_F(esphome_ota_tests, test_read_size_zero)
{
	int ret;
	size_t size = 0xFFFFFFFF; /* Initialize to non-zero */
	uint8_t size_data[] = {0x00, 0x00, 0x00, 0x00};

	prepare_recv_data(size_data, sizeof(size_data));

	ret = esphome_ota_read_size(fixture->socket_fd, &size);

	zassert_equal(ret, 0, "Should handle zero size, got %d", ret);
	zassert_equal(size, 0, "Size should be 0, got 0x%08zx", size);
}

/*
 * Test: Small size
 */
ZTEST_F(esphome_ota_tests, test_read_size_small)
{
	int ret;
	size_t size = 0;
	/* Big-endian representation of 0x00001234 (4660 bytes) */
	uint8_t size_data[] = {0x00, 0x00, 0x12, 0x34};

	prepare_recv_data(size_data, sizeof(size_data));

	ret = esphome_ota_read_size(fixture->socket_fd, &size);

	zassert_equal(ret, 0, "Read size should return 0, got %d", ret);
	zassert_equal(size, 0x1234, "Size should be 0x1234, got 0x%08zx", size);
}

/*
 * Test: Maximum size
 */
ZTEST_F(esphome_ota_tests, test_read_size_max)
{
	int ret;
	size_t size = 0;
	uint8_t size_data[] = {0xFF, 0xFF, 0xFF, 0xFF};

	prepare_recv_data(size_data, sizeof(size_data));

	ret = esphome_ota_read_size(fixture->socket_fd, &size);

	zassert_equal(ret, 0, "Should handle max size, got %d", ret);
	zassert_equal(size, 0xFFFFFFFF, "Size should be 0xFFFFFFFF, got 0x%08zx", size);
}

/*
 * Test: Recv error should be propagated
 */
ZTEST_F(esphome_ota_tests, test_read_size_recv_error)
{
	int ret;
	size_t size = 0;

	/* Simulate recv error */
	test_socket_state.recv_error = -ECONNRESET;

	ret = esphome_ota_read_size(fixture->socket_fd, &size);

	zassert_equal(ret, -EIO, "Recv error should return -EIO, got %d", ret);
}

/*
 * Test: Short read (partial size data)
 */
ZTEST_F(esphome_ota_tests, test_read_size_short)
{
	int ret;
	size_t size = 0;
	/* Only 2 bytes instead of 4 */
	uint8_t size_data[] = {0x12, 0x34};

	prepare_recv_data(size_data, sizeof(size_data));

	ret = esphome_ota_read_size(fixture->socket_fd, &size);

	zassert_equal(ret, -EIO, "Short read should return -EIO, got %d", ret);
}

/*
 * =============================================================================
 * Tests for esphome_ota_send_prepare_ok()
 * =============================================================================
 */

/*
 * Test: Prepare OK should be sent correctly
 */
ZTEST_F(esphome_ota_tests, test_send_prepare_ok_valid)
{
	int ret;

	ret = esphome_ota_send_prepare_ok(fixture->socket_fd);

	zassert_equal(ret, 0, "Send prepare OK should return 0, got %d", ret);
	zassert_equal(test_socket_state.send_len, 1, "Should send 1 byte, sent %zu",
		      test_socket_state.send_len);
	zassert_equal(test_socket_state.send_buffer[0], OTA_RESPONSE_UPDATE_PREPARE_OK,
		      "Should send OTA_RESPONSE_UPDATE_PREPARE_OK (0x%02x), got 0x%02x",
		      OTA_RESPONSE_UPDATE_PREPARE_OK, test_socket_state.send_buffer[0]);
}

/*
 * Test: Send error should be handled
 */
ZTEST_F(esphome_ota_tests, test_send_prepare_ok_send_error)
{
	int ret;

	test_socket_state.send_error = -ECONNRESET;

	ret = esphome_ota_send_prepare_ok(fixture->socket_fd);

	zassert_equal(ret, -EIO, "Send error should return -EIO, got %d", ret);
}

/*
 * =============================================================================
 * Tests for esphome_ota_read_md5()
 * =============================================================================
 */

/*
 * Test: MD5 string should be read and acknowledged
 */
ZTEST_F(esphome_ota_tests, test_read_md5_valid)
{
	int ret;
	char md5_buf[33];                                          /* 32 chars + null terminator */
	const char *test_md5 = "d41d8cd98f00b204e9800998ecf8427e"; /* 32 chars */

	prepare_recv_data((const uint8_t *)test_md5, 32);

	ret = esphome_ota_read_md5(fixture->socket_fd, md5_buf, sizeof(md5_buf));

	zassert_equal(ret, 0, "Read MD5 should return 0, got %d", ret);
	zassert_mem_equal(md5_buf, test_md5, 32, "MD5 data mismatch");
	zassert_equal(md5_buf[32], '\0', "MD5 should be null-terminated");

	/* Verify acknowledgment was sent */
	zassert_equal(test_socket_state.send_len, 1, "Should send 1 byte ack, sent %zu",
		      test_socket_state.send_len);
	zassert_equal(test_socket_state.send_buffer[0], OTA_RESPONSE_BIN_MD5_OK,
		      "Should send OTA_RESPONSE_BIN_MD5_OK (0x%02x), got 0x%02x",
		      OTA_RESPONSE_BIN_MD5_OK, test_socket_state.send_buffer[0]);
}

/*
 * Test: Recv error should be handled
 */
ZTEST_F(esphome_ota_tests, test_read_md5_recv_error)
{
	int ret;
	char md5_buf[33];

	test_socket_state.recv_error = -ECONNRESET;

	ret = esphome_ota_read_md5(fixture->socket_fd, md5_buf, sizeof(md5_buf));

	zassert_equal(ret, -EIO, "Recv error should return -EIO, got %d", ret);
	/* Error response (0x00) should be attempted */
	zassert_equal(test_socket_state.send_len, 1, "Error response should be sent");
	zassert_equal(test_socket_state.send_buffer[0], 0x00, "Should send error code 0x00");
}

/*
 * Test: Short read (partial MD5)
 */
ZTEST_F(esphome_ota_tests, test_read_md5_short)
{
	int ret;
	char md5_buf[33];
	const char *partial_md5 = "d41d8cd98f00b204"; /* Only 16 chars */

	prepare_recv_data((const uint8_t *)partial_md5, 16);

	ret = esphome_ota_read_md5(fixture->socket_fd, md5_buf, sizeof(md5_buf));

	zassert_equal(ret, -EIO, "Short read should return -EIO, got %d", ret);
}

/*
 * Test: Send error after successful recv
 */
ZTEST_F(esphome_ota_tests, test_read_md5_send_error)
{
	int ret;
	char md5_buf[33];
	const char *test_md5 = "d41d8cd98f00b204e9800998ecf8427e";

	prepare_recv_data((const uint8_t *)test_md5, 32);
	test_socket_state.send_error = -EPIPE;

	ret = esphome_ota_read_md5(fixture->socket_fd, md5_buf, sizeof(md5_buf));

	/* MD5 should be read and null-terminated even though send failed */
	zassert_mem_equal(md5_buf, test_md5, 32, "MD5 should be read before send error");
	zassert_equal(md5_buf[32], '\0', "MD5 should be null-terminated");
	zassert_equal(ret, -EIO, "Send error should return -EIO, got %d", ret);
}

/*
 * =============================================================================
 * Tests for esphome_ota_read_data()
 * =============================================================================
 */

/*
 * Test: Data should be read correctly
 */
ZTEST_F(esphome_ota_tests, test_read_data_valid)
{
	int ret;
	char buf[64];
	uint8_t test_data[64];

	/* Prepare test data */
	for (int i = 0; i < sizeof(test_data); i++) {
		test_data[i] = i & 0xFF;
	}
	prepare_recv_data(test_data, sizeof(test_data));

	ret = esphome_ota_read_data(fixture->socket_fd, buf, sizeof(buf));

	zassert_equal(ret, sizeof(test_data), "Should return bytes read (%zu), got %d",
		      sizeof(test_data), ret);
	zassert_mem_equal(buf, test_data, sizeof(test_data), "Data mismatch");
	/* No acknowledgment should be sent by read_data itself */
	zassert_equal(test_socket_state.send_len, 0, "read_data should not send acknowledgment");
}

/*
 * Test: Small data chunk
 */
ZTEST_F(esphome_ota_tests, test_read_data_small)
{
	int ret;
	char buf[1024];
	uint8_t test_data[] = {0xAA, 0xBB, 0xCC, 0xDD};

	prepare_recv_data(test_data, sizeof(test_data));

	ret = esphome_ota_read_data(fixture->socket_fd, buf, sizeof(buf));

	zassert_equal(ret, sizeof(test_data), "Should return %zu bytes, got %d", sizeof(test_data),
		      ret);
	zassert_mem_equal(buf, test_data, sizeof(test_data), "Data mismatch");
}

/*
 * Test: Large data chunk (8KB)
 */
ZTEST_F(esphome_ota_tests, test_read_data_large)
{
	int ret;
	char buf[8192];
	uint8_t test_data[8192];

	/* Fill with pattern */
	for (int i = 0; i < sizeof(test_data); i++) {
		test_data[i] = (i * 17) & 0xFF;
	}

	/* Note: Our test buffer is only 1024 bytes, so we'll test partial read */
	prepare_recv_data(test_data, 1024);

	ret = esphome_ota_read_data(fixture->socket_fd, buf, sizeof(buf));

	zassert_equal(ret, 1024, "Should return 1024 bytes, got %d", ret);
	zassert_mem_equal(buf, test_data, 1024, "Data mismatch");
}

/*
 * Test: Recv error should be propagated
 */
ZTEST_F(esphome_ota_tests, test_read_data_recv_error)
{
	int ret;
	char buf[64];

	test_socket_state.recv_error = -ECONNRESET;

	ret = esphome_ota_read_data(fixture->socket_fd, buf, sizeof(buf));

	zassert_equal(ret, -EIO, "Recv error should return -EIO, got %d", ret);
	/* Error response should be sent */
	zassert_equal(test_socket_state.send_len, 1, "Error response should be sent");
}

/*
 * Test: Zero bytes available (connection closed gracefully)
 */
ZTEST_F(esphome_ota_tests, test_read_data_zero_bytes)
{
	int ret;
	char buf[64];

	/* Prepare empty buffer (recv returns 0) */
	prepare_recv_data(NULL, 0);

	ret = esphome_ota_read_data(fixture->socket_fd, buf, sizeof(buf));

	zassert_equal(ret, 0, "Zero bytes should return 0, got %d", ret);
}

/*
 * =============================================================================
 * Tests for esphome_ota_read_ack()
 * =============================================================================
 */

/*
 * Test: Valid acknowledgment should be accepted
 */
ZTEST_F(esphome_ota_tests, test_read_ack_valid)
{
	int ret;
	uint8_t ack = OTA_RESPONSE_OK;

	prepare_recv_data(&ack, sizeof(ack));

	ret = esphome_ota_read_ack(fixture->socket_fd);

	zassert_equal(ret, 0, "Valid ack should return 0, got %d", ret);
	zassert_equal(test_socket_state.send_len, 0, "read_ack should not send any data");
}

/*
 * Test: Invalid acknowledgment should be rejected
 */
ZTEST_F(esphome_ota_tests, test_read_ack_invalid)
{
	int ret;
	uint8_t ack = 0xFF; /* Invalid ack code */

	prepare_recv_data(&ack, sizeof(ack));

	ret = esphome_ota_read_ack(fixture->socket_fd);

	zassert_equal(ret, -EINVAL, "Invalid ack should return -EINVAL, got %d", ret);
}

/*
 * Test: Error response should be rejected
 */
ZTEST_F(esphome_ota_tests, test_read_ack_error_response)
{
	int ret;
	uint8_t ack = OTA_RESPONSE_ERROR_MAGIC; /* Error code */

	prepare_recv_data(&ack, sizeof(ack));

	ret = esphome_ota_read_ack(fixture->socket_fd);

	zassert_equal(ret, -EINVAL, "Error response should return -EINVAL, got %d", ret);
}

/*
 * Test: Recv error should be propagated
 */
ZTEST_F(esphome_ota_tests, test_read_ack_recv_error)
{
	int ret;

	test_socket_state.recv_error = -ECONNRESET;

	ret = esphome_ota_read_ack(fixture->socket_fd);

	zassert_equal(ret, -EIO, "Recv error should return -EIO, got %d", ret);
}

/*
 * Test: No data available (short read)
 */
ZTEST_F(esphome_ota_tests, test_read_ack_no_data)
{
	int ret;

	/* Empty buffer - recv returns 0 */
	prepare_recv_data(NULL, 0);

	ret = esphome_ota_read_ack(fixture->socket_fd);

	zassert_equal(ret, -EIO, "No data should return -EIO, got %d", ret);
}

#endif
