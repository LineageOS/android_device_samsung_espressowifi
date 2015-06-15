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

#ifndef _ORIENTATIOND_H_
#define _ORIENTATIOND_H_

struct orientationd_data;

struct orientationd_handlers {
	char *input_name;
	int handle;
	int poll_fd;

	int (*get_data)(struct orientationd_handlers *handlers,
		struct orientationd_data *data);
};

struct orientationd_data {
	struct orientationd_handlers **handlers;
	int handlers_count;

	struct pollfd *poll_fds;
	int poll_fds_count;

	sensors_vec_t orientation;
	sensors_vec_t acceleration;
	sensors_vec_t magnetic;

	int64_t delay;
	int input_fd;

	int activated;

	pthread_t thread;
	pthread_mutex_t mutex;
	int thread_continue;
};

extern struct orientationd_handlers *orientationd_handlers[];
extern int orientationd_handlers_count;

/*
 * Input
 */

void input_event_set(struct input_event *event, int type, int code, int value);
int64_t timestamp(struct timeval *time);
int64_t input_timestamp(struct input_event *event);
int uinput_rel_create(const char *name);
void uinput_destroy(int uinput_fd);
int input_open(char *name);
int sysfs_path_prefix(char *name, char *path_prefix);
int64_t sysfs_value_read(char *path);
int sysfs_value_write(char *path, int64_t value);
int sysfs_string_read(char *path, char *buffer, size_t length);
int sysfs_string_write(char *path, char *buffer, size_t length);

/*
 * Sensors
 */

extern struct orientationd_handlers bma250;
extern struct orientationd_handlers yas530;

#endif
