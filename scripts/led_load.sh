#!/bin/sh
module=led_driver
device=aitutor_led
mode="666"

set -e

# Group: use staff if it exists, else wheel
if grep -q '^staff:' /etc/group; then
    group="staff"
else
    group="wheel"
fi

# Load module
echo "Loading module ${module}"
modprobe ${module} \
    gpio_base=0 \
    led_gpio=24 \
    red_led_gpio=23 \
    start_button_gpio=17 \
    cancel_button_gpio=27 \
    "$@" || exit 1

# Get major number from /proc/devices
major=$(awk "\$2==\"$device\" {print \$1}" /proc/devices)
if [ -z "$major" ]; then
    echo "Error: could not find device $device in /proc/devices"
    exit 1
fi

# Remove old device node and create new one
rm -f /dev/${device}
mknod /dev/${device} c "$major" 0
chgrp "$group" /dev/${device} 2>/dev/null || true
chmod "$mode" /dev/${device}

echo "LED driver loaded successfully"
echo "Green LED: GPIO 24, Red LED: GPIO 23"
echo "START button: GPIO 17, CANCEL button: GPIO 27"
echo "Device node: /dev/${device} (major: $major, minor: 0)"
ls -l /dev/${device}
