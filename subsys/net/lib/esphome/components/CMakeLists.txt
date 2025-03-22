zephyr_library_sources(esphome.c)

macro(add_compile_definitions_ifdef feature_toggle)
	if(${${feature_toggle}})
		add_compile_definitions(${ARGN})
	endif()
endmacro()

zephyr_library_sources(action.c)
zephyr_library_sources_ifdef(CONFIG_ESPHOME_COMPONENT_LOGGER logger.c)

add_subdirectory_ifdef(CONFIG_ESPHOME_COMPONENT_API api)
add_subdirectory_ifdef(CONFIG_ESPHOME_COMPONENT_OTA ota)
add_subdirectory_ifdef(CONFIG_ESPHOME_COMPONENT_BUTTON button)
add_subdirectory_ifdef(CONFIG_ESPHOME_COMPONENT_OPENTHREAD openthread)
add_subdirectory_ifdef(CONFIG_ESPHOME_COMPONENT_SENSOR sensor)
add_subdirectory_ifdef(CONFIG_ESPHOME_COMPONENT_SWITCH switch)
add_subdirectory_ifdef(CONFIG_ESPHOME_COMPONENT_WIFI wifi)
