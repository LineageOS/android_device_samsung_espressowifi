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

struct yas_orientation_data {
	struct piranha_sensors_handlers *acceleration_sensor;
	struct piranha_sensors_handlers *magnetic_sensor;

	char path_enable[PATH_MAX];
	char path_delay[PATH_MAX];

	sensors_vec_t orientation;
};

int yas_orientation_init(struct piranha_sensors_handlers *handlers,
	struct piranha_sensors_device *device)
{
	struct yas_orientation_data *data = NULL;
	char path[PATH_MAX] = { 0 };
	int input_fd = -1;
	int rc;
	int i;

	ALOGD("%s(%p, %p)", __func__, handlers, device);

	if (handlers == NULL)
		return -EINVAL;

	data = (struct yas_orientation_data *) calloc(1, sizeof(struct yas_orientation_data));

	for (i = 0; i < device->handlers_count; i++) {
		if (device->handlers[i] == NULL)
			continue;

		if (device->handlers[i]->handle == SENSOR_TYPE_ACCELEROMETER)
			data->acceleration_sensor = device->handlers[i];
		else if (device->handlers[i]->handle == SENSOR_TYPE_MAGNETIC_FIELD)
			data->magnetic_sensor = device->handlers[i];
	}

	if (data->acceleration_sensor == NULL || data->magnetic_sensor == NULL) {
		ALOGE("%s: Missing sensors for orientation", __func__);
		goto error;
	}

	input_fd = input_open("orientation");
	if (input_fd < 0) {
		ALOGE("%s: Unable to open input", __func__);
		goto error;
	}

	rc = sysfs_path_prefix("orientation", (char *) &path);
	if (rc < 0 || path[0] == '\0') {
		ALOGE("%s: Unable to open sysfs", __func__);
		goto error;
	}

	snprintf(data->path_enable, PATH_MAX, "%s/enable", path);
	snprintf(data->path_delay, PATH_MAX, "%s/delay", path);

	handlers->poll_fd = input_fd;
	handlers->data = (void *) data;

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

int yas_orientation_deinit(struct piranha_sensors_handlers *handlers)
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

int yas_orientation_activate(struct piranha_sensors_handlers *handlers)
{
	struct yas_orientation_data *data;
	int rc;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct yas_orientation_data *) handlers->data;

	if (data->acceleration_sensor == NULL || data->magnetic_sensor == NULL)
		return -1;

	data->acceleration_sensor->needed |= PIRANHA_SENSORS_NEEDED_ORIENTATION;
	if (data->acceleration_sensor->needed == PIRANHA_SENSORS_NEEDED_ORIENTATION)
		data->acceleration_sensor->activate(data->acceleration_sensor);

	data->magnetic_sensor->needed |= PIRANHA_SENSORS_NEEDED_ORIENTATION;
	if (data->magnetic_sensor->needed == PIRANHA_SENSORS_NEEDED_ORIENTATION)
		data->magnetic_sensor->activate(data->magnetic_sensor);

	rc = sysfs_value_write(data->path_enable, 1);
	if (rc < 0) {
		ALOGE("%s: Unable to write sysfs value", __func__);
		return -1;
	}

	handlers->activated = 1;

	return 0;
}

int yas_orientation_deactivate(struct piranha_sensors_handlers *handlers)
{
	struct yas_orientation_data *data;
	int rc;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct yas_orientation_data *) handlers->data;

	if (data->acceleration_sensor == NULL || data->magnetic_sensor == NULL)
		return -1;

	data->acceleration_sensor->needed &= ~(PIRANHA_SENSORS_NEEDED_ORIENTATION);
	if (data->acceleration_sensor->needed == 0)
		data->acceleration_sensor->deactivate(data->acceleration_sensor);

	data->magnetic_sensor->needed &= ~(PIRANHA_SENSORS_NEEDED_ORIENTATION);
	if (data->magnetic_sensor->needed == 0)
		data->magnetic_sensor->deactivate(data->magnetic_sensor);

	rc = sysfs_value_write(data->path_enable, 0);
	if (rc < 0) {
		ALOGE("%s: Unable to write sysfs value", __func__);
		return -1;
	}

	handlers->activated = 1;

	return 0;
}

int yas_orientation_set_delay(struct piranha_sensors_handlers *handlers, int64_t delay)
{
	struct yas_orientation_data *data;
	int rc;

	ALOGD("%s(%p, %" PRId64 ")", __func__, handlers, delay);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct yas_orientation_data *) handlers->data;

	if (data->acceleration_sensor == NULL || data->magnetic_sensor == NULL)
		return -1;

	if (data->acceleration_sensor->needed == PIRANHA_SENSORS_NEEDED_ORIENTATION)
		data->acceleration_sensor->set_delay(data->acceleration_sensor, delay);

	if (data->magnetic_sensor->needed == PIRANHA_SENSORS_NEEDED_ORIENTATION)
		data->magnetic_sensor->set_delay(data->magnetic_sensor, delay);

	if (delay < 10000000)
		delay = 10;
	else
		delay /= 1000000;

	rc = sysfs_value_write(data->path_delay, delay);
	if (rc < 0) {
		ALOGE("%s: Unable to write sysfs value", __func__);
		return -1;
	}

	return 0;
}

float yas_orientation_convert(int value)
{
	return value / 1000.0f;
}

int yas_orientation_get_data(struct piranha_sensors_handlers *handlers,
	struct sensors_event_t *event)
{
	struct yas_orientation_data *data;
	struct input_event input_event;
	int input_fd;
	int rc;

//	ALOGD("%s(%p, %p)", __func__, handlers, event);

	if (handlers == NULL || handlers->data == NULL || event == NULL)
		return -EINVAL;

	data = (struct yas_orientation_data *) handlers->data;

	input_fd = handlers->poll_fd;
	if (input_fd < 0)
		return -EINVAL;

	memset(event, 0, sizeof(struct sensors_event_t));
	event->version = sizeof(struct sensors_event_t);
	event->sensor = handlers->handle;
	event->type = handlers->handle;

	event->orientation.azimuth = data->orientation.azimuth;
	event->orientation.pitch = data->orientation.pitch;
	event->orientation.roll = data->orientation.roll;

	do {
		rc = read(input_fd, &input_event, sizeof(input_event));
		if (rc < (int) sizeof(input_event))
			break;

		if (input_event.type == EV_ABS) {
			switch (input_event.code) {
				case ABS_X:
					event->orientation.azimuth = yas_orientation_convert(input_event.value);
					break;
				case ABS_Y:
					event->orientation.pitch = yas_orientation_convert(input_event.value);
					break;
				case ABS_Z:
					event->orientation.roll = yas_orientation_convert(input_event.value);
					break;
				default:
					continue;
			}
		} else if (input_event.type == EV_SYN) {
			if (input_event.code == SYN_REPORT)
				event->timestamp = input_timestamp(&input_event);
		}
	} while (input_event.type != EV_SYN);

	data->orientation.azimuth = event->orientation.azimuth;
	data->orientation.pitch = event->orientation.pitch;
	data->orientation.roll = event->orientation.roll;

	return 0;
}

struct piranha_sensors_handlers yas_orientation = {
	.name = "YAS Orientation",
	.handle = SENSOR_TYPE_ORIENTATION,
	.init = yas_orientation_init,
	.deinit = yas_orientation_deinit,
	.activate = yas_orientation_activate,
	.deactivate = yas_orientation_deactivate,
	.set_delay = yas_orientation_set_delay,
	.get_data = yas_orientation_get_data,
	.activated = 0,
	.needed = 0,
	.poll_fd = -1,
	.data = NULL,
};
