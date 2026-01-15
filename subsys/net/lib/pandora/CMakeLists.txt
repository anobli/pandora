# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(include)
zephyr_library_sources(log.c)
zephyr_library_sources_ifdef(CONFIG_PANDORA_SERVICE events.c service.c)

zephyr_library_sources_ifdef(CONFIG_WIFI wifi.c)
zephyr_library_sources_ifdef(CONFIG_BOARD_NATIVE_SIM native_sim.c)

zephyr_library_sources_ifdef(CONFIG_SETTINGS settings.c)
zephyr_linker_sources_ifdef(CONFIG_SETTINGS DATA_SECTIONS settings.ld)

zephyr_library_sources_ifdef(CONFIG_PANDORA_FACTORY_RESET factory_reset.c)

zephyr_linker_sources(DATA_SECTIONS iterables.ld)
add_subdirectory(components/)

zephyr_library_sources_ifdef(CONFIG_PANDORA_SHELL shell.c)
