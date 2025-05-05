#ifndef ZEPHYR_SENSOR_SIMULATOR_H_
#define ZEPHYR_SENSOR_SIMULATOR_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Sensor simulator channel definitions */
enum sensor_sim_channel {
	SENSOR_SIM_CHAN_TEMP = 0, /* Temperature in degrees Celsius */
	SENSOR_SIM_CHAN_HUMIDITY, /* Humidity in percentage */
	/* Add more channels as needed */
	SENSOR_SIM_CHAN_COUNT /* Must be last item */
};

/* API for setting simulated sensor values */
int sensor_sim_set_value(const struct device *dev, enum sensor_sim_channel chan, double value);

/* API for setting simulated sensor noise parameters */
int sensor_sim_set_noise(const struct device *dev, enum sensor_sim_channel chan, double amplitude,
			 double frequency);

/* API for triggering simulated data ready event */
int sensor_sim_generate_event(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SENSOR_SIMULATOR_H_ */
