// ============================================================================
// global_memoryInterface.h
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global memory interface for Alpha boxes
//
// Primary memory interface for:
//   - MBox: Load/store execution
//   - IBox: Instruction fetch (via grains)
//   - ABox: PTE fetch
//
// Handles:
//   - Physical address reads/writes
//   - MMIO routing
//   - Alignment validation
//   - Width checking
// ============================================================================

#ifndef GLOBAL_MEMORYINTERFACE_H
#define GLOBAL_MEMORYINTERFACE_H

#include <QtGlobal>
#include "memory_core.h"

// Forward declaration
class GuestMemory;

/**
 * @brief Global memory accessor for Alpha boxes
 * @return Reference to singleton GuestMemory instance
 */
GuestMemory& globalBoxMemory() noexcept;

// ============================================================================
// Convenience Functions for Common Operations
// ============================================================================

/**
 * @brief Read quadword (64-bit) from physical address
 * @param pa Physical address
 * @param outValue Output value
 * @return Memory operation status
 */
MEM_STATUS readPA_Quad(quint64 pa, quint64& outValue) noexcept;

/**
 * @brief Write quadword (64-bit) to physical address
 * @param pa Physical address
 * @param value Value to write
 * @return Memory operation status
 */
MEM_STATUS writePA_Quad(quint64 pa, quint64 value) noexcept;

/**
 * @brief Read longword (32-bit) from physical address
 * @param pa Physical address
 * @param outValue Output value
 * @return Memory operation status
 */
MEM_STATUS readPA_Long(quint64 pa, quint32& outValue) noexcept;

/**
 * @brief Write longword (32-bit) to physical address
 * @param pa Physical address
 * @param value Value to write
 * @return Memory operation status
 */
MEM_STATUS writePA_Long(quint64 pa, quint32 value) noexcept;

/**
 * @brief Read word (16-bit) from physical address
 * @param pa Physical address
 * @param outValue Output value
 * @return Memory operation status
 */
MEM_STATUS readPA_Word(quint64 pa, quint16& outValue) noexcept;

/**
 * @brief Write word (16-bit) to physical address
 * @param pa Physical address
 * @param value Value to write
 * @return Memory operation status
 */
MEM_STATUS writePA_Word(quint64 pa, quint16 value) noexcept;

/**
 * @brief Read byte from physical address
 * @param pa Physical address
 * @param outValue Output value
 * @return Memory operation status
 */
MEM_STATUS readPA_Byte(quint64 pa, quint8& outValue) noexcept;

/**
 * @brief Write byte to physical address
 * @param pa Physical address
 * @param value Value to write
 * @return Memory operation status
 */
MEM_STATUS writePA_Byte(quint64 pa, quint8 value) noexcept;

#endif // GLOBAL_MEMORYINTERFACE_H
