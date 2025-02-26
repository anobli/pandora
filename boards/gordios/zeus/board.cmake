# Copyright (c) 2025 Alexandre Bailon
#
# SPDX-License-Identifier: Apache-2.0

find_program(CC1352_FLASHER NAMES cc1352_flasher)
board_set_flasher_ifnset(misc-flasher)
board_finalize_runner_args(misc-flasher ${CC1352_FLASHER} --bcf)

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
