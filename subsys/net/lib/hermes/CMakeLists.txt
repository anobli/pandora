# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(include)

zephyr_library_sources(device.c discovery.c hermes_libcoap.c)
zephyr_library_sources_ifdef(CONFIG_HERMES_SERVICE service.c)
zephyr_library_sources_ifdef(CONFIG_HERMES_SHELL shell.c)
zephyr_library_sources_ifdef(CONFIG_WIFI wifi.c)
zephyr_library_sources_ifdef(CONFIG_BOARD_NATIVE_SIM native_sim.c)

add_subdirectory_ifdef(CONFIG_HERMES_LIGHT light)

zephyr_linker_sources(DATA_SECTIONS iterables.ld)
zephyr_iterable_section(NAME coap_resource_hermes_service
  GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT}
  SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})


zephyr_library_sources_ifdef(CONFIG_SETTINGS settings.c)
zephyr_linker_sources_ifdef(CONFIG_SETTINGS DATA_SECTIONS settings.ld)