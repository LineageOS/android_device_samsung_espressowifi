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
#include <math.h>
#include <linux/input.h>

#include <hardware/sensors.h>
#include <hardware/hardware.h>

#define LOG_TAG "orientationd"
#include <utils/Log.h>

#include "orientationd.h"

struct orientationd_handlers *orientationd_handlers[] = {
	&bma250,
	&yas530,
};

int orientationd_handlers_count = sizeof(orientationd_handlers) /
	sizeof(struct orientationd_handlers *);

static float rad2deg(float v)
{
	return (v * 180.0f / 3.1415926535f);
}

static float vector_scalar(sensors_vec_t *v, sensors_vec_t *d)
{
	return v->x * d->x + v->y * d->y + v->z * d->z;
}

static float vector_length(sensors_vec_t *v)
{
	return sqrtf(vector_scalar(v, v));
}

int orientation_calculate(struct orientationd_data *data)
{
	sensors_vec_t *a, *m, *o;
	float azimuth, pitch, roll;
	float la, sinp, cosp, sinr, cosr, x, y;

	if (data == NULL)
		return -EINVAL;

	a = &data->acceleration;
	m = &data->magnetic;
	o = &data->orientation;

	la = vector_length(a);
	pitch = asinf(-(a->y) / la);
	roll = asinf((a->x) / la);

	sinp = sinf(pitch);
	cosp = cosf(pitch);
	sinr = sinf(roll);
	cosr = cosf(roll);

	y = -(m->x) * cosr + m->z * sinr;
	x = m->x * sinp * sinr + m->y * cosp + m->z * sinp * cosr;
	azimuth = atan2f(y, x);

	o->azimuth = rad2deg(azimuth);
	o->pitch = rad2deg(pitch);
	o->roll = rad2deg(roll);

	if (o->azimuth < 0)
		o->azimuth += 360.0f;

	return 0;
}

void *orientationd_thread(void *thread_data)
{
	struct orientationd_data *data;
	struct input_event event;
	struct timeval time;
	int64_t before, after;
	useconds_t diff;
	int input_fd;
	int rc;

	if (thread_data == NULL)
		return NULL;

	data = (struct orientationd_data *) thread_data;

	input_fd = data->input_fd;
	if (input_fd < 0)
		return NULL;

	while (data->thread_continue) {
		pthread_mutex_lock(&data->mutex);
		if (!data->thread_continue)
			break;

		while (data->activated) {
			gettimeofday(&time, NULL);
			before = timestamp(&time);

			rc = orientation_calculate(data);
			if (rc < 0) {
				ALOGE("%s: Unable to calculate orientation", __func__);
				goto next;
			}

			input_event_set(&event, EV_ABS, ABS_X, (int) (data->orientation.azimuth * 1000));
			write(input_fd, &event, sizeof(event));
			input_event_set(&event, EV_ABS, ABS_Y, (int) (data->orientation.pitch * 1000));
			write(input_fd, &event, sizeof(event));
			input_event_set(&event, EV_ABS, ABS_Z, (int) (data->orientation.roll * 1000));
			write(input_fd, &event, sizeof(event));
			input_event_set(&event, EV_SYN, 0, 0);
			write(input_fd, &event, sizeof(event));

next:
			gettimeofday(&time, NULL);
			after = timestamp(&time);

			diff = (useconds_t) (data->delay * 1000000 - (after - before)) / 1000;
			if (diff <= 0)
				continue;

			usleep(diff);
		}
	}
	return NULL;
}

int orientation_get_data(struct orientationd_data *data)
{
	struct input_event input_event;
	int input_fd;
	int activated;
	unsigned int delay;
	int rc;

	if (data == NULL)
		return -EINVAL;

	input_fd = data->input_fd;
	if (input_fd < 0)
		return -1;

	do {
		rc = read(input_fd, &input_event, sizeof(input_event));
		if (rc < (int) sizeof(input_event))
			break;

		if (input_event.type == EV_ABS) {
			switch (input_event.code) {
				case ABS_THROTTLE:
					data->activated = input_event.value & (1 << 16) ? 1 : 0;
					data->delay = input_event.value & ~(1 << 16);

					pthread_mutex_unlock(&data->mutex);
					break;
				default:
					continue;
			}
		}
	} while (input_event.type != EV_SYN);

	return 0;
}

int orientationd_poll(struct orientationd_data *data)
{
	int count;
	int i, j;
	int rc;

	if (data == NULL)
		return -EINVAL;

	ALOGD("Starting orientationd poll");

	while (1) {
		if (data->activated)
			count = data->poll_fds_count;
		else
			count = 1;

		rc = poll(data->poll_fds, count, -1);
		if (rc < 0) {
			ALOGE("%s: poll failure", __func__);
			goto error;
		}

		for (i = 0; i < count; i++) {
			if (data->poll_fds[i].revents & POLLIN) {
				data->poll_fds[i].revents = 0;

				if (data->poll_fds[i].fd == data->input_fd) {
					orientation_get_data(data);
					continue;
				}

				for (j = 0; j < data->handlers_count; j++)
					if (data->handlers[j] != NULL && data->handlers[j]->poll_fd == data->poll_fds[i].fd && data->handlers[j]->get_data != NULL)
						data->handlers[j]->get_data(data->handlers[j], data);
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
	struct orientationd_data *orientationd_data = NULL;
	pthread_attr_t thread_attr;
	int input_fd = -1;
	int poll_fd;
	int p, i;
	int rc;

	orientationd_data = (struct orientationd_data *)
		calloc(1, sizeof(struct orientationd_data));
	orientationd_data->handlers = orientationd_handlers;
	orientationd_data->handlers_count = orientationd_handlers_count;
	orientationd_data->activated = 0;
	orientationd_data->poll_fds = (struct pollfd *)
		calloc(1, (orientationd_handlers_count + 1) * sizeof(struct pollfd));

	p = 0;

	input_fd = input_open("orientation");
	if (input_fd < 0) {
		ALOGE("%s: Unable to open input", __func__);
		goto error;
	}

	orientationd_data->input_fd = input_fd;

	orientationd_data->poll_fds[p].fd = input_fd;
	orientationd_data->poll_fds[p].events = POLLIN;
	p++;

	for (i = 0; i < orientationd_handlers_count; i++) {
		if (orientationd_handlers[i] == NULL || orientationd_handlers[i]->input_name == NULL)
			continue;

		poll_fd = input_open(orientationd_handlers[i]->input_name);
		if (poll_fd < 0) {
			ALOGE("%s: Unable to open input %s", __func__, orientationd_handlers[i]->input_name);
			continue;
		}

		orientationd_handlers[i]->poll_fd = poll_fd;
		orientationd_data->poll_fds[p].fd = poll_fd;
		orientationd_data->poll_fds[p].events = POLLIN;
		p++;
	}

	orientationd_data->poll_fds_count = p;

	orientationd_data->thread_continue = 1;

	pthread_mutex_init(&orientationd_data->mutex, NULL);
	pthread_mutex_lock(&orientationd_data->mutex);

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

	rc = pthread_create(&orientationd_data->thread, &thread_attr, orientationd_thread, (void *) orientationd_data);
	if (rc < 0) {
		ALOGE("%s: Unable to create orientationd thread", __func__);
		goto error;
	}

	rc = orientationd_poll(orientationd_data);
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

	if (orientationd_data != NULL) {
		orientationd_data->thread_continue = 0;
		pthread_mutex_destroy(&orientationd_data->mutex);

		if (orientationd_data->poll_fds != NULL)
			free(orientationd_data->poll_fds);

		free(orientationd_data);
	}

	return rc;
}
