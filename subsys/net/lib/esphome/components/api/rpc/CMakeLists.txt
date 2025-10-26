# SPDX-License-Identifier: Apache-2.0

include(esphome.cmake OPTIONAL)

zephyr_library(esphome_rpc)
zephyr_library_include_directories(.)

FILE(GLOB esphome_src *.c)
zephyr_library_sources(${esphome_src})

add_compile_definitions_ifdef(CONFIG_ESPHOME_RPC_DUMP HAS_PROTO_MESSAGE_DUMP)
zephyr_library_compile_options(-Wno-deprecated-declarations)