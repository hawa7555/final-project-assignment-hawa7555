#!/bin/sh
module=led_driver
device=aitutor_led
mode="666"
led_gpio_default=23

set -e

# Group: use staff if it exists, else wheel
if grep -q '^staff:' /etc/group; then
    group="staff"
else
    group="wheel"
fi

# Load the module using modprobe (works in Buildroot)
echo "Loading module ${module}"
if echo "$*" | grep -q "led_gpio="; then
    modprobe ${module} "$@" || exit 1
else
    modprobe ${module} led_gpio=${led_gpio_default} "$@" || exit 1
fi

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
echo "Device node: /dev/${device} (major: $major, minor: 0)"
ls -l /dev/${device}
