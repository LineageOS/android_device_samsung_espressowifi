#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := device/samsung/espressowifi

# Recovery Ramdisk
PRODUCT_PACKAGES += \
    $(LOCAL_PATH)/recovery/root/init.recovery.espresso.rc:recovery/root/init.recovery.espresso.rc

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init.espresso.variant.sh:system/bin/init.espresso.variant.sh
