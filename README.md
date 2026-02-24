# ASA-EMulatR — Alpha System Architecture Emulator

**A high-fidelity emulator for the DEC Alpha AXP architecture, targeting the AlphaServer ES40 (EV6/21264) platform.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE.md)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows%20x64-brightgreen.svg)]()
[![Built with Qt](https://img.shields.io/badge/Built%20with-Qt%206-41cd52.svg)](https://www.qt.io/)
[![Build: CMake](https://img.shields.io/badge/Build-CMake-064f8c.svg)]()

---

## Overview

ASA-EMulatR is a cycle-approximate emulator for the Alpha AXP 64-bit RISC architecture, built on the Qt 6 framework with C++17. The project aims to boot Tru64 UNIX, OpenVMS Alpha, and Linux/Alpha by faithfully emulating the Alpha 21264 (EV6) processor, PALcode execution environment, and ES40 system hardware.

### Key Design Principles

- **Architectural fidelity** — instruction semantics derived directly from the Alpha Architecture Reference Manual (SRM v6) and EV6 Hardware Reference Manual
- **Box-based execution domains** — IBox, EBox, FBox, MBox, CBox, and PalBox model the EV6 functional unit decomposition
- **Software TLB model (SPAM)** — full software implementation of the Alpha TLB with two-axis lazy invalidation, seqlock concurrency, and per-CPU epoch tracking
- **Template-driven portability** — PTE traits, replacement policies, and invalidation strategies are compile-time selectable via C++ template parameters (EV4/EV5/EV6)
- **Instruction grain architecture** — every Alpha opcode (integer, floating-point, memory, branch, PAL) implemented as a discrete grain class, auto-generated from GrainMaster.tsv
- **SMP support** — up to 4 emulated CPUs with per-CPU sharded data structures and lock-free hot paths
- **High performance** — lock-free seqlocks on TLB lookup, CAS-based slot allocation, GH coverage bitmap probe reduction, decode cache with PA-indexed global sharing

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                     EmulatorManager                          │
│           (INI config, SMP threading, lifecycle)             │
├──────────────┬───────────────┬────────────────┬──────────────┤
│  AlphaCPU 0  │  AlphaCPU 1   │  AlphaCPU 2    │  AlphaCPU 3  │
├──────────────┴───────────────┴────────────────┴──────────────┤
│                                                              │
│                  EV6 Execution Domain Boxes                   │
│                                                              │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────────┐ │
│  │  IBox  │ │  EBox  │ │  FBox  │ │  MBox  │ │    CBox    │ │
│  │ Fetch  │ │ IntExe │ │ FPExe  │ │ LdSt   │ │ Cache/Ctrl │ │
│  │ Decode │ │ Branch │ │ FPCR   │ │ WrBuf  │ │ BPred/L2   │ │
│  └────────┘ └────────┘ └────────┘ └────────┘ └────────────┘ │
│                                                              │
│  ┌──────────────────────────────────────────────────────────┐│
│  │                       PalBox                             ││
│  │   PAL dispatch · Shadow registers · HWPCB · HW_REI      ││
│  └──────────────────────────────────────────────────────────┘│
│                                                              │
├──────────────────────────────────────────────────────────────┤
│                Pipeline / Machine Library                     │
│   PipeLineSlot · MemoryExecutionStages · Stall/Flush/Retire │
├──────────────────────────────────────────────────────────────┤
│                Grain Factory (Instruction Grains)             │
│   Integer (96) · FP (280+) · Memory · Branch · PAL          │
│   DecodedInstruction · GrainRegistry · Decode Cache          │
├──────────────────────────────────────────────────────────────┤
│                SPAM TLB / PTE Subsystem                      │
│   4-D Shard Array [cpu][realm][GH][bucket]                   │
│   Two-axis lazy invalidation (global + per-ASN epochs)       │
│   Seqlock readers · CAS bitmap · Ev6SiliconTLB              │
├──────────────────────────────────────────────────────────────┤
│                Memory Subsystem                              │
│   GuestMemory · SafeMemory · SparseMemoryBacking            │
│   WriteBufferManager · MemoryBarrierCoordinator              │
│   SrmRomLoader · FirmwareDeviceManager                       │
├──────────────────────────────────────────────────────────────┤
│                Exception / Fault Subsystem                   │
│   ExceptionFactory · FaultDispatcher · PendingEvent          │
│   Arithmetic/Alignment/Translation fault inlines             │
├──────────────────────────────────────────────────────────────┤
│                MMIO / Device Subsystem                       │
│   MMIO Manager · Device Catalog · DMA Coherency              │
│   UART/SIO (OPA0:) · SRM Console · SRM Environment Store    │
├──────────────────────────────────────────────────────────────┤
│                SCSI / PCI / Controllers                      │
│   PCI Bus · QLogic ISP1040B · KZPBA · ALi 1553C IDE         │
│   SCSI Bus/Command/CDB · VirtualDisk · VirtualTape · ISO    │
│   DE500 NIC · IRQ Controller · Interval Timer                │
├──────────────────────────────────────────────────────────────┤
│                Firmware                                       │
│   SRM ROMs (ES40/ES45/DS10/DS20/GS320) · Clipper.bin        │
└──────────────────────────────────────────────────────────────┘
```

## Project Status

| Subsystem | Status | Notes |
|-----------|--------|-------|
| Integer instructions | ✅ Complete | All Alpha integer opcodes (96 grains) |
| Floating-point instructions | ✅ Complete | F/G/S/T/D_FLOAT, IEEE rounding, traps (280+ grains) |
| Memory instructions | ✅ Complete | LD/ST, LL/SC, unaligned |
| Branch instructions | ✅ Complete | Conditional, JSR/RET, barriers |
| PAL instructions | ✅ Complete | CALL_PAL, MFPR/MTPR, queue ops |
| Grain factory / decode cache | ✅ Complete | Auto-generated from GrainMaster.tsv |
| SPAM TLB subsystem | ✅ Complete | 4-D shards, lazy invalidation, SRRIP |
| IPR subsystem | ✅ Complete | EV4/EV5/EV6 orchestrated |
| Box architecture | 🔧 In progress | IBox/EBox/FBox/MBox/CBox/PalBox bases |
| Pipeline staging | 🔧 In progress | PipeLineSlot, stall/flush mechanics |
| PALcode / CALL_PAL | 🔧 In progress | EV6 PAL dispatch, shadow registers, HWPCB |
| Exception / fault subsystem | 🔧 In progress | ExceptionFactory, FaultDispatcher |
| Memory subsystem | 🔧 In progress | GuestMemory, SafeMemory, write buffers |
| MMIO subsystem | 🔧 In progress | Manager, device catalog, DMA coherency |
| SRM console | 🔧 In progress | Console device, environment store |
| SCSI / PCI subsystem | 🔧 In progress | ISP1040B, KZPBA, PCI bus, virtual disks |
| SRM firmware loading | 🔧 In progress | ROM loader, multi-platform SRM images |
| OS boot (Tru64) | 📋 Planned | Target milestone |

## Building

### Prerequisites

- **Visual Studio 2022** (v17.x) with C++17 support
- **Qt 6.x** (Core, Network modules)
- **CMake 3.21+**

### Build Steps

```batch
cd EmulatRAppUni
cmake --preset x64-debug
cmake --build out/build/debug
```

Or open the folder directly in Visual Studio 2022 (CMake integration).

## Project Structure

```
EmulatRAppUni/
├── main.cpp                    Entry point
├── CMakeLists.txt              Build configuration
├── CMakePresets.json           Build presets (x64-debug, x64-release)
│
├── cpuCoreLib/                 AlphaCPU, pipeline integration, reservation manager
├── coreLib/                    Core types, ALU/FP helpers, IPR storage, ExecTrace
│
│   ── Box Libraries (EV6 Execution Domains) ──
├── IBoxLib/                    Instruction Box (fetch, decode, branch predict)
├── EBoxLib/                    Execution Box (integer ALU)
├── FBoxLib/                    Floating-Point Box (FP execute, FPCR)
├── MBoxLib_EV6/                Memory Box (load/store, write buffer)
├── CBoxLib/                    Cache/Control Box (L2, branch predictor, CSR)
├── PalBoxLib/                  PAL Box (privileged execution domain)
│
│   ── Pipeline & Instruction System ──
├── machineLib/                 PipeLineSlot, MemoryExecutionStages
├── grainFactoryLib/            Instruction grain framework and code generation
│   ├── GrainMaster.tsv         Master opcode table (source of truth)
│   ├── DecodedInstruction.h    Decoded instruction representation
│   ├── InstructionGrain.h      Base grain class
│   └── grains/generated/       Auto-generated grain headers
│       ├── Integer/            96 integer operation grains
│       ├── FloatingPoint/      280+ FP grains (all variants)
│       ├── Memory/             Load/store/fetch grains
│       ├── Branch/             Branch, barrier, RPCC grains
│       └── PAL/                PAL instruction grains
│
│   ── TLB / PTE / Address Translation ──
├── pteLib/                     SPAM TLB, PTE, Ev6SiliconTLB, epoch tables
├── mmuLib/                     MMU interface layer
│
│   ── Memory System ──
├── memoryLib/                  GuestMemory, SafeMemory, write buffers, SRM ROM loader
│
│   ── Exception / Fault Handling ──
├── exceptionLib/               ExceptionFactory, exception classification
├── faultLib/                   FaultDispatcher, fault inlines, pending events
│
│   ── PAL / Firmware ──
├── palLib_EV6/                 PAL vector table, CSERVE, PAL services
├── firmware/                   SRM ROM images (ES40, ES45, DS10, DS20, GS320)
├── romLib/                     Compiled ROM data (.inc files)
│
│   ── Devices & I/O ──
├── deviceLib/                  SRM console, console manager, environment store
├── mmioLib/                    MMIO manager, device catalog, DMA coherency
├── controllersLib/             PCI bus, ISP1040B/KZPBA SCSI, ALi IDE, DE500 NIC
├── scsiCoreLib/                SCSI CDB, command queue, virtual disk/tape/ISO
│
│   ── Configuration & Management ──
├── configLib/                  ASA-EmulatR.ini, global settings
├── emulatrLib/                 Emulator init, execution coordinator, IPI manager
│
│   ── Tooling ──
├── Python/                     Build/analysis scripts (grain generation, audits)
│
│   ── Documentation ──
├── docs/                       HTML documentation (Help & Manual output)
└── manual/                     Help & Manual source project
    └── ASA-EmulatR/            H&M project file (.hmxz)
```

## Configuration

The emulator is configured via `ASA-EmulatR.ini` using standard INI sections with flattened dot-notation for device properties. Excerpts from the configuration:

```ini
; ── System ──────────────────────────────────────────────────
[System]
MemorySizeGB=32
hw-Model=ES40
hw-Serial-Number=AX122312341243134
Coherency-Cache=2048
Platform-Ev=6
PTE-PageSize=8192
ThreadCount=4
CpuCount=1
system_type_q8=2
CPU_FREQUENCY_HZ=500000000

; ── Physical Address Space Layout ───────────────────────────
[MemoryMap]
HwrpbBase=0x2000
HwrpbSize=0x4000
SrmBase=0x0
SrmSize=0x200000
SrmInitialPC=0x8001
RamBase=0x80000000
MmioBase=0xF0000000
MmioSize=0x10000000
PciMemBase=auto
PciMemSize=0x100000000

; ── SRM Firmware ────────────────────────────────────────────
[ROM]
SrmRomVariant=ES45

; ── Cache Hierarchy ─────────────────────────────────────────
[CACHE/l1]
NumSets=256
Associativity=2
LineSize=64
CoherencyProtocol=MESI
ReplacementPolicy=MRU

[CACHE/L2]
NumSets=512
Associativity=4
LineSize=64
TotalSize=131072
CoherencyProtocol=MESI

[CACHE/L3]
NumSets=1024
Associativity=8
LineSize=64
TotalSize=524288
CoherencyProtocol=MOESI

; ── Floating-Point SSE Acceleration ─────────────────────────
[FloatingPoint]
UseSSEForF_Float=0
UseSSEForG_Float=0
UseSSEForD_Float=0
UseSSEForS_Float=1
UseSSEForT_Float=1

; ── Console (OPA0:) ────────────────────────────────────────
[Device.OPA0]
classType=UART
location=cab0/drw0
iface=Net
iface_port=5555
application=putty -raw localhost 5555
rx_buffer_size=256
tx_buffer_size=1024

; ── SCSI Host Adapter ──────────────────────────────────────
[Device.PKB0]
classType=SCSI_HBA
controller_type=KZPBA
location=cab0/drw0/io0/hose0/bus3/slot1
pci_bus=3
pci_slot=1
scsi_id=7
mmio.bar0=auto
irqIpl=20
irqTrigger=LEVEL
irqPolicy=ROUND_ROBIN

; ── SCSI Disk (flattened dot-notation) ─────────────────────
[Device.DKA0]
schema=MDisk-1
parent=PKA0
scsi_id=0
classType=SCSI_DISK
container.deviceType=VMDK
container.path=rdsk0.img
container.readonly=false
geometry.logical_sector=512
geometry.physical_sector=512
identity.vendor=NVY
identity.product=VirtualDisk
identity.revision=0001
identity.serial=NVY00000001
cache.writeback=true
cache.flush_on_sync=true
binding.created_utc=2025-10-25T19:16:08Z

; ── IDE Controller ─────────────────────────────────────────
[Device.PQA0]
classType=IDE_CONTROLLER
description=ALi 1553C Integrated IDE Controller
location=cab0/drw0/io0/hose0/bus15/slot0
pci_bus=15
pci_slot=0

; ── Network ────────────────────────────────────────────────
[Device.EWA0]
classType=NIC
description=DE500 Fast Ethernet
location=cab0/drw0/io0/hose0/bus1/slot1
mac_address=00-68-EB-AA-9E-63
irqIpl=18

; ── SCSI Tape ──────────────────────────────────────────────
[Device.MKA600]
classType=SCSI_TAPE
parent=PKA0
scsi_id=6
format=VTAPE_BE8
readonly=true
uncompressed=true

; ── Execution Trace ────────────────────────────────────────
[ExecTrace]
ExecTraceEnabled=true
ExecTraceMode=continuous
TraceFormat=asm
PerCpuTraceFiles=true
TraceFilePattern=traces/es40_instance.cpu{cpu}.trace
TraceRingRecordsPerCpu=10000
CpuMask=0x1
TriggerOnException=true
TriggerOnIpi=true
IncludeIntRegWrites=true
IncludeIprWrites=true
IncludeMemVA=true
IncludeOpcodeWord=true
```

## Documentation

Full project documentation is authored in Help & Manual and published to GitHub Pages:

**📖 [ASA-EMulatR Documentation](https://timothypeer.github.io/EmulatRAppUni/index.html?introduction.html/)**

The documentation covers 22 chapters and 13+ appendices including:

- Alpha AXP instruction set (integer, floating-point, PAL)
- Execution model and pipeline architecture
- Box-based execution domains (IBox, EBox, FBox, MBox, CBox, PalBox)
- Memory system architecture and serialization
- SPAM TLB/PTE management (Appendix M)
- Instruction grain decode mechanics (Appendix G)
- EV6 Internal Processor Registers (Appendix A)
- Branch prediction, pipeline retirement, cycle walkthrough

## License

This project is licensed under the GNU General Public License v3.0 — see [LICENSE.md](LICENSE.md) for details.

Commercial use prohibited without separate license.
Contact: peert@envysys.com

## Author

**Timothy Peer** — Project Architect  
eNVy Systems, Inc.

- 📧 peert@envysys.com
- 🌐 [envysys.com](https://envysys.com)
- 📖 [Project Documentation](https://timothypeer.github.io/EmulatRAppUni/index.html?introduction.html)
