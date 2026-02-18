// ============================================================================
// IPI_core.h - Inter-Processor Interrupt Command Types
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// 
// Description:
//   Consolidated IPI command enumeration for SMP coordination.
//   Covers TLB shootdowns, cache coherency, and synchronization.
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
//
// References:
//   [1998] Alpha Architecture Reference Manual, Third Edition
//          Chapter 6: Common PAL-code Architecture
//          Section 6.2.2: TLB Fill and Invalidate Operations
// ============================================================================

#ifndef IPI_CORE_H
#define IPI_CORE_H

#include <QtGlobal>

// ============================================================================
// IPI Hardware Constants
// ============================================================================

/**
 * @brief IPI interrupt vector number
 *
 * Alpha architecture reserves 0x600 for inter-processor interrupts.
 * This vector is delivered when one CPU sends an IPI to another.
 *
 * Reference: Alpha Architecture Reference Manual, Chapter 6
 */
constexpr quint32 IPI_VECTOR = 0x600;
/**
 * @brief IPI interrupt priority level
 *
 * IPIs are delivered at IPL 20 (high priority, below machine check).
 * This ensures IPIs can interrupt most code but not critical sections.
 *
 * IPL levels:
 *   0-7:   Software/device interrupts
 *   20:    Inter-processor interrupts (IPI)
 *   31:    Machine check (highest)
 */
constexpr quint8 IPI_IPL = 20;

// ============================================================================
// IPI Command Types (Consolidated)
// ============================================================================

enum class IPICommand : quint8 {
    // ========================================================================
    // INVALID / NO-OP
    // ========================================================================
    INVALID = 0x00,                 // Invalid/no operation

    // ========================================================================
    // TLB INVALIDATION COMMANDS (0x01-0x0F)
    // ========================================================================
    // These map directly to Alpha MTPR instructions

    TLB_INVALIDATE_ALL = 0x01,      // TBIA: Invalidate all TLBs (ITB+DTB, all ASNs)
    TLB_INVALIDATE_ASN = 0x02,      // TBIAP: Invalidate by ASN (ITB+DTB for process)
    TLB_INVALIDATE_VA_BOTH = 0x03,  // TBIS: Invalidate single VA (ITB+DTB)
    TLB_INVALIDATE_VA_ITB = 0x04,   // TBISI: Invalidate single VA (ITB only)
    TLB_INVALIDATE_VA_DTB = 0x05,   // TBISD: Invalidate single VA (DTB only)

    // Future TLB operations (reserved)
    TLB_INVALIDATE_GLOBAL = 0x06,   // Reserved: Invalidate global entries only
    TLB_INVALIDATE_RANGE = 0x07,    // Reserved: Invalidate VA range

    // ========================================================================
    // CACHE COHERENCY COMMANDS (0x10-0x1F)
    // ========================================================================

    CACHE_INVALIDATE_LINE = 0x10,   // Invalidate cache line at PA
    CACHE_FLUSH_LINE = 0x11,        // Flush (write-back + invalidate) cache line
    CACHE_EVICT_LINE = 0x12,        // Evict cache line (ECB instruction)
    CACHE_INVALIDATE_ALL = 0x13,    // Invalidate all caches

    // ========================================================================
    // MEMORY BARRIER COMMANDS (0x20-0x2F)
    // ========================================================================

    MEMORY_BARRIER_FULL = 0x20,     // MB: Full memory barrier
    MEMORY_BARRIER_WRITE = 0x21,    // WMB: Write memory barrier
    MEMORY_BARRIER_READ = 0x22,     // Reserved: Read barrier (if needed)

    // ========================================================================
    // SYNCHRONIZATION COMMANDS (0x30-0x3F)
    // ========================================================================

    SYNC_REQUEST = 0x30,            // Request synchronization point
    SYNC_ACKNOWLEDGE = 0x31,        // Acknowledge synchronization

    // ========================================================================
    // SYSTEM CONTROL COMMANDS (0x40-0x4F)
    // ========================================================================

    HALT_CPU = 0x40,                // Halt target CPU
    WAKE_CPU = 0x41,                // Wake target CPU from halt
    CONTEXT_SWITCH = 0x42,          // Remote context switch request

    // ========================================================================
    // CUSTOM/EXTENSIBLE (0xF0-0xFF)
    // ========================================================================

    CUSTOM_BASE = 0xF0,             // Base for custom commands
    CUSTOM = 0xFF                   // Generic custom IPI
};

// ============================================================================
// LEGACY COMPATIBILITY (Deprecated - use IPICommand directly)
// ============================================================================



// ============================================================================
// IPI DATA ENCODING HELPERS
// ============================================================================

constexpr inline quint64 encodeIPIData(IPICommand cmd, quint8 param = 0) noexcept
{
    return (static_cast<quint64>(param) << 8) | static_cast<quint64>(cmd);
}

constexpr inline quint64 encodeIPIDataLarge(IPICommand cmd, quint64 param) noexcept
{
    return ((param & 0x00FF'FFFF'FFFF'FFFFULL) << 8) | static_cast<quint64>(cmd);
}

constexpr inline IPICommand decodeIPICommand(quint64 data) noexcept
{
    return static_cast<IPICommand>(data & 0xFF);
}

constexpr inline quint8 decodeIPIParam8(quint64 data) noexcept
{
    return static_cast<quint8>((data >> 8) & 0xFF);
}

constexpr inline quint64 decodeIPIParam56(quint64 data) noexcept
{
    return (data >> 8) & 0x00FF'FFFF'FFFF'FFFFULL;
}

/**
 * @brief Encode IPI with virtual address for TLB shootdown
 *
 * Layout:
 * Bits 63-56: Command (IPICommand)
 * Bits 55-48: Flags/Subtype
 * Bits 47-0:  Virtual Address (48 bits - full Alpha VA space)
 */
inline quint64 encodeIPIWithVA(IPICommand cmd, quint64 va) noexcept
{
    quint64 data = static_cast<quint64>(cmd) << 56;  // Command in top 8 bits
    data |= (va & 0x0000FFFFFFFFFFFF);               // VA in lower 48 bits
    return data;
}

/**
 * @brief Decode virtual address from IPI data
 */
inline quint64 decodeIPIVA(quint64 ipiData) noexcept
{
    return ipiData & 0x0000FFFFFFFFFFFF;  // Extract lower 48 bits
}

/**
 * @brief Encode IPI with ASN for TLB shootdown by ASN
 *
 * Layout:
 * Bits 63-56: Command (IPICommand)
 * Bits 55-48: Flags
 * Bits 7-0:   ASN (8 bits)
 */
inline quint64 encodeIPIWithASN(IPICommand cmd, ASNType asn) noexcept
{
    quint64 data = static_cast<quint64>(cmd) << 56;
    data |= (asn & 0xFF);  // ASN in lowest 8 bits
    return data;
}

/**
 * @brief Decode ASN from IPI data
 */
inline ASNType decodeIPIASN(quint64 ipiData) noexcept
{
    return static_cast<ASNType>(ipiData & 0xFF);
}

// ============================================================================
// IPI COMMAND HELPERS
// ============================================================================

constexpr inline bool ipiCommandNeedsParam(IPICommand cmd) noexcept
{
    switch (cmd) {
    case IPICommand::TLB_INVALIDATE_ASN:
    case IPICommand::TLB_INVALIDATE_VA_BOTH:
    case IPICommand::TLB_INVALIDATE_VA_ITB:
    case IPICommand::TLB_INVALIDATE_VA_DTB:
    case IPICommand::CACHE_INVALIDATE_LINE:
    case IPICommand::CACHE_FLUSH_LINE:
    case IPICommand::CACHE_EVICT_LINE:
        return true;
    default:
        return false;
    }
}

constexpr inline const char* ipiCommandName(IPICommand cmd) noexcept
{
    switch (cmd) {
    case IPICommand::INVALID:                   return "INVALID";
    case IPICommand::TLB_INVALIDATE_ALL:        return "TLB_INVALIDATE_ALL";
    case IPICommand::TLB_INVALIDATE_ASN:        return "TLB_INVALIDATE_ASN";
    case IPICommand::TLB_INVALIDATE_VA_BOTH:    return "TLB_INVALIDATE_VA_BOTH";
    case IPICommand::TLB_INVALIDATE_VA_ITB:     return "TLB_INVALIDATE_VA_ITB";
    case IPICommand::TLB_INVALIDATE_VA_DTB:     return "TLB_INVALIDATE_VA_DTB";
    case IPICommand::CACHE_INVALIDATE_LINE:     return "CACHE_INVALIDATE_LINE";
    case IPICommand::CACHE_FLUSH_LINE:          return "CACHE_FLUSH_LINE";
    case IPICommand::CACHE_EVICT_LINE:          return "CACHE_EVICT_LINE";
    case IPICommand::CACHE_INVALIDATE_ALL:      return "CACHE_INVALIDATE_ALL";
    case IPICommand::MEMORY_BARRIER_FULL:       return "MEMORY_BARRIER_FULL";
    case IPICommand::MEMORY_BARRIER_WRITE:      return "MEMORY_BARRIER_WRITE";
    case IPICommand::HALT_CPU:                  return "HALT_CPU";
    case IPICommand::WAKE_CPU:                  return "WAKE_CPU";
    case IPICommand::CONTEXT_SWITCH:            return "CONTEXT_SWITCH";
    case IPICommand::CUSTOM:                    return "CUSTOM";
    default:                                    return "UNKNOWN";
    }
}


#endif // IPI_CORE_H