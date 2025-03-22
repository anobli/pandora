# SPDX-License-Identifier: Apache-2.0

zephyr_library(esphome)
zephyr_include_directories(. include)

add_subdirectory(components)

zephyr_linker_sources(DATA_SECTIONS iterables.ld)