# Communication in Critical Autonomous Systems

Library for a reliable and secure communication
for critical autonomous systems, in the context of autonomous
vehicles.

This project is part of the class of Operational Systems II,
from the B.S. in Computer Science offered by @UFSC.

---

## Summary
- [Requirements](#requirements)
- [Components](#components-of-each-autonomous-vehicle)
- [Dependencies (Debian)](#dependencies-debian)
- [Running](#running)
- [Latency](#latency-analysis)



## Requirements
- libc / C++ Standard Library in native POSIX platform
- Autonomous systems in this simulation (e.g. vehicle)
are represented by a macro object associated with a VM 
powered by QEMU
- Components for each Autonomous System are designed
to be a process in the virtualized OS



## Components of each Autonomous Vehicle

For the second release we present an initial version of the LISHA's Smart Data API, preserving the suggested Observer x Observed pattern and developing a Component template class to consume the Local-Smart-Data end-point. To focus on the network, no further detail was implemented now, making using just of components with sensor capabilities.

Further in the nexts releases we will cover a more enriched SmartData specification, defining the Units consistently with the real world and powering our project with components modes and typed Interest and Command messages. This way, we will easily implement ECU's and actuators, but, for now, this backbone is sufficient to develop the communication.

Note: for more information, see the Documentation specifications.


## Dependencies (Debian)

Compilers + tmux
```bash
sudo apt install g++-riscv64-linux-gnu tmux
```

## Running

### Simple run

It compiles the project, busybox and the Linux Kernel image, also follows the basic steps of the initramfs.cpio creation. It is already configured to run the project.

```bash
make
```

Note: the defined path to the Image and the initramfs.cpio is the os/. You can manually put your already compiled Image by creating the os/, saving compilation time.


### Custom run

Calls a configured shell script to run the simulation choosing the number of VM's and components.

```bash
# Replace <n_components> and <n_vms> with numbers
make run COMPS=<n_components> VM=<n_vms>
```


### Cleaning

Removing the project build in busybox/_install/ and items like the initramfs.cpio that need constant re-building when the source code changes.

```bash
make clean
```

Note: obviously, the Image will stay in the os/ folder.


## Latency Analysis

We redirect the default output of each VM instance to files called vm<num>.log, making the analysis way easier.

Right after run with "make", you can already see the logs constinuosly being writed in these files.

To run the analyzer script, type the follow command after you end the simulation or while running for a considerable amount of time

```bash
python3 latency_analyzer.py
```


## Documentation

This project uses Doxygen to automaticaly generate class diagrams in the form of html files. In order to generate them, you must have Doxygen and Dot installed in your machine. Otherwise, you may view the last version of the documentation by accessing the file html\index.html.

In order to update the documentation, run doxygen Doxyfile


