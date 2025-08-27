#!/bin/bash

# --- Configuration ---
SESSION_NAME="vehicle_sim"
VM_COUNT=5 

# --- Script Logic ---

# Check if a tmux session with this name is already running
tmux has-session -t $SESSION_NAME 2>/dev/null

if [ $? != 0 ]; then
    echo "Creating new tmux simulation session: '$SESSION_NAME' with $VM_COUNT VMs."

    # Start a new, detached tmux session
    tmux new-session -d -s $SESSION_NAME

    # --- Create panes in a loop ---
    # Run the first VM in the initial pane
    tmux send-keys -t $SESSION_NAME:0.0 "make run-vehicle ID=1" C-m

    # Loop to create and run the rest of the VMs
    for i in $(seq 2 $VM_COUNT); do
        # Split the current window vertically. The new pane becomes active.
        tmux split-window -v

        # Run the make command in the new pane
        tmux send-keys "make run-vehicle ID=$i" C-m
    done

    # After creating all panes, tell tmux to arrange them in a tiled layout.
    # This automatically finds the best grid arrangement for the number of panes.
    tmux select-layout tiled

else
    echo "Session '$SESSION_NAME' already exists. Attaching."
fi

# Attach to the session to view the VMs
echo "Attaching to session. Use 'Ctrl-b d' to detach."
tmux attach-session -t $SESSION_NAME
