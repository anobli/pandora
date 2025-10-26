# SPDX-License-Identifier: Apache-2.0

list(APPEND SYSCALL_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include/zephyr/drivers/)
set(SYSCALL_INCLUDE_DIRS ${SYSCALL_INCLUDE_DIRS} PARENT_SCOPE)

set(PANDORA_BASE ${CMAKE_CURRENT_LIST_DIR} CACHE STRING "Pandora base dir")

zephyr_include_directories(include)

add_subdirectory(drivers)
add_subdirectory_ifdef(CONFIG_HERMES subsys/hermes)
add_subdirectory_ifdef(CONFIG_ESPHOME subsys/net/lib/esphome)
