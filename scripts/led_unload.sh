#!/bin/sh
module=led_driver
device=aitutor_led

cd "$(dirname "$0")"
set -e

# Remove /dev node if present
if [ -e /dev/${device} ]; then
    echo "Removing /dev/${device}"
    rm -f /dev/${device}
fi

# Remove module if loaded
if lsmod | grep -q "^${module}"; then
    echo "Removing module ${module}"
    rmmod "${module}"
else
    echo "Module ${module} not loaded"
fi

echo "LED driver unloaded"

