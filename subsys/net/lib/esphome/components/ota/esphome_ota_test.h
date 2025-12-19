#ifndef ESPHOME_OTA_TEST_H
#define ESPHOME_OTA_TEST_H

#ifdef CONFIG_TEST
#define STATIC

extern uint8_t MAGIC_BYTES[];

extern int (*esphome_ota_test_recv)(int socket, void *data, size_t len, int flags);
extern int (*esphome_ota_test_send)(int socket, void *data, size_t len, int flags);

int esphome_ota_read_magic(int socket);
int esphome_ota_send_version(int socket);
int esphome_ota_read_features(int socket, uint8_t *ota_features);
int esphome_ota_send_auth_ok(int socket);
int esphome_ota_read_size(int socket, size_t *ota_size);
int esphome_ota_send_prepare_ok(int socket);
int esphome_ota_read_md5(int socket, char *md5_buf, int size);
int esphome_ota_read_data(int socket, char *buf, int size);
int esphome_ota_send_data_ack(int socket);
int esphome_ota_read_ack(int socket);
int esphome_ota_run(int socket, struct flash_img_context *ctx);
#else
#define STATIC static
#endif

#endif /* ESPHOME_OTA_TEST_H */
