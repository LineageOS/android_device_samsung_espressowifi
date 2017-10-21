# Charger
BOARD_CHARGER_SHOW_PERCENTAGE := true
BOARD_NO_CHARGER_LED := true

# F2FS filesystem
TARGET_USERIMAGES_USE_F2FS := true

# Liblights
TARGET_PROVIDES_LIBLIGHT := true

# Recovery
TARGET_RECOVERY_DEVICE_DIRS += device/samsung/espressowifi
BOARD_HAS_DOWNLOAD_MODE := true
LZMA_RAMDISK_TARGETS := recovery

# SELinux
BOARD_SEPOLICY_DIRS += \
    device/samsung/espressowifi/sepolicy-custom
