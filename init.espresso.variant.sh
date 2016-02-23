#!/system/bin/sh
# Copyright (c) 2016 OMAP4-AOSP
# Copyright (c) Ketut P. Kumajaya, Sept 2013

export PATH=/system/bin

DEVICE=/sys/board_properties/type

if [ -f "$DEVICE" ]
then
  variant=`cat $DEVICE`
  case "$variant" in
      "espresso" | "espressowifi")
            # set p31xx to portrait mode
            setprop ro.sf.hwrotation 270

            # set mms136 touchcreen driver orientation
            echo 1 > /sys/devices/virtual/sec/tsp/pivot

            # set accelerometer position
            position=$(find /sys/devices/virtual/input/ -type f -name name | xargs grep '^accelerometer$' | sed 's@name:accelerometer@position@')
            echo -n 6 > $position

            # set geomagnetic position
            position=$(find /sys/devices/platform/omap/omap_i2c.4/i2c-4/4-002e/input/ -type f -name name | xargs grep '^geomagnetic$' | sed 's@name:geomagnetic@position@')
            echo -n 6 > $position
            ;;
  esac
fi
