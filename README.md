# Interprocess Communication Library for Autonomous Vehicle Simulation

A C++17 communication library implementing zero-copy IPC and low-latency networking for simulated autonomous vehicle systems. Built as the final project for Operational Systems II at UFSC, the library supports both intra-VM process communication (System V shared memory) and inter-VM network communication (raw Ethernet sockets), with time synchronization (PTP), secure group management, and cryptographic message authentication.

The simulation runs on RISC-V virtual machines orchestrated via QEMU, where each VM represents an autonomous vehicle or roadside unit (RSU).

---

## Architecture

```
┌──────────────────────────────────────────────────────────┐
│                        Vehicle VM                        │
│                                                          │
│  [Sensor Component] [Sensor Component] [...]             │
│         │                   │                            │
│         └─────────┬─────────┘                            │
│               ShmEngine                                  │
│          (System V shared memory,                        │
│           circular buffer + semaphores)                  │
│               │                                          │
│           [Gateway / RCU]                                │
│               │                                          │
│         RawSocketEngine                                  │
│     (AF_PACKET raw Ethernet + SIGIO)                     │
└──────────────────┬───────────────────────────────────────┘
                   │  Ethernet (QEMU virtual network)
┌──────────────────┴───────────────────────────────────────┐
│               Roadside Unit VM (RSU)                     │
│       PTP Master · Group Leader · Key Distributor        │
└──────────────────────────────────────────────────────────┘
```

### Key Components

| Component | Description |
|---|---|
| `ShmEngine` | Zero-copy IPC via System V shared memory. Multi-writer/multi-reader circular buffer with semaphore-based synchronization and dynamic service registration. |
| `RawSocketEngine` | Inter-VM networking using `AF_PACKET` raw sockets with async I/O (`SIGIO`/`O_ASYNC`) and a custom Ethernet protocol. |
| `Protocol<LocalNIC, ExternalNIC>` | Meyers' Singleton routing layer. Bridges intra- and inter-VM traffic, enforces MAC authentication, and dispatches to port-based or type-based (TEDS) observers. |
| `PTP Stack` | Precision Time Protocol implementation. RSU acts as master; vehicle gateways act as slaves, measuring RTT to adjust system clock. |
| `Group Management` | Quadrant-based membership. Vehicles broadcast `JOIN` on quadrant entry; RSU responds with `KEY_DISTRIBUTION`. All data packets carry a Message Authentication Code (MAC). |
| `CryptoService` | Provider-pattern MAC generation via pluggable `ICryptoProvider` (default: XOR-based). Packets with invalid MACs are silently dropped. |

---

## Performance

- **Intra-VM latency:** sub-millisecond IPC via zero-copy shared memory buffers
- **Inter-VM latency:** below 0.5 ms average RTT measured over raw Ethernet frames between QEMU VMs
- **Throughput:** concurrent multi-writer/multi-reader circular buffer with 32-slot capacity and backpressure via semaphores

---

## Dependencies

Tested on Debian/Ubuntu. Requires a RISC-V cross-compiler and tmux:

```bash
sudo apt install g++-riscv64-linux-gnu tmux
```

The build system fetches and compiles the Linux 6.15.5 kernel and BusyBox automatically if they are not present in the `os/` directory.

---

## Build & Run

### Default simulation

Compiles the project, builds the kernel image and initramfs (if not already present), and launches 5 VMs with 7 processes each (1 gateway + 6 sensor components):

```bash
make
```

You can skip kernel compilation by placing a pre-built `Image` and `initramfs.cpio` in the `os/` directory.

### Custom configuration

```bash
make run COMPS=<n_components> VM=<n_vms>
```

### Latency analysis

Redirect VM output to log files and run the analyzer:

```bash
LATENCY=1 make run
python3 scripts/latency_analyzer.py
```

Log files are written to `vm<n>.log`. The analyzer extracts RTT measurements and summarizes latency statistics.

### Clean build artifacts

```bash
make clean
```

This removes compiled binaries and the initramfs. The kernel image and BusyBox are preserved.

---

## Project Structure

```
source/
├── api/network/
│   ├── engines/          # ShmEngine (IPC) and RawSocketEngine (network)
│   ├── definitions/      # Protocol structures: frames, packets, TEDS types, quadrants
│   ├── ptp/              # Precision Time Protocol (master + slave)
│   ├── groups/           # Group membership and key distribution
│   ├── crypto/           # MAC generation interface and implementations
│   ├── protocol.hpp      # Core routing layer (Protocol<LocalNIC, ExternalNIC>)
│   └── nic.hpp           # NIC template wrapping any engine
├── vm/
│   ├── gateway.hpp       # Gateway/RCU initialization
│   ├── vehicle/          # Vehicle entry point and sensor components
│   └── roadside-unit/    # RSU entry point
└── handlers/             # Application-level message handlers (TEDS, latency test)
scripts/
├── run_simulation.sh     # QEMU orchestration
└── latency_analyzer.py   # RTT log parser
```
