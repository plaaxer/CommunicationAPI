#!/bin/bash

# configuration
SESSION_NAME="vehicle_sim"
VM_COUNT=5 

# checking if already exists tmux session with this name 
tmux has-session -t $SESSION_NAME 2>/dev/null

if [ $? != 0 ]; then
    echo "Creating new tmux simulation session: '$SESSION_NAME' with $VM_COUNT VMs."

    # starting a tmux session
    tmux new-session -d -s $SESSION_NAME

    # running the first VM in the initial pane
    tmux send-keys -t $SESSION_NAME:0.0 "make run-vehicle ID=1" C-m

    # loop to create VM's
    for i in $(seq 1 $VM_COUNT); do
        # Split the current window vertically. The new pane becomes active.
        tmux split-window -v

        # running make commands for instantiating the VMs
        tmux send-keys "make run-vehicle ID=$i" C-m
    done

    # automatically finding the best grid arrangement
    tmux select-layout tiled

else
    echo "Session '$SESSION_NAME' already exists. Attaching."
fi

# attaching to the session to view the VMs
echo "Attaching to session. Use 'Ctrl-b d' to detach."
tmux attach-session -t $SESSION_NAME
