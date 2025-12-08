#ifndef ESPHOME_API_COMPONENT_H
#define ESPHOME_API_COMPONENT_H

#ifdef CONFIG_ESPHOME_COMPONENT_API
#include <rpc/esphome_rpc.h>
#include <rpc/api.pb-c.h>

#define MAC_ADDRESS_LEN 24
char *get_mac_address_string(char *buffer, int size);
char *get_unique_device_name(const char *name, char *buffer, int size);

#endif

#endif /* ESPHOME_API_COMPONENT_H */
