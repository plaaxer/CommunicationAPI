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
- [Roadside Unit](#roadside-unit)
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

In the **sixth release**, the communication API has been evolved to support **Secure Group Communication**. 

The Gateway now integrates an `IGroupHandler` to manage dynamic group membership based on geographic **Quadrants**. It enforces security by verifying a Message Authentication Code (MAC) on every received packet, ensuring integrity and authenticity using a Session Key provided by the Group Leader.

Additionally, the Gateway continues to handle typed message subscriptions (TEDS) and maintains the Slave Synchronizer for the PTP (Precision Time Protocol) stack.


## Roadside Unit

In this release, the Roadside Unit (RSU) assumes the role of **Group Leader** for its specific Quadrant. It is responsible for:

1.  **Time Synchronization:** Acting as the PTP Master (Grandmaster Clock).
2.  **Key Distribution:** Generating and distributing the cryptographic Session Key to vehicles that send `JOIN` requests upon entering the quadrant.
3.  **Member Management:** Monitoring group members and broadcasting notifications when members leave.


## Secure Group Communication

The simulation environment is now logically divided into **Quadrants**.

- **Dynamic Movement:** Vehicles simulate movement between quadrants via the external NIC. When a quadrant change is detected, the Protocol layer triggers a `JOIN` handshake.
- **Join Protocol:** Vehicles broadcast a `JOIN` request. The local RSU responds with a unicast `KEY_DISTRIBUTION` message containing the active Session Key.
- **MAC Authentication:** All data packets now include a **Message Authentication Code (MAC)** appended to the payload. This is calculated via an `ICryptoProvider` interface (currently using XOR) and the Session Key. Packets with invalid MACs are silently discarded.
- **Nearby Entities:** The Shared Memory Engine now maintains a list of "nearby entities" (neighbors in the same quadrant). Application components (like TEDS Handlers) monitor this list to optimize transmission, stopping data flow if interested subscribers move away.


## Dependencies (Debian)

Compilers + tmux
```bash
sudo apt install g++-riscv64-linux-gnu tmux
```

## Running

### Simple run

It compiles the project, busybox and the Linux Kernel image (if it isn't already at the `/os` folder), also follows the basic steps of the initramfs files creation. It is already configured to run the project and a short simulation. To configure a longest simulation, modify the RUN_TIME variable presented in scripts/run_simulation.sh.

```bash
make
```

This default command runs 5 virtual machines with 7 processes (including the gateway) each.

Note: the defined path to the Image and the initramfs.cpio is the `os/`. You can manually put your already compiled Image by creating the `os/`, saving compilation time.


### Custom run

Calls a configured shell script to run the simulation choosing the number of VM's and components.

```bash
# Replace <n_components> and <n_vms> with numbers
make run COMPS=<n_components> VM=<n_vms>
```

The logging features received packets by components. As the testing only sends packets to components whose port is 1000, they are the only one to print those. Latency calculations, made with RTT using ping and echo, can also be found in the logging.

### Cleaning

Removing the project build in `busybox/_install/` and items like the initramfs.cpio that need constant re-building when the source code changes.

```bash
make clean
```

Note: obviously, the Image and Busybox will not be removed.


## Latency Analysis

By adding the flag `LATENCY=1` to `make run`, we map the standard output (and logging) of each VM instance to files called vm<num>.log, making the analysis way easier.

Right after running, you can already see the logs constinuosly being writed in these files.

The analyzer is called automatically, but you can run manually with: 

```bash
python3 scripts/latency_analyzer.py
```


