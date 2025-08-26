zephyr_syscall_header(${PANDORA_BASE}/include/zephyr/drivers/light.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LIGHT_SHELL shell.c)
zephyr_library_sources_ifdef(CONFIG_LIGHT_GPIO light_gpio.c)
zephyr_library_sources_ifdef(CONFIG_LIGHT_RGBCT light_rgbct.c)
