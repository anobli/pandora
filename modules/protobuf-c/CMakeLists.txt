# Copyright (c) 2024, Alexandre Bailon
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PROTOBUF_C)

  set(PROTOBUF_C_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_library()
  zephyr_library_sources(
    ${PROTOBUF_C_DIR}/protobuf-c/protobuf-c.c
  )

  zephyr_include_directories(${PROTOBUF_C_DIR})

endif()
