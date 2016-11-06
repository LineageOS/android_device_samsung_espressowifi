# Charger
BOARD_CHARGER_SHOW_PERCENTAGE := true

# Screencast
TARGET_USE_AVC_BASELINE_PROFILE := true
BOARD_NO_INTRA_MACROBLOCK_MODE_SUPPORT := true

# F2FS filesystem
TARGET_USERIMAGES_USE_F2FS := true

# Liblights
TARGET_PROVIDES_LIBLIGHT := true

# Recovery
TARGET_RECOVERY_DEVICE_DIRS += device/samsung/espressowifi
BOARD_HAS_DOWNLOAD_MODE := true

# SELinux
BOARD_SEPOLICY_DIRS += \
    device/samsung/espressowifi/sepolicy-custom
