#!/bin/sh

# ====================================================
# A helper script called by QEMU to bring up a tap interface
# and add it to our network bridge
# ====================================================

# bridge name
BRIDGE="br0"

echo "Bringing up interface $1 and adding it to bridge $BRIDGE..."

# activating the tap interface
sudo ip link set "$1" up

# adding the interface to the bridge
sudo brctl addif "$BRIDGE" "$1"

echo "Interface $1 is ready."