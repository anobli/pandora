# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_BOARD_ZEUS_SKY13317 board_antenna.c)