#!/bin/bash

# ====================================================
# Script to launch the VM's tiled in tmux
# ====================================================

# --- Configuration ---
SESSION_NAME="vehicle_simulation"
VM_COUNT=
IMAGE_SRC="os/Image"
INITRD_SRC="os/initramfs.cpio"
# ---------------------

while getopts "v:" opt; do
    case $opt in
        v) VM_COUNT="$OPTARG" ;;
        *) echo "Usage: $0 [-v <num_vms>]"; exit 1 ;;
    esac
done

# Check if the tmux session already exists
tmux has-session -t $SESSION_NAME 2>/dev/null

# If the session doesn't exist ($? is not 0), create it and launch the VMs
if [ $? != 0 ]; then
    echo "Creating new tmux session '$SESSION_NAME' with $VM_COUNT VMs..."

    # Start a new, detached tmux session and launch the first VM
    tmux new-session -d -s $SESSION_NAME
    tmux send-keys -t $SESSION_NAME \
        "qemu-system-riscv64 \
            -machine virt \
            -nographic \
            -kernel ${IMAGE_SRC} \
            -initrd ${INITRD_SRC} \
            -append 'root=/dev/ram rw console=ttyS0 vehicle_id=vehicle-01' \
            -netdev socket,id=vlan0,mcast=230.0.0.1:1234 \
            -icount shift=0,align=on \
            -serial file:vm1.log \
            -device virtio-net,id=eth0,netdev=vlan0,mac=52:54:00:12:34:01" C-m

    # Loop to create and run the rest of the VMs in split panes
    for i in $(seq 2 $VM_COUNT); do
        tmux split-window -t $SESSION_NAME -h

        tmux send-keys -t $SESSION_NAME \
            "qemu-system-riscv64 \
                -machine virt \
                -nographic \
                -kernel ${IMAGE_SRC} \
                -initrd ${INITRD_SRC} \
                -append 'root=/dev/ram rw console=ttyS0 vehicle_id=vehicle-0${i}' \
                -netdev socket,id=vlan0,mcast=230.0.0.1:1234 \
                -icount shift=0,align=on \
                -serial file:vm${i}.log \
                -device virtio-net,id=eth0,netdev=vlan0,mac=52:54:00:12:34:0${i}" C-m

        # Rearrange panes into a tiled layout for best visibility
        tmux select-layout -t $SESSION_NAME tiled
    done

else
    echo "Session '$SESSION_NAME' already exists. Attaching."
fi

# Attach to the tmux session
echo "Attaching to session. Use 'Ctrl-b d' to detach."
sleep 1
tmux attach-session -t $SESSION_NAME