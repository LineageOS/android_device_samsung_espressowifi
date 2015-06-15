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
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <linux/input.h>

#include <hardware/sensors.h>
#include <hardware/hardware.h>

#define LOG_TAG "geomagneticd"
#include <utils/Log.h>

#include "geomagneticd.h"

// This geomagnetic daemon is in charge of finding the correct calibration
// offsets to apply to the YAS530 magnetic field sensor.
// This is done by finding raw data extrema (minimum and maximum) for each axis
// and calculating the offset so that these values are -45uT and 45uT.

/*
 * Config
 */

int geomagneticd_config_read(struct geomagneticd_data *data)
{
	char buffer[100] = { 0 };
	int config_fd = -1;
	int rc;

	if (data == NULL)
		return -EINVAL;

	config_fd = open(GEOMAGNETICD_CONFIG_PATH, O_RDONLY);
	if (config_fd < 0) {
		ALOGE("%s: Unable to open config", __func__);
		goto error;
	}

	rc = read(config_fd, buffer, sizeof(buffer));
	if (rc <= 0) {
		ALOGE("%s: Unable to read config", __func__);
		goto error;
	}

	rc = sscanf(buffer, "%d,%d,%d,%d,%d,%d,%d",
		&data->hard_offsets[0], &data->hard_offsets[1], &data->hard_offsets[2],
		&data->calib_offsets[0], &data->calib_offsets[1], &data->calib_offsets[2],
		&data->accuracy);
	if (rc != 7) {
		ALOGE("%s: Unable to parse config", __func__);
		goto error;
	}

	rc = 0;
	goto complete;

error:
	rc = -1;

complete:
	if (config_fd >= 0)
		close(config_fd);

	return rc;
}

int geomagneticd_config_write(struct geomagneticd_data *data)
{
	char buffer[100] = { 0 };
	int config_fd = -1;
	int rc;

	if (data == NULL)
		return -EINVAL;

	config_fd = open(GEOMAGNETICD_CONFIG_BACKUP_PATH, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (config_fd < 0) {
		ALOGE("%s: Unable to open config", __func__);
		goto error;
	}

	sprintf(buffer, "%d,%d,%d,%d,%d,%d,%d",
		data->hard_offsets[0], data->hard_offsets[1], data->hard_offsets[2],
		data->calib_offsets[0], data->calib_offsets[1], data->calib_offsets[2],
		data->accuracy);

	rc = write(config_fd, buffer, strlen(buffer) + 1);
	if (rc < (int) strlen(buffer) + 1) {
		ALOGE("%s: Unable to write config", __func__);
		goto error;
	}

	rename(GEOMAGNETICD_CONFIG_BACKUP_PATH, GEOMAGNETICD_CONFIG_PATH);

	rc = 0;
	goto complete;

error:
	rc = -1;

complete:
	if (config_fd >= 0)
		close(config_fd);

	return rc;
}

/*
 * Offsets
 */

int geomagneticd_offsets_read(struct geomagneticd_data *data)
{
	char buffer[100] = { 0 };
	int offsets_fd = -1;
	int rc;

	if (data == NULL)
		return -EINVAL;

	offsets_fd = open(data->path_offsets, O_RDONLY);
	if (offsets_fd < 0) {
		ALOGE("%s: Unable to open offsets", __func__);
		goto error;
	}

	rc = read(offsets_fd, buffer, sizeof(buffer));
	if (rc <= 0) {
		ALOGE("%s: Unable to read offsets", __func__);
		goto error;
	}

	rc = sscanf(buffer, "%d %d %d %d %d %d %d",
		&data->hard_offsets[0], &data->hard_offsets[1], &data->hard_offsets[2],
		&data->calib_offsets[0], &data->calib_offsets[1], &data->calib_offsets[2],
		&data->accuracy);
	if (rc != 7) {
		ALOGE("%s: Unable to parse offsets", __func__);
		goto error;
	}

	rc = 0;
	goto complete;

error:
	rc = -1;

complete:
	if (offsets_fd >= 0)
		close(offsets_fd);

	return rc;
}

int geomagneticd_offsets_write(struct geomagneticd_data *data)
{
	char buffer[100] = { 0 };
	int offsets_fd = -1;
	int rc;

	if (data == NULL)
		return -EINVAL;

	offsets_fd = open(data->path_offsets, O_WRONLY);
	if (offsets_fd < 0) {
		ALOGE("%s: Unable to open offsets", __func__);
		goto error;
	}

	sprintf(buffer, "%d %d %d %d %d %d %d\n",
		data->hard_offsets[0], data->hard_offsets[1], data->hard_offsets[2],
		data->calib_offsets[0], data->calib_offsets[1], data->calib_offsets[2],
		data->accuracy);

	rc = write(offsets_fd, buffer, strlen(buffer) + 1);
	if (rc < (int) strlen(buffer) + 1) {
		ALOGE("%s: Unable to write offsets", __func__);
		goto error;
	}

	rc = 0;
	goto complete;

error:
	rc = -1;

complete:
	if (offsets_fd >= 0)
		close(offsets_fd);

	return rc;
}

int geomagneticd_offsets_init(struct geomagneticd_data *data)
{
	int count;
	int i;

	if (data == NULL)
		return -EINVAL;

	count = sizeof(data->hard_offsets) / sizeof(int);

	// 0x7f is an invalid value for hard offsets
	for (i = 0; i < count; i++)
		data->hard_offsets[i] = 0x7f;

	count = sizeof(data->calib_offsets) / sizeof(int);

	// 0x0x7fffffff is an invalid value for calib offsets
	for (i = 0; i < count; i++)
		data->calib_offsets[i] = 0x7fffffff;

	return 0;
}

int geomagneticd_offsets_check(struct geomagneticd_data *data)
{
	int count;
	int i;

	if (data == NULL)
		return -EINVAL;

	count = sizeof(data->hard_offsets) / sizeof(int);

	// 0x7f is an invalid value for hard offsets
	for (i = 0; i < count; i++)
		if (data->hard_offsets[i] == 0x7f)
			return 0;

	count = sizeof(data->calib_offsets) / sizeof(int);

	// 0x0x7fffffff is an invalid value for calib offsets
	for (i = 0; i < count; i++)
		if (data->calib_offsets[i] == 0x7fffffff)
			return 0;

	return 1;
}

/*
 * Geomagneticd
 */

int geomagneticd_magnetic_extrema_init(struct geomagneticd_data *data)
{
	int count;
	int i;

	if (data == NULL)
		return -EINVAL;

	count = sizeof(data->calib_offsets) / sizeof(int);

	// Approximate the previous extrema from the calib offsets
	for (i = 0; i < count; i++) {
		data->magnetic_extrema[0][i] = -(data->calib_offsets[i] + 45) * 1000 + 5000;
		data->magnetic_extrema[1][i] = (data->calib_offsets[i] + 45) * 1000 - 5000;
	}

	return 0;
}

int geomagneticd_magnetic_extrema(struct geomagneticd_data *data, int index,
	int value)
{
	if (data == NULL || index < 0 || index >= 3)
		return -EINVAL;

	if (value == 0)
		return 0;

	// Update the extrema from the current value if needed
	if (value < data->magnetic_extrema[0][index] || data->magnetic_extrema[0][index] == 0)
		data->magnetic_extrema[0][index] = value;
	if (value > data->magnetic_extrema[1][index] || data->magnetic_extrema[1][index] == 0)
		data->magnetic_extrema[1][index] = value;

	return 0;
}

int geomagneticd_calib_offsets(struct geomagneticd_data *data)
{
	int calib_offsets[3];
	int offsets[2];
	int update;
	int count;
	int rc;
	int i;

	if (data == NULL)
		return -EINVAL;

	// Calculating the offset is only meaningful when enough values were
	// obtained. There is no need to calculate it too often either.
	if (data->count % 10 != 0)
		return 0;

	update = 0;

	count = sizeof(data->calib_offsets) / sizeof(int);

	// Calculate the calib offset for each axis to have values in [-45;45] uT
	for (i = 0; i < count; i++) {
		offsets[0] = data->magnetic_extrema[0][i] + 45 * 1000;
		offsets[1] = data->magnetic_extrema[1][i] - 45 * 1000;
		calib_offsets[i] = (offsets[0] + offsets[1]) / 2;

		if (calib_offsets[i] != data->calib_offsets[i]) {
			data->calib_offsets[i] = calib_offsets[i];
			update = 1;
		}
	}

	if (update) {
		data->accuracy = 1;

		rc = geomagneticd_offsets_write(data);
		if (rc < 0) {
			ALOGE("%s: Unable to write offsets", __func__);
			return -1;
		}

		rc = geomagneticd_config_write(data);
		if (rc < 0) {
			ALOGE("%s: Unable to write config", __func__);
			return -1;
		}
	}

	return 0;
}

int geomagneticd_poll(struct geomagneticd_data *data)
{
	struct input_event input_event;
	struct pollfd poll_fd;
	int rc;

	if (data == NULL)
		return -EINVAL;

	if (data->input_fd < 0)
		return -1;

	memset(&poll_fd, 0, sizeof(poll_fd));
	poll_fd.fd = data->input_fd;
	poll_fd.events = POLLIN;

	while (1) {
		rc = poll(&poll_fd, 1, -1);
		if (rc < 0) {
			ALOGE("%s: poll failure", __func__);
			goto error;
		}

		if (!(poll_fd.revents & POLLIN))
			continue;

		poll_fd.revents = 0;

		rc = read(data->input_fd, &input_event, sizeof(input_event));
		if (rc < (int) sizeof(input_event)) {
			ALOGE("%s: Unable to read input event", __func__);
			continue;
		}

		// Update the extrema from the current value
		if(input_event.type == EV_ABS) {
			switch (input_event.code) {
				case ABS_X:
					geomagneticd_magnetic_extrema(data, 0, input_event.value);
					break;
				case ABS_Y:
					geomagneticd_magnetic_extrema(data, 1, input_event.value);
					break;
				case ABS_Z:
					geomagneticd_magnetic_extrema(data, 2, input_event.value);
					break;
			}
		}

		if (input_event.type == EV_SYN) {
			// Sometimes, the hard offsets cannot be read at startup
			// so we need to do it now
			if (!geomagneticd_offsets_check(data)) {
				rc = geomagneticd_offsets_read(data);
				if (rc < 0) {
					ALOGE("%s: Unable to read offsets", __func__);
					continue;
				}

				// Most likely, the calib offset will be invalid
				if (geomagneticd_offsets_check(data)) {
					data->accuracy = 1;
					geomagneticd_magnetic_extrema_init(data);
				}

				rc = geomagneticd_config_write(data);
				if (rc < 0) {
					ALOGE("%s: Unable to write config", __func__);
					continue;
				}
			}

			data->count++;

			rc = geomagneticd_calib_offsets(data);
			if (rc < 0) {
				ALOGE("%s: Unable to calib offsets", __func__);
				continue;
			}
		}
	}

	rc = 0;
	goto complete;

error:
	rc = -1;

complete:
	return rc;
}

int main(int argc, char *argv[])
{
	struct geomagneticd_data *geomagneticd_data = NULL;
	char path[PATH_MAX] = { 0 };
	int input_fd = -1;
	int rc;

	geomagneticd_data = (struct geomagneticd_data *)
		calloc(1, sizeof(struct geomagneticd_data));

	input_fd = input_open("geomagnetic_raw");
	if (input_fd < 0) {
		ALOGE("%s: Unable to open input", __func__);
		goto error;
	}

	rc = sysfs_path_prefix("geomagnetic_raw", (char *) &path);
	if (rc < 0 || path[0] == '\0') {
		ALOGE("%s: Unable to open sysfs", __func__);
		goto error;
	}

	snprintf(geomagneticd_data->path_offsets, PATH_MAX, "%s/offsets", path);

	geomagneticd_data->input_fd = input_fd;

	geomagneticd_offsets_init(geomagneticd_data);

	// Attempt to read the offsets from the config
	rc = geomagneticd_config_read(geomagneticd_data);
	if (rc < 0 || !geomagneticd_offsets_check(geomagneticd_data)) {
		// Read the offsets from the driver
		rc = geomagneticd_offsets_read(geomagneticd_data);
		if (rc < 0) {
			ALOGE("%s: Unable to read offsets", __func__);
			goto error;
		}

		// Most likely, the calib offset will be invalid and the hard
		// offset may be invalid as well
		if (geomagneticd_offsets_check(geomagneticd_data)) {
			geomagneticd_data->accuracy = 1;
			geomagneticd_magnetic_extrema_init(geomagneticd_data);
		}
	} else {
		// Get the magnetic extrema from the config's offsets
		geomagneticd_magnetic_extrema_init(geomagneticd_data);

		rc = geomagneticd_offsets_write(geomagneticd_data);
		if (rc < 0) {
			ALOGE("%s: Unable to write offsets", __func__);
			goto error;
		}
	}

	rc = geomagneticd_poll(geomagneticd_data);
	if (rc < 0)
		goto error;

	rc = 0;
	goto complete;

error:
	while (1)
		sleep(3600);

	rc = 1;

complete:
	if (input_fd >= 0)
		close(input_fd);

	if (geomagneticd_data != NULL)
		free(geomagneticd_data);

	return rc;
}
