#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <narada/switch/server.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(narada, CONFIG_NARADA_LOG_LEVEL);

#define DT_DRV_COMPAT narada_switch_gpio

struct gpio_switch_data {
	struct gpio_dt_spec gpio;
	int state;
};

static int gpio_switch_init(struct narada_switch_resource *sw)
{
	struct gpio_switch_data *data = sw->rsc.data;
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

static int gpio_switch_set_state(struct narada_switch_resource *sw, int state)
{
	struct gpio_switch_data *data = sw->rsc.data;

	data->state = state > 0;
	return gpio_pin_set_dt(&data->gpio, state > 0);
}

static int gpio_switch_get_state(struct narada_switch_resource *sw)
{
	struct gpio_switch_data *data = sw->rsc.data;

	return data->state;
}

static struct narada_switch_api gpio_switch = {
	.init = gpio_switch_init,
	.set_state = gpio_switch_set_state,
	.get_state = gpio_switch_get_state,
};

#define NARADA_GPIO_SWITCH(inst)                                                                   \
	static struct gpio_switch_data gpio_switch_data_##inst = {                                 \
		.gpio = GPIO_DT_SPEC_INST_GET(inst, gpios),                                        \
	};                                                                                         \
	DEFINE_NARADA_SWITCH(inst, DT_NODE_FULL_NAME_TOKEN(DT_DRV_INST(inst)), &gpio_switch,       \
			     &gpio_switch_data_##inst);

DT_INST_FOREACH_STATUS_OKAY(NARADA_GPIO_SWITCH)
