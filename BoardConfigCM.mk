# F2FS filesystem
TARGET_USERIMAGES_USE_F2FS := true

# Vold
BOARD_VOLD_MAX_PARTITIONS := 12
BOARD_VOLD_EMMC_SHARES_DEV_MAJOR := true
BOARD_VOLD_DISC_HAS_MULTIPLE_MAJORS := true
TARGET_USE_CUSTOM_LUN_FILE_PATH := "/sys/class/android_usb/f_mass_storage/lun%d/file"

# Charger
BOARD_CHARGER_SHOW_PERCENTAGE := true

# Screencast
TARGET_USE_AVC_BASELINE_PROFILE := true
BOARD_NO_INTRA_MACROBLOCK_MODE_SUPPORT := true

# Recovery
TARGET_RECOVERY_DEVICE_DIRS += device/samsung/espressowifi
BOARD_HAS_DOWNLOAD_MODE := true

# SELinux
BOARD_SEPOLICY_DIRS += \
    device/samsung/espressowifi/sepolicy-custom

BOARD_SEPOLICY_UNION += \
    file_contexts \
    property.te \
    property_contexts \
    variant_setup.te
