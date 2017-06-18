# Copyright (C) 2013 Paul Kocialkowski <contact@paulk.fr>
# Copyright (C) 2016 Dániel Járai <jaraidaniel@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

LOCAL_PATH := $(call my-dir)

LIBSENSORS_PATH := $(LOCAL_PATH)

# HAL module implemenation stored in
# hw/<SENSORS_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	sensors.cpp \
	InputEventReader.cpp \
	SensorBase.cpp \
	AccelerationSensor.cpp \
	LightSensor.cpp \
	MagneticSensor.cpp \
	OrientationSensor.cpp \
	ProximitySensor.cpp

LOCAL_C_INCLUDES := \
	$(LIBSENSORS_PATH)

LOCAL_CFLAGS := -Wall -Werror

LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libhardware
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := sensors.omap4
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(LIBSENSORS_PATH)/geomagneticd

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	geomagneticd.c \
	input.c

LOCAL_CFLAGS := -Wall -Werror

LOCAL_SHARED_LIBRARIES := libutils libcutils liblog
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := geomagneticd
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

LOCAL_PATH := $(LIBSENSORS_PATH)/orientationd

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	orientationd.c \
	input.c \
	bma250.c \
	yas530.c

LOCAL_CFLAGS := -Wall -Werror

LOCAL_SHARED_LIBRARIES := libutils libcutils liblog
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := orientationd
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
