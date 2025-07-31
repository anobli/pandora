# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(include)

zephyr_library_sources(device.c)
zephyr_library_sources_ifdef(CONFIG_NARADA_CLIENT settings.c shell.c)
zephyr_library_sources_ifdef(CONFIG_COAP narada_libcoap.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_COAP narada_ot.c)
zephyr_library_sources_ifdef(CONFIG_NARADA_STATE narada_state.c)

add_subdirectory_ifdef(CONFIG_NARADA_LIGHT light)
add_subdirectory_ifdef(CONFIG_NARADA_SWITCH switch)
add_subdirectory_ifdef(CONFIG_NARADA_PAIRING pairing)
add_subdirectory_ifdef(CONFIG_NARADA_DISCOVERY discovery)

if(NOT CONFIG_OPENTHREAD_COAP)
zephyr_linker_sources(DATA_SECTIONS iterables.ld)
zephyr_iterable_section(NAME coap_resource_narada_service
  GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT}
  SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
else()
zephyr_linker_sources(DATA_SECTIONS ot_iterables.ld)
endif()
