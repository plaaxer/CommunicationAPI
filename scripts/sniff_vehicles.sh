#!/bin/bash

# ============================================================================
# A helper script to sniff the QEMU vehicle simulation network
# using the bridge and tap interfaces for precise analysis.
# Usage:
#   sudo ./sniff_vehicles.sh all
#   sudo ./sniff_vehicles.sh vehicle <ID>
#
# Examples:
#   sudo ./sniff_vehicles.sh all          # Sniffs all traffic between all VMs on the bridge.
#   sudo ./sniff_vehicles.sh vehicle 3    # Sniffs only the traffic for vehicle 3.
#
# ============================================================================

# configuration
BRIDGE_INTERFACE="br0"
# etherthype of the custom protocol. there is only one yet
CUSTOM_PROTO_HEX="0x88b5"

# checking root
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (sudo ./sniff_vehicles.sh)"
  exit 1
fi

# checking the arguments
if [ "$#" -lt 1 ]; then
    echo "Error: Missing mode."
    echo "Use: $0 all | vehicle <ID>"
    exit 1
fi

MODE=$1

# main

if [ "$MODE" == "all" ]; then
    echo "Sniffing ALL traffic on bridge: $BRIDGE_INTERFACE"
    echo "--------------------------------------------------------"
    sudo tcpdump -i "$BRIDGE_INTERFACE" -e -n -X "ether proto $CUSTOM_PROTO_HEX"

elif [ "$MODE" == "vehicle" ]; then
    if [ -z "$2" ]; then
        echo "Error: Missing vehicle ID for 'vehicle' mode."
        echo "Usage: $0 vehicle <ID>"
        exit 1
    fi
    
    VEHICLE_ID=$2
    TAP_INTERFACE="tap${VEHICLE_ID}"

    # Check if the specified tap interface exists
    if ! ip link show "$TAP_INTERFACE" > /dev/null 2>&1; then
        echo "Error: Interface '$TAP_INTERFACE' does not exist. Is vehicle $VEHICLE_ID running?"
        exit 1
    fi

    echo "Sniffing traffic for Vehicle $VEHICLE_ID on interface: $TAP_INTERFACE"
    echo "--------------------------------------------------------"
    sudo tcpdump -i "$TAP_INTERFACE" -e -n -X "ether proto $CUSTOM_PROTO_HEX"

else
    echo "Error: Unknown mode '$MODE'."
    echo "Usage: $0 all | vehicle <ID>"
    exit 1
fi
