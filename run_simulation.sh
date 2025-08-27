#!/bin/bash

# creating new bridge device named 'br0'
sudo ip link add name br0 type bridge

# activating the bridge
sudo ip link set br0 up


# ====================================================
# Script to launch the VM's tiled in tmux
# ====================================================

# configuration
SESSION_NAME="vehicle_simulation"
VM_COUNT=5

# script

# checking if already has a session with the same name
tmux has-session -t $SESSION_NAME 2>/dev/null

if [ $? != 0 ]; then
    echo "Creating new tmux simulation session: '$SESSION_NAME' with $VM_COUNT VMs."

    # starting session in tmux
    tmux new-session -d -s $SESSION_NAME

    # running the first VM
    tmux send-keys -t $SESSION_NAME:0.0 "sudo make run-vehicle ID=1" C-m

    # loop to create rest of the VMs
    for i in $(seq 2 $VM_COUNT); do
        # splits horizontally to better view
        tmux split-window -h
        
        sleep 1.5

        # running the make that starts the VM in each pane
        tmux send-keys "sudo make run-vehicle ID=$i" C-m

        # automatically finding the best arrangement
        tmux select-layout tiled
    done

else
    echo "Session with name '$SESSION_NAME' already exists. Attaching."
fi

echo "Attaching to session. Use 'Ctrl-b d' to detach."
tmux attach-session -t $SESSION_NAME
