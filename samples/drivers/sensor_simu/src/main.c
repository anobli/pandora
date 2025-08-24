/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_simu.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Define a device tree alias for the sensor simulator */
#define SENSOR_SIM_NODE DT_ALIAS(sensor_sim0)

#if !DT_NODE_EXISTS(SENSOR_SIM_NODE)
#error "No sensor simulator defined in the device tree!"
#endif

static const struct device *sensor_dev;

#ifdef CONFIG_SENSOR_SIM_TRIGGER
static void data_ready_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	struct sensor_value temp, humidity;

	/* Get the current temperature value */
	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) == 0) {
		LOG_INF("Temperature: %.1f C", sensor_value_to_double(&temp));
	}

	/* Get the current humidity value */
	if (sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity) == 0) {
		LOG_INF("Humidity: %.1f %%", sensor_value_to_double(&humidity));
	}
}
#endif

int main(void)
{
	struct sensor_value temp, humidity;
	int err;

	LOG_INF("Sensor Simulator Example");

	/* Get the sensor simulator device */
	sensor_dev = DEVICE_DT_GET(SENSOR_SIM_NODE);
	if (!device_is_ready(sensor_dev)) {
		LOG_ERR("Sensor simulator not ready");
		return -ENODEV;
	}

	LOG_INF("Sensor simulator device initialized");

	/* Set initial values for temperature and humidity */
	err = sensor_sim_set_value(sensor_dev, SENSOR_SIM_CHAN_TEMP, 22.5);
	if (err) {
		LOG_ERR("Failed to set temperature: %d", err);
	}

	err = sensor_sim_set_value(sensor_dev, SENSOR_SIM_CHAN_HUMIDITY, 45.8);
	if (err) {
		LOG_ERR("Failed to set humidity: %d", err);
	}

	/* Add some noise to the temperature readings */
	err = sensor_sim_set_noise(sensor_dev, SENSOR_SIM_CHAN_TEMP, 0.5, 0.1);
	if (err) {
		LOG_ERR("Failed to set temperature noise: %d", err);
	}

#ifdef CONFIG_SENSOR_SIM_TRIGGER
	/* Set up trigger for data ready */
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	err = sensor_trigger_set(sensor_dev, &trig, data_ready_handler);
	if (err) {
		LOG_ERR("Failed to set trigger: %d", err);
	}
#else
	/* Polling mode - read sensor values periodically */
	while (1) {
		/* Get the current temperature value */
		if (sensor_sample_fetch(sensor_dev) == 0) {
			if (sensor_channel_get(sensor_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) == 0) {
				LOG_INF("Temperature: %.1f C", sensor_value_to_double(&temp));
			}

			if (sensor_channel_get(sensor_dev, SENSOR_CHAN_HUMIDITY, &humidity) == 0) {
				LOG_INF("Humidity: %.1f %%", sensor_value_to_double(&humidity));
			}
		}

		/* Wait for 1 second before next reading */
		k_sleep(K_SECONDS(1));
	}
#endif

	/* Modify values programmatically during runtime */
	k_sleep(K_SECONDS(5));
	LOG_INF("Updating temperature to 30.0 C");
	sensor_sim_set_value(sensor_dev, SENSOR_SIM_CHAN_TEMP, 30.0);

	k_sleep(K_SECONDS(5));
	LOG_INF("Updating humidity to 75.0 %");
	sensor_sim_set_value(sensor_dev, SENSOR_SIM_CHAN_HUMIDITY, 75.0);

	return 0;
}
