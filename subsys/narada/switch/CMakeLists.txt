# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_NARADA_SWITCH switch-common.c)
zephyr_library_sources_ifdef(CONFIG_NARADA_SWITCH_CLIENT switch-client.c)
zephyr_library_sources_ifdef(CONFIG_NARADA_SWITCH_SERVER switch-server.c)
zephyr_library_sources_ifdef(CONFIG_NARADA_SWITCH_GPIO switch-gpio.c)
