# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(include)

zephyr_library_sources(device.c discovery.c narada_libcoap.c)
zephyr_library_sources_ifdef(CONFIG_NARADA_SERVICE service.c)

add_subdirectory_ifdef(CONFIG_NARADA_LIGHT light)

zephyr_linker_sources(DATA_SECTIONS iterables.ld)
zephyr_iterable_section(NAME coap_resource_narada_service
  GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT}
  SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
