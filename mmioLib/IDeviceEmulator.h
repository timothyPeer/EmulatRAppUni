// ============================================================================
// IDeviceEmulator.h - Abstract interface for all device emulators.
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

#pragma once
// ============================================================================
//  ASA Emulator (c) 2025 Timothy Peer / eNVy Systems, Inc.
//  Non-Commercial Use Only.
// ============================================================================
//
// IDeviceEmulator.h
// Purpose: Abstract interface for all device emulators (HBA, NIC, UART, etc.)
//
// Contract:
//  - Devices implement this interface to receive MMIO and lifecycle events
//  - MMIOManager routes MMIO reads/writes to onRead/onWrite handlers
//  - DeviceInitializer calls initialize() during Phase 3
//  - Devices are stateless until initialize() is called
//
// Threading model:
//  - onRead/onWrite may be called from multiple vCPU threads concurrently
//  - Devices must provide their own internal locking if needed
//  - initialize/onReset called from single init thread

#pragma once

#include <QtGlobal>
#include <QString>
#include "../coreLib/mmio_core.h"
#include "../palLib_EV6/PAL_core.h"

// ============================================================================
// DEVICE EMULATOR INTERFACE
// ============================================================================

/**
 * @brief Abstract interface for all device emulators.
 *
 * Lifecycle:
 *   1. Construction (device-specific parameters)
 *   2. initialize(hoseId, basePA, irqVector, ipl) - Phase 3
 *   3. MMIO operations (onRead/onWrite)
 *   4. Optional reset (onReset)
 *   5. Destruction
 *
 * Thread-safety:
 *   - onRead/onWrite: may be called concurrently from vCPU threads
 *   - initialize/onReset: called from single init/reset thread
 *   - Devices must provide internal locking if state is shared
 *
 * Example implementations:
 *   - QLogicISP1020 (SCSI HBA)
 *   - DECTulip21143 (NIC)
 *   - NS16550UART (serial console)
 */

 //Forward Declarations
struct DeviceNode;
class MMIOManager;
class IRQController; // TODO Remove
class DMACoherencyManager;
class GuestMemory;


class IDeviceEmulator {
public:
    virtual ~IDeviceEmulator() = default;

    // ========================================================================
    // PHASE 3 INITIALIZATION
    // ========================================================================

    /**
     * @brief Initialize device with allocated resources.
     *
     * Called by DeviceInitializer after Phase 2 resource allocation.
     * Device should:
     *   - Store hoseId, basePA, irqVector, ipl
     *   - Reset internal registers to power-on defaults
     *   - Register MMIO windows with MMIOManager (done by caller)
     *   - Register IRQ vectors with IRQController (done by caller)
     *
     * @param hoseId PCI hose ID (interrupt domain)
     * @param basePA Physical address of BAR0 (primary MMIO window)
     * @param irqVector Allocated IRQ vector number
     * @param ipl Interrupt Priority Level (0-31)
     *
     * Thread-safety: called from init thread (not concurrent)
     */
    virtual bool initialize(quint16 hoseId,
        quint64 basePA,
        quint32 irqVector,
        quint8 ipl) = 0;

    // ========================================================================
    // MMIO HANDLERS (Called by MMIOManager)
    // ========================================================================

    /**
     * @brief Handle MMIO read.
     *
     * MMIOManager calls this after:
     *   - Validating PA is in device's window
     *   - Validating access width is allowed
     *   - Converting offset to BAR-relative
     *
     * Device should:
     *   - Read from internal register at offset
     *   - Write result to dst buffer (host-endian)
     *   - Return immediately (no blocking I/O)
     *
     * @param barIndex BAR number (0-5 for PCI devices)
     * @param offset Byte offset within BAR
     * @param dst Destination buffer (host-endian, size = width)
     * @param width Access width in bytes (1, 2, 4, or 8)
     *
     * Endianness:
     *   - Device handler receives/returns host-endian values
     *   - MMIOManager handles conversion based on regEndian field
     *
     * Side-effects:
     *   - If sideEffectOnRead=true, may trigger state changes
     *   - Example: clear-on-read ISR, FIFO pop
     *
     * Thread-safety:
     *   - May be called concurrently from multiple vCPU threads
     *   - Device must provide internal locking if needed
     *
     * Example (QLogic ISR read):
     *   case 0x20: // ISR register
     *     memcpy(dst, &m_isr, width);
     *     m_isr = 0; // Clear on read
     *     irqController->clearInterrupt(m_hoseId, m_irqVector);
     *     break;
     */
    virtual MMIOStatus onRead(quint64 offset, quint8 width, quint64& outValue) = 0;

    /**
     * @brief Handle MMIO write.
     *
     * MMIOManager calls this after:
     *   - Validating PA is in device's window
     *   - Validating access width is allowed
     *   - Converting offset to BAR-relative
     *   - Converting value to host-endian
     *
     * Device should:
     *   - Write to internal register at offset
     *   - Trigger side-effects if sideEffectOnWrite=true
     *   - Return immediately (defer long operations to worker thread)
     *
     * @param barIndex BAR number (0-5 for PCI devices)
     * @param offset Byte offset within BAR
     * @param src Source buffer (host-endian, size = width)
     * @param width Access width in bytes (1, 2, 4, or 8)
     *
     * Endianness:
     *   - Device handler receives host-endian values
     *   - MMIOManager already converted from guest endianness
     *
     * Side-effects:
     *   - Doorbell writes: trigger DMA/command processing
     *   - Control register writes: start/stop device
     *   - IMR writes: update interrupt mask
     *
     * Posted writes:
     *   - If device needs ordering, call mmioManager->drainPostedWrites(uid)
     *     in doorbell handler before reading descriptors
     *
     * Thread-safety:
     *   - May be called concurrently from multiple vCPU threads
     *   - Device must provide internal locking if needed
     *
     * Example (QLogic doorbell):
     *   case 0x10: // DOORBELL register
     *     quint32 descriptorPA;
     *     memcpy(&descriptorPA, src, sizeof(descriptorPA));
     *     m_mmioManager->drainPostedWrites(m_uid);  // Ensure CPU writes visible
     *     QMetaObject::invokeMethod(this, "processDoorbell",
     *                              Qt::QueuedConnection,
     *                              Q_ARG(quint32, descriptorPA));
     *     break;
     */
    virtual MMIOStatus onWrite(quint64 offset, quint8 width, quint64 value) = 0;

    // ========================================================================
    // LIFECYCLE EVENTS
    // ========================================================================

    /**
     * @brief Reset device to power-on state.
     *
     * Called when:
     *   - System reset
     *   - Device-specific reset register write
     *   - Hot-plug remove/reinsert (future)
     *
     * Device should:
     *   - Reset all registers to defaults
     *   - Clear pending interrupts
     *   - Abort in-flight DMA
     *   - Reinitialize internal state
     *
     * Thread-safety: called from init/reset thread (not concurrent)
     */
    virtual void onReset() = 0;

    /**
     * @brief Handle global MMIO fence (MB/RMB/WMB instructions).
     *
     * Called when guest CPU executes memory barrier instruction.
     * Device may need to:
     *   - Drain posted write buffers
     *   - Complete pending DMA
     *   - Synchronize with I/O thread
     *
     * Optional: default implementation does nothing.
     *
     * @param kind Fence type (MB, RMB, WMB)
     *
     * Thread-safety: called from vCPU thread that executed fence
     */
    virtual void onFence(palCore_FenceKind kind) {
        // Default: no-op (most devices don't need this)
        (void)kind;
    }

    // ========================================================================
    // METADATA (for diagnostics and routing)
    // ========================================================================

    /**
     * @brief Get device unique identifier.
     *
     * @return UID from DeviceNode (set during topology discovery)
     *
     * Used by:
     *   - MMIOManager for routing
     *   - DMACoherencyManager for diagnostics
     *   - Logging/tracing
     */
    virtual quint32 deviceUid() const = 0;

    /**
     * @brief Get human-readable device name.
     *
     * @return Device name (e.g., "PKA0", "EWA0")
     *
     * Used for logging and UI display.
     */
    virtual QString deviceName() const = 0;
};



