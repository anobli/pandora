#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <narada/light/server.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(narada, CONFIG_NARADA_LOG_LEVEL);

#define DT_DRV_COMPAT narada_light_rgbct

#define MAX_WHITE_BRIGHTNESS 100

#define RED_COLOR(color) ((color >> 0) & 0xFF)
#define GREEN_COLOR(color) ((color >> 8) & 0xFF)
#define BLUE_COLOR(color) ((color >> 16) & 0xFF)
#define RGB_WHITE_COLOR 0x00FFFFFF
#define RGB_MAX_PERIOD 255

struct rgbct_light_data {
	static const struct pwm_dt_spec *white_brightness;
	static const struct pwm_dt_spec *color_temperature;
	static const struct pwm_dt_spec *red;
	static const struct pwm_dt_spec *green;
	static const struct pwm_dt_spec *blue;

	/* TODO: get it from settings */
	int state;
	int brightness;
	int temperature;
	int color;
};

static int rgbct_light_init(struct narada_light_resource *sw)
{
	struct rgbct_light_data *data = sw->rsc.data;
	int err;

	if (!pwm_is_ready_dt(data->white_brightness) ||
		!pwm_is_ready_dt(data->color_temperature)) {
		return -ENODEV;
	}

	if (!pwm_is_ready_dt(data->red) || !pwm_is_ready_dt(data->green) ||
		!pwm_is_ready_dt(data->blue)) {
		return -ENODEV;
	}

	/* TODO: load from settings and restore light setttings */

	return 0;
}

static int rgbct_light_set(struct narada_light_resource *sw, uint8_t white_brightness, uint8_t color_temperature, int color)
{
	struct rgbct_light_data *data = sw->rsc.data;

	if (color == RGB_WHITE_COLOR) {
		pwm_set_dt(data->red, 0, 0);
		pwm_set_dt(data->green, 0, 0);
		pwm_set_dt(data->blue, 0, 0);

		if (white_brightness >= MAX_WHITE_BRIGHTNESS) {
			return -EINVAL;
		}

		pwm_set_dt(data->white_brightness, MAX_WHITE_BRIGHTNESS, white_brightness);
		/* Not sure about that one */
		pwm_set_dt(data->color_temperature, MAX_WHITE_BRIGHTNESS, color_temperature);

		return 0;
	} else {
		pwm_set_dt(data->white_brightness, 0, 0);
		pwm_set_dt(data->color_temperature, 0, 0);

		pwm_set_dt(data->red, MAX_RGB, red);
		pwm_set_dt(data->green, MAX_RGB, green);
		pwm_set_dt(data->blue, MAX_RGB, blue);

		return 0;
	}
}

static int rgbct_light_update(struct narada_light_resource *sw)
{
	struct rgbct_light_data *data = sw->rsc.data;

	data->state = state > 0;
	if (data->state) {
		return rgbct_light_set(sw, data->brightness, 100, data->color);
	} else {
		return rgbct_light_set(sw, 0, 0, 0);
	}
}


static int rgbct_light_set_state(struct narada_light_resource *sw, int state)
{
	struct rgbct_light_data *data = sw->rsc.data;

	data->state = state > 0;
	return rgbct_light_update(sw);
}

static int rgbct_light_get_state(struct narada_light_resource *sw)
{
	struct rgbct_light_data *data = sw->rsc.data;

	return data->state;
}

static int rgbct_light_set_brightness(struct narada_light_resource *sw, uint8_t brightness)
{
	struct rgbct_light_data *data = sw->rsc.data;

	data->brightness = brightness;
	return rgbct_light_update(sw);
}

static struct narada_light_api rgbct_light = {
	.init = rgbct_light_init,
	.set_state = rgbct_light_set_state,
	.get_state = rgbct_light_get_state,
	.set_brightness = rgbct_light_set_brightness,
};

#define NARADA_RGBCT_LIGHT(inst)                                                                    \
	static struct rgbct_light_data rgbct_light_data_##inst = {                                   \
		.white_brightness = PWM_DT_SPEC_INST_GET_BY_NAME(inst, white_brightness_pwms),                                        \
		.color_temperature = PWM_DT_SPEC_INST_GET_BY_NAME(inst, color_temperature_pwms),                                        \
		.red = PWM_DT_SPEC_INST_GET_BY_NAME(inst, red_pwms),                                        \
		.green = PWM_DT_SPEC_INST_GET_BY_NAME(inst, green_pwms),                                        \
		.blue = PWM_DT_SPEC_INST_GET_BY_NAME(inst, blue_pwms),                                        \
	};                                                                                         \
	DEFINE_NARADA_LIGHT(inst, &gpio_light, &rgbct_light_data_##inst);

DT_INST_FOREACH_STATUS_OKAY(NARADA_RGBCT_LIGHT)
