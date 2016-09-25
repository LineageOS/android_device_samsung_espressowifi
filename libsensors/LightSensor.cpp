/*
 * Copyright (C) 2013 Paul Kocialkowski <contact@paulk.fr>
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "LightSensor"

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cstring>

#include <cutils/log.h>

#include "LightSensor.h"

LightSensor::LightSensor(const int sensorType)
    : SensorBase(NULL, "light_sensor"),
    mSensorType(sensorType),
    mEnabled(0),
    mInputReader(4),
    mHasPendingEvent(false)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_L;
    mPendingEvent.type = SENSOR_TYPE_LIGHT;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

    if (data_fd) {
        strcpy(input_sysfs_path, "/sys/class/input/");
        strcat(input_sysfs_path, input_name);
        strcat(input_sysfs_path, "/device/");
        input_sysfs_path_len = strlen(input_sysfs_path);
        enable(0, 1);
    }
}

LightSensor::~LightSensor() {
    if (mEnabled) {
        enable(0, 0);
    }
}

int LightSensor::setDelay(int32_t handle __unused, int64_t ns)
{
    int fd;

    strcpy(&input_sysfs_path[input_sysfs_path_len], "poll_delay");
    fd = open(input_sysfs_path, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%lld", ns);
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;
    }
    return -1;
}

int LightSensor::enable(int32_t handle __unused, int en)
{
    int flags = en ? 1 : 0;
    int fd;
    if (flags != mEnabled) {
        strcpy(&input_sysfs_path[input_sysfs_path_len], "enable");
        fd = open(input_sysfs_path, O_RDWR);
        if (fd >= 0) {
            write(fd, en == 1 ? "1" : "0", 2);
            close(fd);
            mEnabled = flags;
            return 0;
        }
        return -1;
    }
    return 0;
}

bool LightSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int LightSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
        return mEnabled ? 1 : 0;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_REL) {
            if (event->code == EVENT_TYPE_LIGHT)
                mPendingEvent.light = valueToLux(event->value);
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        } else {
            ALOGE("unknown event (type=%d, code=%d, value=%d)",
                    type, event->code, event->value);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

float LightSensor::valueToLux(int value) const
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

    switch (mSensorType) {
        case SENSOR_TYPE_AL3201:
            return powf(10, value * (330.0f / 4095.0f / 47.0f)) * 4;
        case SENSOR_TYPE_BH1721:
            return value * 0.712f;
        case SENSOR_TYPE_GP2A:
            return powf(10, value * (125.0f / 1023.0f / 24.0f)) * 4;
        default:
            ALOGE("Unknown sensor type");
    }

    return 0;
}
