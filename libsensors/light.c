/*
 * Copyright (C) 2013 Paul Kocialkowski <contact@paulk.fr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <linux/ioctl.h>
#include <linux/input.h>

#include <hardware/sensors.h>
#include <hardware/hardware.h>

#define LOG_TAG "piranha_sensors"
#include <utils/Log.h>

#include "piranha_sensors.h"

enum {
	SENSOR_TYPE_BH1721 = 1,
	SENSOR_TYPE_AL3201,
	SENSOR_TYPE_GP2A,
};

struct light_data {
	char path_enable[PATH_MAX];
	char path_delay[PATH_MAX];
	int sensor_type;
};

int light_init(struct piranha_sensors_handlers *handlers,
	struct piranha_sensors_device *device)
{
	struct light_data *data = NULL;
	char path[PATH_MAX] = { 0 };
	int input_fd = -1;
	int rc;

	ALOGD("%s(%p, %p)", __func__, handlers, device);

	if (handlers == NULL)
		return -EINVAL;

	data = (struct light_data *) calloc(1, sizeof(struct light_data));

	input_fd = input_open("light_sensor");
	if (input_fd < 0) {
		ALOGE("%s: Unable to open input", __func__);
		goto error;
	}

	rc = sysfs_path_prefix("light_sensor", (char *) &path);
	if (rc < 0 || path[0] == '\0') {
		ALOGE("%s: Unable to open sysfs", __func__);
		goto error;
	}

	snprintf(data->path_enable, PATH_MAX, "%s/enable", path);
	snprintf(data->path_delay, PATH_MAX, "%s/poll_delay", path);

	handlers->poll_fd = input_fd;
	handlers->data = (void *) data;

	char device_variant[16];
	FILE *f = fopen(DEVICE_VARIANT_SYSFS, "r");
	if (!f || fgets(device_variant, 16, f) == NULL) {
		ALOGE("Failed to read " DEVICE_VARIANT_SYSFS ", assuming P51xx\n");
		strcpy(device_variant, "espresso10");
	}
	if (f)
		fclose(f);

	ALOGD("Device: %s", device_variant);

	if (strcmp(device_variant, "espresso10") == 0) {
		/* Device is P51xx */
		data->sensor_type = SENSOR_TYPE_BH1721;
	} else if (strcmp(device_variant, "espressowifi") == 0) {
		/* Device is P3110 */
		data->sensor_type = SENSOR_TYPE_AL3201;
	} else {
		/* Device is P3100 */
		data->sensor_type = SENSOR_TYPE_GP2A;
	}

	return 0;

error:
	if (data != NULL)
		free(data);

	if (input_fd >= 0)
		close(input_fd);

	handlers->poll_fd = -1;
	handlers->data = NULL;

	return -1;
}

int light_deinit(struct piranha_sensors_handlers *handlers)
{
	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL)
		return -EINVAL;

	if (handlers->poll_fd >= 0)
		close(handlers->poll_fd);
	handlers->poll_fd = -1;

	if (handlers->data != NULL)
		free(handlers->data);
	handlers->data = NULL;

	return 0;
}

int light_activate(struct piranha_sensors_handlers *handlers)
{
	struct light_data *data;
	int rc;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct light_data *) handlers->data;

	rc = sysfs_value_write(data->path_enable, 1);
	if (rc < 0) {
		ALOGE("%s: Unable to write sysfs value", __func__);
		return -1;
	}

	handlers->activated = 1;

	return 0;
}

int light_deactivate(struct piranha_sensors_handlers *handlers)
{
	struct light_data *data;
	int rc;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct light_data *) handlers->data;

	rc = sysfs_value_write(data->path_enable, 0);
	if (rc < 0) {
		ALOGE("%s: Unable to write sysfs value", __func__);
		return -1;
	}

	handlers->activated = 1;

	return 0;
}

int light_set_delay(struct piranha_sensors_handlers *handlers, int64_t delay)
{
	struct light_data *data;
	int rc;

	ALOGD("%s(%p, %" PRId64 ")", __func__, handlers, delay);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct light_data *) handlers->data;

	rc = sysfs_value_write(data->path_delay, delay);
	if (rc < 0) {
		ALOGE("%s: Unable to write sysfs value", __func__);
		return -1;
	}

	return 0;
}

float light_convert(int value, int sensor_type)
{
	// Converting the AL3201 raw value to lux:
	// I = 10 * log(light) uA
	// U = raw * 3300 / 4095 (max ADC value is 3.3V for 4095 LSB)
	// R = 47kOhm
	// => light = 10 ^ (I / 10) = 10 ^ (U / R / 10)
	// => light = 10 ^ (raw * 330 / 4095 / 47)
	// Only 1/4 of light reaches the sensor:
	// => light = 4 * (10 ^ (raw * 330 / 4095 / 47))

	// Converting the GP2A raw value to lux:
	// I = 10 * log(Ev) uA
	// R = 24kOhm
	// Max adc value 1023 = 1.25V
	// 1/4 of light reaches sensor

	switch (sensor_type) {
		case SENSOR_TYPE_AL3201:
			return powf(10, value * (330.0f / 4095.0f / 47.0f)) * 4;
		case SENSOR_TYPE_BH1721:
			return value * 0.712f;
		case SENSOR_TYPE_GP2A:
			return powf(10, value * (125.0f / 1023.0f / 24.0f)) * 4;
		default:
			ALOGE("%s: Unknown sensor type", __func__);
	}

	return 0;
}

int light_get_data(struct piranha_sensors_handlers *handlers,
	struct sensors_event_t *event)
{
	struct light_data *data = (struct light_data *) handlers->data;
	struct input_event input_event;
	int input_fd;
	int rc;

//	ALOGD("%s(%p, %p)", __func__, handlers, event);

	if (handlers == NULL || event == NULL)
		return -EINVAL;

	input_fd = handlers->poll_fd;
	if (input_fd < 0)
		return -EINVAL;

	memset(event, 0, sizeof(struct sensors_event_t));
	event->version = sizeof(struct sensors_event_t);
	event->sensor = handlers->handle;
	event->type = handlers->handle;

	do {
		rc = read(input_fd, &input_event, sizeof(input_event));
		if (rc < (int) sizeof(input_event))
			break;

		if (input_event.type == EV_REL) {
			if (input_event.code == REL_MISC)
				event->light = light_convert(input_event.value, data->sensor_type);
		} else if (input_event.type == EV_SYN) {
			if (input_event.code == SYN_REPORT)
				event->timestamp = input_timestamp(&input_event);
		}
	} while (input_event.type != EV_SYN);

	return 0;
}

struct piranha_sensors_handlers light = {
	.name = "Light",
	.handle = SENSOR_TYPE_LIGHT,
	.init = light_init,
	.deinit = light_deinit,
	.activate = light_activate,
	.deactivate = light_deactivate,
	.set_delay = light_set_delay,
	.get_data = light_get_data,
	.activated = 0,
	.needed = 0,
	.poll_fd = -1,
	.data = NULL,
};
