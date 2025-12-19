# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(esphome_component_ota_test)

# Test sources
target_sources(app PRIVATE
    src/main.c
)

# Include paths
target_include_directories(app PRIVATE
    ${ZEPHYR_PANDORA_MODULE_DIR}/subsys/net/lib/esphome/include
    ${ZEPHYR_PANDORA_MODULE_DIR}/subsys/net/lib/esphome/components/ota
)
