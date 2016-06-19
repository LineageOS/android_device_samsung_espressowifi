#
# Copyright (C) 2012-2015 The CyanogenMod Project
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

# Include common omap4 makefile
$(call inherit-product, hardware/ti/omap4/omap4.mk)

DEVICE_PACKAGE_OVERLAYS += device/samsung/espressowifi/overlay/aosp-common

LOCAL_PATH := device/samsung/espressowifi

PRODUCT_AAPT_CONFIG := large
PRODUCT_AAPT_PREF_CONFIG := mdpi

# Ramdisk
PRODUCT_PACKAGES += \
    fstab.espresso \
    init.espresso.usb.rc \
    init.espresso.rc \
    ueventd.espresso.rc

# Recovery Ramdisk
PRODUCT_PACKAGES += \
    init.recovery.espresso.rc

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init.espresso.variant.sh:system/bin/init.espresso.variant.sh

# GPS
# gps config appropriate for this device
$(call inherit-product, device/common/gps/gps_us_supl.mk)

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/gps.xml:system/etc/gps.xml

# Wifi
PRODUCT_PACKAGES += \
    libwpa_client \
    hostapd \
    dhcpcd.conf \
    wpa_supplicant \
    wpa_supplicant.conf

PRODUCT_PROPERTY_OVERRIDES += \
    wifi.interface=wlan0

# Media profiles
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/media_codecs.xml:system/etc/media_codecs.xml \
    $(LOCAL_PATH)/configs/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml \
    $(LOCAL_PATH)/configs/media_profiles.xml:system/etc/media_profiles.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:system/etc/media_codecs_google_telephony.xml

# Keylayout
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/usr/keychars/espresso-gpio-keypad.kcm:system/usr/keychars/espresso-gpio-keypad.kcm \
    $(LOCAL_PATH)/usr/keychars/sec_keyboard.kcm:system/usr/keychars/sec_keyboard.kcm \
    $(LOCAL_PATH)/usr/keylayout/espresso-gpio-keypad.kl:system/usr/keylayout/espresso-gpio-keypad.kl \
    $(LOCAL_PATH)/usr/keylayout/sec_keyboard.kl:system/usr/keylayout/sec_keyboard.kl

# Packages
PRODUCT_PACKAGES += \
    audio.a2dp.default \
    audio.primary.piranha \
    audio.r_submix.default \
    audio.usb.default \
    camera.omap4 \
    lights.piranha \
    libinvensense_mpl \
    power.piranha \
    sensors.piranha \
    geomagneticd \
    orientationd

# Storage
PRODUCT_PROPERTY_OVERRIDES += \
    persist.sys.isUsbOtgEnabled=true

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp

# Charger
PRODUCT_PACKAGES += \
    charger_res_images

# Samsung dock keyboard
PRODUCT_PACKAGES += \
    dock_kbd_attach

# These are the hardware-specific features
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.audio.low_latency.xml:system/etc/permissions/android.hardware.audio.low_latency.xml \
    frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.location.xml:system/etc/permissions/android.hardware.location.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.xml:system/etc/permissions/android.hardware.touchscreen.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml

PRODUCT_CHARACTERISTICS := tablet

PRODUCT_PROPERTY_OVERRIDES += \
    ro.opengles.version=131072 \
    ro.bq.gpu_to_cpu_unsupported=1 \
    ro.sf.lcd_density=160

PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.dex2oat-flags=--no-watch-dog

$(call inherit-product-if-exists, vendor/samsung/espresso-common/espresso-common-vendor.mk)
$(call inherit-product, frameworks/native/build/tablet-dalvik-heap.mk)
$(call inherit-product-if-exists, hardware/broadcom/wlan/bcmdhd/firmware/bcm4330/device-bcm.mk)
