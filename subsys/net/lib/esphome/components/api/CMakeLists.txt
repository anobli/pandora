# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(. include)

zephyr_library_sources(service.c api.c entity.c)

add_subdirectory(rpc)
