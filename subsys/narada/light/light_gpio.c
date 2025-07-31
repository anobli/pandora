#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <narada/light/server.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(narada, CONFIG_NARADA_LOG_LEVEL);

#define DT_DRV_COMPAT narada_light_gpio

struct gpio_light_data {
	struct gpio_dt_spec gpio;
	int state;
};

static int gpio_light_init(struct narada_light_resource *sw)
{
	struct gpio_light_data *data = sw->rsc.data;
	int err;

	if (device_is_ready(data->gpio.port)) {
		data->state = 0;
		err = gpio_pin_configure_dt(&data->gpio, GPIO_OUTPUT_INACTIVE);

		if (err) {
			LOG_ERR("Cannot configure GPIO (err %d)", err);
		}
	} else {
		LOG_ERR("%s: GPIO device not ready", sw->rsc.name);
		err = -ENODEV;
	}

	return 0;
}

static int gpio_light_set_state(struct narada_light_resource *sw, int state)
{
	struct gpio_light_data *data = sw->rsc.data;

	data->state = state > 0;
	return gpio_pin_set_dt(&data->gpio, state > 0);
}

static int gpio_light_get_state(struct narada_light_resource *sw)
{
	struct gpio_light_data *data = sw->rsc.data;

	return data->state;
}

static struct narada_light_api gpio_light = {
	.init = gpio_light_init,
	.set_state = gpio_light_set_state,
	.get_state = gpio_light_get_state,
};

#define NARADA_GPIO_LIGHT(inst)                                                                    \
	static struct gpio_light_data gpio_light_data_##inst = {                                   \
		.gpio = GPIO_DT_SPEC_INST_GET(inst, gpios),                                        \
	};                                                                                         \
	DEFINE_NARADA_LIGHT(inst, &gpio_light, &gpio_light_data_##inst);

DT_INST_FOREACH_STATUS_OKAY(NARADA_GPIO_LIGHT)
