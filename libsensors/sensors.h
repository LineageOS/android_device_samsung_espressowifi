/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef ANDROID_SENSORS_H
#define ANDROID_SENSORS_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>

__BEGIN_DECLS

/*****************************************************************************/

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

enum {
    ID_A = 1,
    ID_M,
    ID_O,
    ID_L,
    ID_PX,
};

enum {
	SENSOR_TYPE_BH1721 = 1,
	SENSOR_TYPE_AL3201,
	SENSOR_TYPE_GP2A,
};

#define DEVICE_VARIANT_SYSFS "/sys/board/type"

#define EVENT_TYPE_PROXIMITY        ABS_DISTANCE
#define EVENT_TYPE_LIGHT            REL_MISC

/*****************************************************************************/

__END_DECLS

#endif  /* ANDROID_SENSORS_H */
