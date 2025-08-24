# Communication in Critical Autonomous Systems

Library for a reliable and secure communication
for critical autonomous systems, in the context of autonomous
vehicles.

This project is part of the class of Operational Systems II,
from the B.S. in Computer Science offered by @UFSC.

---

## Summary
- [Requirements](#requirements)
- [Components](#-components-of-each-av-autonomous-vehicle)
- [Dependencies]()
- [Running]()

---

## Requirements

- libc / C++ Standard Library in native POSIX platform
- Autonomous systems in this simulation (e.g. vehicle)
are represented by a macro object associated with a VM 
powered by QEMU
- Components for each Autonomous System are designed
to be a process in the virtualized OS


## Components of each AV (Autonomous Vehicle)

To define yet



## Dependencies (Debian)

'''bash
sudo apt install g++-riscv64-linux-gnu


## Running

- Build
'''bash
make

- Creating initramfs.cpio
'''bash
make initramfs

- Instantianting 5 QEMU VM's with different MAC's
'''bash
qemu-system-riscv64 -machine virt -nographic -kernel Image -initrd initramfs.cpio -append "root=/dev/ram rw" -netdev socket,id=vlan0,mcast=230.0.0.1:1234 -device virtio-net,netdev=vlan0,mac=52:54:00:12:34:00

qemu-system-riscv64 -machine virt -nographic -kernel Image -initrd initramfs.cpio -append "root=/dev/ram rw" -netdev socket,id=vlan0,mcast=230.0.0.1:1234 -device virtio-net,netdev=vlan0,mac=52:54:00:12:34:01

qemu-system-riscv64 -machine virt -nographic -kernel Image -initrd initramfs.cpio -append "root=/dev/ram rw" -netdev socket,id=vlan0,mcast=230.0.0.1:1234 -device virtio-net,netdev=vlan0,mac=52:54:00:12:34:02

qemu-system-riscv64 -machine virt -nographic -kernel Image -initrd initramfs.cpio -append "root=/dev/ram rw" -netdev socket,id=vlan0,mcast=230.0.0.1:1234 -device virtio-net,netdev=vlan0,mac=52:54:00:12:34:03

qemu-system-riscv64 -machine virt -nographic -kernel Image -initrd initramfs.cpio -append "root=/dev/ram rw" -netdev socket,id=vlan0,mcast=230.0.0.1:1234 -device virtio-net,netdev=vlan0,mac=52:54:00:12:34:04


