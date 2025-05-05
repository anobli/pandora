#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_simu.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include <stdlib.h>

#define DT_DRV_COMPAT zephyr_sensor_simulator

LOG_MODULE_REGISTER(sensor_simulator, CONFIG_SENSOR_LOG_LEVEL);

/* Define configuration struct for device tree data */
struct sensor_sim_config {
	uint32_t default_temp;
	uint32_t default_temp_frac;
	uint32_t default_humidity;
	uint32_t default_humidity_frac;
	uint32_t temp_noise_amp;
	uint32_t temp_noise_freq;
	uint32_t humidity_noise_amp;
	uint32_t humidity_noise_freq;
};

/* Structure to hold channel-specific data */
struct sim_channel_data {
	double value;           /* Current simulated value */
	double noise_amplitude; /* Amplitude of noise to add to the value */
	double noise_frequency; /* Frequency of noise variation */
	bool simulated;         /* Whether this channel has been set or not */
};

/* Main data structure for the sensor simulator */
struct sensor_sim_data {
	struct sim_channel_data channels[SENSOR_SIM_CHAN_COUNT];
	struct k_mutex mutex; /* Mutex to protect access to simulator data */
#ifdef CONFIG_SENSOR_SIM_TRIGGER
	struct sensor_trigger trigger;
	sensor_trigger_handler_t trigger_handler;
	struct k_work_delayable work;
	const struct device *dev;
#endif
};

/* Map simulator channels to sensor channels */
static const enum sensor_channel channel_map[] = {
	[SENSOR_SIM_CHAN_TEMP] = SENSOR_CHAN_AMBIENT_TEMP,
	[SENSOR_SIM_CHAN_HUMIDITY] = SENSOR_CHAN_HUMIDITY,
	/* Add more mappings as needed */
};

/**
 * @brief Set simulated value for a specific channel
 *
 * @param dev Pointer to the device structure
 * @param chan Channel to set the value for
 * @param value Value to set
 *
 * @return 0 if successful, negative error code otherwise
 */
int sensor_sim_set_value(const struct device *dev, enum sensor_sim_channel chan, double value)
{
	struct sensor_sim_data *data = dev->data;

	if (chan >= SENSOR_SIM_CHAN_COUNT) {
		return -EINVAL;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	data->channels[chan].value = value;
	data->channels[chan].simulated = true;
	k_mutex_unlock(&data->mutex);

	LOG_DBG("Channel %d value set to %f", chan, value);

	return 0;
}

/**
 * @brief Set noise parameters for a specific channel
 *
 * @param dev Pointer to the device structure
 * @param chan Channel to set the noise for
 * @param amplitude Amplitude of the noise
 * @param frequency Frequency of the noise variation
 *
 * @return 0 if successful, negative error code otherwise
 */
int sensor_sim_set_noise(const struct device *dev, enum sensor_sim_channel chan, double amplitude,
			 double frequency)
{
	struct sensor_sim_data *data = dev->data;

	if (chan >= SENSOR_SIM_CHAN_COUNT) {
		return -EINVAL;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	data->channels[chan].noise_amplitude = amplitude;
	data->channels[chan].noise_frequency = frequency;
	k_mutex_unlock(&data->mutex);

	LOG_DBG("Channel %d noise set to amplitude=%f, frequency=%f", chan, amplitude, frequency);

	return 0;
}

/* Get a random value to simulate sensor noise */
static double get_noise_value(struct sim_channel_data *channel_data)
{
	if (channel_data->noise_amplitude <= 0.0) {
		return 0.0;
	}

	/* Simple noise model: random value within amplitude range */
	double random_factor = (double)rand() / RAND_MAX; /* 0.0 to 1.0 */
	return (random_factor * 2 - 1) * channel_data->noise_amplitude;
}

/**
 * @brief Get simulated sensor value for a specific channel
 *
 * @param data Sensor simulator data
 * @param chan Channel to get the value for
 *
 * @return Simulated sensor value
 */
static double get_sim_value(struct sensor_sim_data *data, enum sensor_sim_channel chan)
{
	struct sim_channel_data *channel_data = &data->channels[chan];

	if (!channel_data->simulated) {
		/* Return default values if not explicitly set */
		switch (chan) {
		case SENSOR_SIM_CHAN_TEMP:
			return 25.0; /* Default room temperature in Celsius */
		case SENSOR_SIM_CHAN_HUMIDITY:
			return 50.0; /* Default humidity in percentage */
		default:
			return 0.0;
		}
	}

	/* Add noise to the value */
	return channel_data->value + get_noise_value(channel_data);
}

/**
 * @brief Get sensor channel value
 *
 * @param dev Pointer to the device structure
 * @param chan Sensor channel to get
 * @param val Pointer to sensor_value to store the result
 *
 * @return 0 if successful, negative error code otherwise
 */
static int sensor_sim_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	/* Nothing to do here as we generate values on-demand */
	return 0;
}

/**
 * @brief Get sensor channel value
 *
 * @param dev Pointer to the device structure
 * @param chan Sensor channel to get
 * @param val Pointer to sensor_value to store the result
 *
 * @return 0 if successful, negative error code otherwise
 */
static int sensor_sim_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct sensor_sim_data *data = dev->data;
	double value = 0.0;
	int i;

	/* Find corresponding simulator channel */
	if (chan == SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	for (i = 0; i < SENSOR_SIM_CHAN_COUNT; i++) {
		if (channel_map[i] == chan) {
			k_mutex_lock(&data->mutex, K_FOREVER);
			value = get_sim_value(data, i);
			k_mutex_unlock(&data->mutex);

			switch (chan) {
			case SENSOR_CHAN_AMBIENT_TEMP:
				sensor_value_from_double(val, value);
				return 0;
			case SENSOR_CHAN_HUMIDITY:
				sensor_value_from_double(val, value);
				return 0;
			default:
				return -ENOTSUP;
			}
		}
	}

	return -ENOTSUP;
}

#ifdef CONFIG_SENSOR_SIM_TRIGGER

/**
 * @brief Work handler for simulating sensor data ready events
 */
static void sensor_sim_work_handler(struct k_work *work)
{
	struct sensor_sim_data *data = CONTAINER_OF(work, struct sensor_sim_data, work.work);

	if (data->trigger_handler) {
		data->trigger_handler(data->dev, &data->trigger);
	}

	/* Reschedule the work */
	k_work_schedule(&data->work, K_SECONDS(1));
}

/**
 * @brief Set sensor trigger
 *
 * @param dev Pointer to the device structure
 * @param trig Trigger to set
 * @param handler Trigger handler
 *
 * @return 0 if successful, negative error code otherwise
 */
static int sensor_sim_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				  sensor_trigger_handler_t handler)
{
	struct sensor_sim_data *data = dev->data;

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	data->trigger = *trig;
	data->trigger_handler = handler;
	k_mutex_unlock(&data->mutex);

	if (handler) {
		/* Start periodic work */
		k_work_schedule(&data->work, K_NO_WAIT);
	} else {
		/* Cancel periodic work */
		k_work_cancel_delayable(&data->work);
	}

	return 0;
}

/**
 * @brief Manually generate a sensor event
 *
 * @param dev Pointer to the device structure
 *
 * @return 0 if successful, negative error code otherwise
 */
int sensor_sim_generate_event(const struct device *dev)
{
	struct sensor_sim_data *data = dev->data;

	if (data->trigger_handler) {
		data->trigger_handler(dev, &data->trigger);
		return 0;
	}

	return -EAGAIN;
}

#else /* !CONFIG_SENSOR_SIM_TRIGGER */

int sensor_sim_generate_event(const struct device *dev)
{
	return -ENOTSUP;
}

#endif /* CONFIG_SENSOR_SIM_TRIGGER */

static const struct sensor_driver_api sensor_sim_api = {
	.sample_fetch = sensor_sim_sample_fetch,
	.channel_get = sensor_sim_channel_get,
#ifdef CONFIG_SENSOR_SIM_TRIGGER
	.trigger_set = sensor_sim_trigger_set,
#endif
};

/**
 * @brief Initialize the sensor simulator with Device Tree configurations
 *
 * @param dev Pointer to the device structure
 *
 * @return 0 if successful, negative error code otherwise
 */
static int sensor_sim_init(const struct device *dev)
{
	struct sensor_sim_data *data = dev->data;
	const struct sensor_sim_config *config = dev->config;
	double temp_default, humidity_default;
	double temp_noise_amp, temp_noise_freq;
	double humidity_noise_amp, humidity_noise_freq;

	k_mutex_init(&data->mutex);

	/* Initialize with values from Device Tree */

	/* Temperature defaults */
	temp_default = (double)config->default_temp + (double)config->default_temp_frac / 100.0;

	/* Humidity defaults */
	humidity_default =
		(double)config->default_humidity + (double)config->default_humidity_frac / 100.0;

	/* Temperature noise */
	temp_noise_amp = (double)config->temp_noise_amp / 100.0;
	temp_noise_freq = (double)config->temp_noise_freq / 100.0;

	/* Humidity noise */
	humidity_noise_amp = (double)config->humidity_noise_amp / 100.0;
	humidity_noise_freq = (double)config->humidity_noise_freq / 100.0;

	/* Set default values */
	data->channels[SENSOR_SIM_CHAN_TEMP].value = temp_default;
	data->channels[SENSOR_SIM_CHAN_TEMP].simulated = true;
	data->channels[SENSOR_SIM_CHAN_TEMP].noise_amplitude = temp_noise_amp;
	data->channels[SENSOR_SIM_CHAN_TEMP].noise_frequency = temp_noise_freq;

	data->channels[SENSOR_SIM_CHAN_HUMIDITY].value = humidity_default;
	data->channels[SENSOR_SIM_CHAN_HUMIDITY].simulated = true;
	data->channels[SENSOR_SIM_CHAN_HUMIDITY].noise_amplitude = humidity_noise_amp;
	data->channels[SENSOR_SIM_CHAN_HUMIDITY].noise_frequency = humidity_noise_freq;

	LOG_INF("Sensor simulator initialized with temperature: %.2f°C (noise: %.2f), humidity: "
		"%.2f%% (noise: %.2f)",
		temp_default, temp_noise_amp, humidity_default, humidity_noise_amp);

#ifdef CONFIG_SENSOR_SIM_TRIGGER
	data->dev = dev;
	k_work_init_delayable(&data->work, sensor_sim_work_handler);
#endif

	return 0;
}

/* Define the sensor simulator device */
#define SENSOR_SIM_INIT(inst)                                                                      \
	static struct sensor_sim_data sensor_sim_data_##inst;                                      \
                                                                                                   \
	static const struct sensor_sim_config sensor_sim_config_##inst = {                         \
		.default_temp = DT_INST_PROP_OR(inst, default_temperature, 25),                    \
		.default_temp_frac = DT_INST_PROP_OR(inst, default_temperature_frac, 0),           \
		.default_humidity = DT_INST_PROP_OR(inst, default_humidity, 50),                   \
		.default_humidity_frac = DT_INST_PROP_OR(inst, default_humidity_frac, 0),          \
		.temp_noise_amp = DT_INST_PROP_OR(inst, temp_noise_amplitude, 0),                  \
		.temp_noise_freq = DT_INST_PROP_OR(inst, temp_noise_frequency, 0),                 \
		.humidity_noise_amp = DT_INST_PROP_OR(inst, humidity_noise_amplitude, 0),          \
		.humidity_noise_freq = DT_INST_PROP_OR(inst, humidity_noise_frequency, 0),         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, sensor_sim_init, NULL, &sensor_sim_data_##inst,                \
			      &sensor_sim_config_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
			      &sensor_sim_api);

/* Create instances based on DT_INST_FOREACH_STATUS_OKAY */
DT_INST_FOREACH_STATUS_OKAY(SENSOR_SIM_INIT)
