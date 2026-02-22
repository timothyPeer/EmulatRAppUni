// ============================================================================
// MMIO_Attribute.h - MMIO region attributes.
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

#ifndef MMIO_ATTRIBUTE_H
#define MMIO_ATTRIBUTE_H
// ============================================================================
//  ASA Emulator (c) 2025 Timothy Peer / eNVy Systems, Inc.
//  Non-Commercial Use Only.
// ============================================================================
//
//  MMIO_attr_CORRECTED.h
//
//  Purpose: Corrected MMIO_attr structure (remove incomplete 'handlers')
//
//  Replace your existing MMIO_attr in mmio_Manager.h with this:
//
// ============================================================================


#include <QtGlobal>
#include "../coreLib/mmio_core.h"

/**
 * @brief MMIO region attributes.
 *
 * Describes behavioral properties of an MMIO region.
 * Handlers are stored separately in RegionEntry.
 */
struct MMIO_attr {
    /**
     * @brief Allow posted writes (fire-and-forget).
     *
     * If true, writes may return before reaching device.
     * Memory barriers must drain these writes.
     */
    bool allowPostedWrites{false};

    /**
     * @brief Strongly ordered region.
     *
     * If true, all accesses are serialized (no reordering).
     * Each access completes before next begins.
     */
    bool stronglyOrdered{false};

    /**
     * @brief Read has side effects.
     *
     * If true, reads may change device state (e.g., status register clear).
     * Prevents speculative/redundant reads.
     */
    bool sideEffectOnRead{false};

    /**
     * @brief Write has side effects.
     *
     * If true, writes may trigger device actions (e.g., DMA start).
     * Prevents write coalescing/elimination.
     */
    bool sideEffectOnWrite{true};  // Usually true for MMIO

    /**
     * @brief Requires fence after write.
     *
     * If true, CPU must issue memory barrier after writing.
     * Used for doorbell/command registers.
     */
    bool needsDoorbellFence{false};

    /**
     * @brief Minimum alignment requirement (bytes).
     *
     * All accesses must be aligned to this boundary.
     * Common values: 1 (byte), 2 (word), 4 (dword), 8 (qword)
     */
    quint8 minAlignment{1};

    /**
     * @brief Supported access widths (bitmask).
     *
     * Bit 0 = 1 byte  (0x01)
     * Bit 1 = 2 bytes (0x02)
     * Bit 2 = 4 bytes (0x04)
     * Bit 3 = 8 bytes (0x08)
     *
     * Example: 0x0F = all widths supported
     *          0x0C = only 4 and 8 bytes
     */
    quint8 supportedWidths{0x0F};  // Default: all widths

    /**
     * @brief Register endianness.
     *
     * Specifies byte order of device registers.
     * MMIOManager converts between host and device endianness.
     */
    mmio_Endianness regEndian{mmio_Endianness::LITTLE};  // Default: little-endian
};


#endif // MMIO_ATTRIBUTE_H
