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

#include <stdint.h>
#include <poll.h>
#include <linux/input.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <hardware/sensors.h>
#include <hardware/hardware.h>

#ifndef _GEOMAGNETICD_H_
#define _GEOMAGNETICD_H_

#define GEOMAGNETICD_CONFIG_PATH		"/data/sensors/yas.cfg"
#define GEOMAGNETICD_CONFIG_BACKUP_PATH		"/data/sensors/yas-backup.cfg"

struct geomagneticd_data {
	int magnetic_extrema[2][3];
	int hard_offsets[3];
	int calib_offsets[3];
	int accuracy;

	int input_fd;
	char path_offsets[PATH_MAX];

	int count;
};

/*
 * Input
 */

int input_open(char *name);
int sysfs_path_prefix(char *name, char *path_prefix);

#endif
