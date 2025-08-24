#!/bin/sh

# Mount essential virtual filesystems
mount -t proc none /proc
mount -t sysfs none /sys

echo "================================="
echo "Init script started."
echo "Waiting for devices to settle..."
sleep 1

# Manually load the network driver module
echo "Loading virtio_net driver..."
modprobe virtio_net

# Bring the network interface up
echo "Bringing up eth0..."
ip link set dev eth0 up

echo "Network interface is up. Launching application."
echo "================================="

# Execute the main application
/test_nic

# This part will only be reached if /test_nic exits.
# We can enter an interactive shell for debugging.
echo "Application exited. Entering debug shell."
exec /bin/sh