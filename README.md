# Communication in Critical Autonomous Systems

Library for a reliable and secure communication
for critical autonomous systems, in the context of autonomous
vehicles.

This project is part of the class of Operational Systems II,
from the B.S. in Computer Science offered by @UFSC.

---

## Summary
- [Requirements](#requirements)
- [Components](#components-of-each-av-autonomous-vehicle)
- [Dependencies (Debian)](#dependencies-debian)
- [Running](#running)

---

## Requirements
- libc / C++ Standard Library in native POSIX platform
- Autonomous systems in this simulation (e.g. vehicle)
are represented by a macro object associated with a VM 
powered by QEMU
- Components for each Autonomous System are designed
to be a process in the virtualized OS

---

## Components of each AV (Autonomous Vehicle)
Each one will have modules following the model Sensing X Perception X Acting,
with ECUs and a RCU who will acts like a gateway. The Observer pattern will be
very utilized.

For now, only the Engine was used, to test the API.

The blueprint for your ECU was already created too, but the architecture
for ECUs <-> Components will be better organized yet.

---

## Dependencies (Debian)
- Compilers + Tmux + tcpdump
'''bash
sudo apt install g++-riscv64-linux-gnu tmux tcpdump bridge-utils tmux

---

## Running

1. Build the project
- Build
'''bash
make

3. Run the Simulation Script
'''bash
sudo ./run_simulation

To sniff the network inside the QEMU communication using tcpdump
1. Install tcpdump
'''bash
sudo apt install tcpdump

2. Run the Sniffing Script using
'''bash
sudo ./sniff_vehicles.sh <mode>

It is possible to sniff one particular vehicle or the entire private network

'''bash
sudo ./sniff_vehicles.sh vehicle <id>

or

'''bash
sudo ./sniff_vehicles.sh all