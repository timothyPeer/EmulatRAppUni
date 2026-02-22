// ============================================================================
// ScsiControllerLib.h - ============================================================================
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

// ============================================================================
// ScsiControllerLib.H  -  Umbrella Header for controllerLib
// ============================================================================
// This header aggregates the public controllerLib interfaces built on top of
// scsiCoreLib. It provides the logical SCSI control-plane architecture:
//
//   - ScsiController      - maps (targetId, LUN) to VirtualScsiDevice*
//   - ScsiBus             - logical SCSI bus fa�ade around ScsiController
//   - ScsiInitiatorPort   - host-side identity and statistics
//   - ScsiTargetPort      - target-side port identity and LUN binding
//   - ScsiTransaction     - neutral transaction descriptor around ScsiCommand
//   - ScsiScheduler       - FIFO SCSI transaction scheduler
//   - ScsiHostAdapter     - abstract host adapter with sync/async hooks
//   - ScsiHostAdapterBackend - abstract bridge to emulator / platform glue
//   - GenericScsiHostAdapter - QThread-based async host adapter implementation
//   - ScsiHostAdapterEvents - basic event structures and enums
//
// Design constraints for controllerLib as a whole:
//   - Header-only, no .CPP required.
//   - Depends on QtCore and scsiCoreLib.
//   - Does NOT depend on coreLib, AlphaCPU, PAL, MMIO, PTE, or SafeMemory.
//   - Pure ASCII, UTF-8 (no BOM).
//
// References (conceptual):
//   - SAM-2: SCSI Architecture Model (Initiator Port, Target Port, LU).
//   - SPC-3: Command and status model.
//   - SBC-3 / SSC-3 / MMC-5: device-type specific behaviors in scsiCoreLib.
//
// Usage:
//   Include this header from any PCI or MMIO controller layer that needs
//   a complete SCSI control-plane stack:
//
//       #include "ScsiControllerLib.H"
//
// ============================================================================

#ifndef SCSI_CONTROLLER_LIB_H
#define SCSI_CONTROLLER_LIB_H

// Bring in scsiCoreLib (SCSI types, devices, backends, cache).
#include "ScsiCoreLib.H"

// Logical controller layer on top of scsiCoreLib.
#include "ScsiController.H"
#include "ScsiBus.H"
#include "ScsiInitiatorPort.h"
#include "ScsiTargetPort.H"
#include "ScsiTransaction.H"
#include "ScsiScheduler.H"
#include "ScsiHostAdapter.H"
#include "ScsiHostAdapterBackend.H"
#include "GenericScsiHostAdapter.H"
#include "ScsiHostAdapterEvents.H"

#endif // SCSI_CONTROLLER_LIB_H

