#!/system/bin/sh
# Copyright (c) 2016 OMAP4-AOSP
#

export PATH=/system/bin


DEVICE=/sys/board_properties/type

if [ -f "$DEVICE" ]
then
  variant=`cat $DEVICE`
  case "$variant" in
      "espresso" | "espressowifi")
            # set p31xx to portrait mode
            setprop ro.sf.hwrotation 270
            ;;
  esac
fi
