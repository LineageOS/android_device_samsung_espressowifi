/*
 * Copyright (C) 2016 The Android Open-Source Project
 * Copyright (C) 2013 Paul Kocialkowski <contact@paulk.fr>
 * Copyright (C) 2016 Dániel Járai <jaraidaniel@gmail.com>
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

#define LOG_TAG "Sensors"

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <cstring>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"

#include "LightSensor.h"
#include "ProximitySensor.h"
#include "OrientationSensor.h"
#include "MagneticSensor.h"
#include "AccelerationSensor.h"

#define LOCAL_SENSORS (5)

static struct sensor_t sSensorList[LOCAL_SENSORS] = {
	{
		.name = "BMA254 Acceleration Sensor",
		.vendor = "Bosch Sensortec",
		.version = 1,
		.handle = ID_A,
		.type = SENSOR_TYPE_ACCELEROMETER,
		.maxRange = 2 * GRAVITY_EARTH,
		.resolution = GRAVITY_EARTH / 256.0f,
		.power = 0.13f,
		.minDelay = 0,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		{},
	},
	{
		.name = "MS-3E (YAS530) Magnetic Sensor",
		.vendor = "Yamaha Corporation",
		.version = 1,
		.handle = ID_M,
		.type = SENSOR_TYPE_MAGNETIC_FIELD,
		.maxRange = 800.0f,
		.resolution = 0.3f,
		.power = 4.0f,
		.minDelay = 10000,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		{},
	},
	{
		.name = "MS-x Orientation Sensor",
		.vendor = "Yamaha Corporation",
		.version = 1,
		.handle = ID_O,
		.type = SENSOR_TYPE_ORIENTATION,
		.maxRange = 360.0f,
		.resolution = 0.1f,
		.power = 0.0f,
		.minDelay = 10000,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		{},
	},
	{
		.name = "BH1721 Light Sensor",	// might change
		.vendor = "ROHM",		// might change
		.version = 1,
		.handle = ID_L,
		.type = SENSOR_TYPE_LIGHT,
		.maxRange = 0.0f,
		.resolution = 0.0f,
		.power = 0.0f,
		.minDelay = 0,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		{},
	},
	{	/* P3100 only */
		.name = "GP2AP002 Proximity Sensor",
		.vendor = "Sharp",
		.version = 1,
		.handle = ID_PX,
		.type = SENSOR_TYPE_PROXIMITY,
		.maxRange = 5.0f,
		.resolution = 0.0f,
		.power = 0.0f,
		.minDelay = 0,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		{},
	},
};
static int numSensors = (sizeof(sSensorList) / sizeof(sensor_t));

static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);

static int sensors__get_sensors_list(struct sensors_module_t* module __unused,
                                     struct sensor_t const** list)
{
    *list = sSensorList;
    return numSensors;
}

static struct hw_module_methods_t sensors_module_methods = {
    .open = open_sensors,
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = SENSORS_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "Galaxy Tab 2 Sensor Module",
        .author = "Daniel Jarai",
        .methods = &sensors_module_methods,
        .dso = 0,
        .reserved = { },
    },
    .get_sensors_list = sensors__get_sensors_list,
};

struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

    sensors_poll_context_t();
    ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);

private:
    enum {
        light = 0,
        orientation,
        magnetic,
        acceleration,
        proximity,
        numSensorDrivers, // wake pipe goes here
        numFds,
    };

    struct pollfd mPollFds[numFds];
    SensorBase* mSensors[numSensorDrivers];

    static const size_t wake = numSensorDrivers;
    static const char WAKE_MESSAGE = 'W';
    int mWritePipeFd;

    // For keeping track of usage (only count from system)
    bool mAccelerationActive;
    bool mMagneticActive;
    bool mOrientationActive;

    int real_activate(int handle, int enabled);

    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_A:
                return acceleration;
            case ID_M:
                return magnetic;
            case ID_O:
                return orientation;
            case ID_L:
                return light;
            case ID_PX:
                return proximity;
        }
        return -EINVAL;
    }
};

sensors_poll_context_t::sensors_poll_context_t()
{
    int lightSensorType = SENSOR_TYPE_BH1721;
    bool hasProximity = false;

    ALOGV("%s+", __PRETTY_FUNCTION__);

    // populate the sensor list
    numSensors = LOCAL_SENSORS;

    // must clean this up early or else the destructor will make a mess
    memset(mSensors, 0, sizeof(mSensors));

    char device[16];
    FILE *f = fopen(DEVICE_VARIANT_SYSFS, "r");
    if (!f || fgets(device, 16, f) == NULL) {
        ALOGE("Failed to read " DEVICE_VARIANT_SYSFS ", assuming P51xx\n");
        strcpy(device, "espresso10");
    }
    if (f)
        fclose(f);

    ALOGD("Device: %s", device);

    if (strcmp(device, "espressowifi") == 0) {
        /* Device is P3110 */
        sSensorList[3].name = "AL3201 Light Sensor";
        sSensorList[3].vendor = "Lite-On";
        lightSensorType = SENSOR_TYPE_AL3201;
    } else if (strcmp(device, "espresso") == 0) {
        /* Device is P3100 */
        sSensorList[3].name = "GP2AP002 Light Sensor";
        sSensorList[3].vendor = "Sharp";
        lightSensorType = SENSOR_TYPE_GP2A;
        hasProximity = true;
    }

    if (hasProximity) {
        mSensors[proximity] = new ProximitySensor();
        mPollFds[proximity].fd = mSensors[proximity]->getFd();
        mPollFds[proximity].events = POLLIN;
        mPollFds[proximity].revents = 0;
    } else {
        numSensors -= 1;
    }

    mSensors[light] = new LightSensor(lightSensorType);
    mPollFds[light].fd = mSensors[light]->getFd();
    mPollFds[light].events = POLLIN;
    mPollFds[light].revents = 0;

    mSensors[acceleration] = new AccelerationSensor();
    mPollFds[acceleration].fd = mSensors[acceleration]->getFd();
    mPollFds[acceleration].events = POLLIN;
    mPollFds[acceleration].revents = 0;

    mSensors[magnetic] = new MagneticSensor();
    mPollFds[magnetic].fd = mSensors[magnetic]->getFd();
    mPollFds[magnetic].events = POLLIN;
    mPollFds[magnetic].revents = 0;

    mSensors[orientation] = new OrientationSensor();
    mPollFds[orientation].fd = mSensors[orientation]->getFd();
    mPollFds[orientation].events = POLLIN;
    mPollFds[orientation].revents = 0;

    /* Timer based sensor initialization */
    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result < 0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;

    mAccelerationActive = false;
    mMagneticActive = false;
    mOrientationActive = false;

    ALOGV("%s-", __PRETTY_FUNCTION__);
}

sensors_poll_context_t::~sensors_poll_context_t()
{
    for (int i = 0; i < numSensorDrivers; i++) {
        if (mSensors[i]) {
            delete mSensors[i];
            close(mPollFds[i].fd);
        }
    }
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled)
{
    int err;

    ALOGV("%s+: %d, %d", __PRETTY_FUNCTION__, handle, enabled);

    // Orientation requires accelerometer and magnetic sensor
    if (handle == ID_O) {
        mOrientationActive = enabled ? true : false;
        if (!mAccelerationActive) {
            err = real_activate(ID_A, enabled);
            if (err)
            	return err;
        }
        if (!mMagneticActive) {
            err = real_activate(ID_M, enabled);
            if (err)
            	return err;
        }
    }
    // Keep track of magnetic and accelerometer use from system
    else if (handle == ID_A) {
        mAccelerationActive = enabled ? true : false;
        // No need to enable or disable if orientation sensor is active as that will handle it
        if (mOrientationActive)
        	return 0;
    }
    else if (handle == ID_M) {
        mMagneticActive = enabled ? true : false;
        // No need to enable or disable if orientation sensor is active as that will handle it
        if (mOrientationActive)
        	return 0;
    }

    ALOGV("%s-", __PRETTY_FUNCTION__);

    return real_activate(handle, enabled);
}

int sensors_poll_context_t::real_activate(int handle, int enabled)
{
    ALOGV("%s+: %d, %d", __PRETTY_FUNCTION__, handle, enabled);

    int index = handleToDriver(handle);
    if (index < 0)
        return index;

    int err =  mSensors[index]->enable(handle, enabled);
    if (!err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result < 0,
                "error sending wake message (%s)", strerror(errno));
    }

    ALOGV("%s-", __PRETTY_FUNCTION__);

    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns)
{
    ALOGV("%s+: %d", __PRETTY_FUNCTION__, handle);

    int index = handleToDriver(handle);
    if (index < 0)
        return index;

    ALOGV("%s-", __PRETTY_FUNCTION__);

    return mSensors[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t *data, int count)
{
    int nbEvents = 0;
    int n = 0;

    ALOGV("%s+: %d", __PRETTY_FUNCTION__, count);

    do {
        // see if we have some leftover from the last poll()
        for (int i = 0; count && i < numSensorDrivers; i++) {
            SensorBase* const sensor(mSensors[i]);
            if (mSensors[i] && (mPollFds[i].revents & POLLIN || sensor->hasPendingEvents())) {
                int nb = sensor->readEvents(data, count);
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
                count -= nb;
                nbEvents += nb;
                data += nb;
            }
        }
        if (count) {
            do {
                n = poll(mPollFds, numFds, nbEvents ? 0 : -1);
            } while (n < 0 && errno == EINTR);
            if (n < 0) {
                ALOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & POLLIN) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                ALOGE_IF(result < 0, "error reading from wake pipe (%s)", strerror(errno));
                ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
        }
        // if we have events and space, go read them
    } while (n && count);

    ALOGV("%s-", __PRETTY_FUNCTION__);

    return nbEvents;
}

static int poll__close(struct hw_device_t *dev)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx)
        delete ctx;

    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
                          int handle, int enabled)
{
    ALOGV("%s", __PRETTY_FUNCTION__);
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
                          int handle, int64_t ns)
{
    ALOGV("%s", __PRETTY_FUNCTION__);
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
                      sensors_event_t* data, int count)
{
    ALOGV("%s", __PRETTY_FUNCTION__);
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

static int open_sensors(const struct hw_module_t* module,
                        const char* id __unused,
                        struct hw_device_t** device)
{
    ALOGV("%s", __PRETTY_FUNCTION__);
    int status = -EINVAL;
    sensors_poll_context_t *dev = new sensors_poll_context_t();

    memset(&dev->device, 0, sizeof(sensors_poll_device_t));

    dev->device.common.tag      = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = SENSORS_DEVICE_API_VERSION_1_0;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;

    *device = &dev->device.common;
    status = 0;

    return status;
}
