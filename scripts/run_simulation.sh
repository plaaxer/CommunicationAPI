#!/bin/bash

# ====================================================
# Script to launch the VM's tiled in tmux
# ====================================================

# --- Configuration ---
SESSION_NAME="vehicle_simulation"
VM_COUNT=
IMAGE_SRC="os/Image"
INITRD_VEHICLE_SRC="os/initramfs_vehicle.cpio"
INITRD_RSU_SRC="os/initramfs_rsu.cpio"
RUN_TIME=90   # Run simulation for 90 seconds
NUM_RSUS=4

LOGS_BASE_DIR="logs"
VEHICLE_LOG_DIR="$LOGS_BASE_DIR/vehicle_files"
RSU_LOG_DIR="$LOGS_BASE_DIR/rsu_files"
# ---------------------

# Logs directories
rm -rf $LOGS_BASE_DIR
mkdir -p $LOGS_BASE_DIR

mkdir -p $RSU_LOG_DIR
mkdir -p $VEHICLE_LOG_DIR

while getopts "v:" opt; do
    case $opt in
        v) VM_COUNT="$OPTARG" ;;
        *) echo "Usage: $0 [-v <num_vms>]"; exit 1 ;;
    esac
done

# Default: do not use tee (to no logs)
USE_TEE=0
if [ "$LOG_FLAG" == "1" ]; then
    USE_TEE=1
fi

# Check if the tmux session already exists
tmux has-session -t $SESSION_NAME 2>/dev/null

# If the session doesn't exist ($? is not 0), create it and launch the VMs
if [ $? != 0 ]; then
    echo "Creating new tmux session '$SESSION_NAME' with $NUM_RSUS RSUs and $VM_COUNT VMs..."

    # Start a new, detached tmux session
    tmux new-session -d -s $SESSION_NAME

    # Starts 4 RSUs with quadrants 0, 1, 2, 3
    for i in $(seq 0 $(($NUM_RSUS - 1))); do
        # Generate a unique MAC (e.g., 0a, 0b, 0c, 0d)
        MAC_SUFFIX_RSU=$(printf "%02x" $((i + 10))) 
        RSU_LOG_FILE="rsu${i}.log"

        RSU_CMD="qemu-system-riscv64 \
            -machine virt \
            -nographic \
            -kernel ${IMAGE_SRC} \
            -initrd ${INITRD_RSU_SRC} \
            -append 'root=/dev/ram rw console=ttyS0 quadrant=${i} log_file=${RSU_LOG_FILE}' \
            -netdev socket,id=vlan0,mcast=230.0.0.1:1234 \
            -icount shift=0,align=on \
            -device virtio-net,id=eth0,netdev=vlan0,mac=52:54:00:12:34:${MAC_SUFFIX_RSU} \
            -fsdev local,id=rsu${i}_log_fs,path=${RSU_LOG_DIR},security_model=none \
            -device virtio-9p-pci,fsdev=rsu${i}_log_fs,mount_tag=host_log"

        # it pipes the QEMU instances terminals to the log file
        if [ "$USE_TEE" -eq 1 ]; then
            RSU_CMD="$RSU_CMD | tee logs/rsu${i}.log"
        fi

        # Start the first RSU in the main pane, split for the rest
        if [ $i -eq 0 ]; then
            tmux send-keys -t $SESSION_NAME "$RSU_CMD" C-m
        else
            tmux split-window -t $SESSION_NAME -h
            tmux send-keys -t $SESSION_NAME "$RSU_CMD" C-m
        fi
        
        # Rearrange panes into a tiled layout after each launch
        tmux select-layout -t $SESSION_NAME tiled
    done


    # Start 5 vehicles
    for i in $(seq 1 $VM_COUNT); do
        tmux split-window -t $SESSION_NAME -h

        # Generate a unique MAC (e.g., 01, 02, ... 0f, 10)
        MAC_SUFFIX_VEHICLE=$(printf "%02x" $i)
        VEHICLE_LOG_FILE="vm${i}.log"

        VEHICLE_CMD="qemu-system-riscv64 \
            -machine virt \
            -nographic \
            -kernel ${IMAGE_SRC} \
            -initrd ${INITRD_VEHICLE_SRC} \
            -append 'root=/dev/ram rw console=ttyS0 quadrant=$(((i - 1) % 4 )) log_file=${VEHICLE_LOG_FILE}' \
            -netdev socket,id=vlan0,mcast=230.0.0.1:1234 \
            -icount shift=0,align=on \
            -device virtio-net,id=eth0,netdev=vlan0,mac=52:54:00:12:34:${MAC_SUFFIX_VEHICLE} \
            -fsdev local,id=vm${i}_log_fs,path=${VEHICLE_LOG_DIR},security_model=none    \
            -device virtio-9p-pci,fsdev=vm${i}_log_fs,mount_tag=host_log"

        # it pipes the QEMU instances terminals to the log file
        if [ "$USE_TEE" -eq 1 ]; then
            VEHICLE_CMD="$VEHICLE_CMD | tee logs/vm${i}.log"
        fi

        tmux send-keys -t $SESSION_NAME "$VEHICLE_CMD" C-m

        # Rearrange panes into a tiled layout for best visibility
        tmux select-layout -t $SESSION_NAME tiled
    done

else
    echo "Session '$SESSION_NAME' already exists. Attaching."
fi

# start background watcher to kill after run_time
(
    sleep $RUN_TIME
    echo "Time is up. Killing all QEMU VMs and tmux session..."
    pkill -f "qemu-system-riscv64"  # kills all QEMU instances
    tmux kill-session -t $SESSION_NAME
) &

# attach to the tmux session
echo "Attaching to session. Use 'Ctrl-b d' to detach."
sleep 1
tmux attach-session -t $SESSION_NAME


# TO ADJUST THE TEST TO SUPPORT THE RSU ACTUATION VALIDATION
python3 scripts/latency_analyzer.py logs/vehicle_files/latency.log