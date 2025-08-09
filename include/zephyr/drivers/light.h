#ifndef PANDORA_LIGHT_H
#define PANDORA_LIGHT_H

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pandora_light_data {
	uint8_t state;
	uint8_t brightness;
	uint8_t temperature;
	uint32_t color;
};

__subsystem struct pandora_light_driver_api {
	int (*update)(const struct device *dev);
	struct pandora_light_data *(*get_light_data)(const struct device *dev);
};

__syscall int pandora_light_update(const struct device *dev);

static inline int z_impl_pandora_light_update(const struct device *dev)
{
	const struct pandora_light_driver_api *api = DEVICE_API_GET(pandora_light, dev);

	__ASSERT(api && api->update, "update is required");

	return api->update(dev);
}

__syscall struct pandora_light_data *pandora_light_get_data(const struct device *dev);

static inline struct pandora_light_data *z_impl_pandora_light_get_data(const struct device *dev)

{
	const struct pandora_light_driver_api *api = DEVICE_API_GET(pandora_light, dev);

	__ASSERT(api && api->get_light_data, "get_light_data is required");

	return api->get_light_data(dev);
}

static int inline pandora_light_get_state(const struct device *dev, uint8_t *state)
{
	struct pandora_light_data *light_data = pandora_light_get_data(dev);

	*state = light_data->state;

	return 0;
}

static inline int pandora_light_set_state(const struct device *dev, uint8_t state)
{
	struct pandora_light_data *light_data = pandora_light_get_data(dev);

	light_data->state = state;
	return pandora_light_update(dev);
}

static inline int pandora_light_get_brightness(const struct device *dev, uint8_t *brightness)
{
	struct pandora_light_data *light_data = pandora_light_get_data(dev);

	*brightness = light_data->brightness;

	return 0;
}

static inline int pandora_light_set_brightness(const struct device *dev, uint8_t brightness)
{
	struct pandora_light_data *light_data = pandora_light_get_data(dev);

	light_data->brightness = brightness;
	return pandora_light_update(dev);
}

static inline int pandora_light_get_temperature(const struct device *dev, uint8_t *temperature)
{
	struct pandora_light_data *light_data = pandora_light_get_data(dev);

	*temperature = light_data->temperature;

	return 0;
}

static inline int pandora_light_set_temperature(const struct device *dev, uint8_t temperature)
{
	struct pandora_light_data *light_data = pandora_light_get_data(dev);

	light_data->temperature = temperature;
	return pandora_light_update(dev);
}

static inline int pandora_light_get_color(const struct device *dev, uint32_t *color)
{
	struct pandora_light_data *light_data = pandora_light_get_data(dev);

	*color = light_data->color;

	return 0;
}

static inline int pandora_light_set_color(const struct device *dev, uint32_t color)
{
	struct pandora_light_data *light_data = pandora_light_get_data(dev);

	light_data->color = color;
	return pandora_light_update(dev);
}

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/light.h>

#endif /* PANDORA_LIGHT_H */
