// ============================================================================
// GuestMemory.h - Physical Address (PA) Router
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
//  ASA Emulator (c) 2025 Timothy Peer / eNVy Systems, Inc.
// ============================================================================
//
//  GuestMemory.h - Physical Address (PA) Router
//
//  Purpose:
//    GuestMemory is the authoritative PA router and ONLY component that
//    knows physical address mappings.
//
//  Design:
//    - Single source of truth for PA -> subsystem routing
//    - Translates PA -> (Subsystem, Offset) for each access
//    - Subsystems are PA-agnostic (offset-based interfaces)
//    - No duplication (SafeMemory is single RAM truth)
//
//  Contract (Authoritative):
//    - GuestMemory classifies PA exactly once and routes to ONE subsystem
//    - Subsystems MUST NOT re-route or know their PA mapping
//    - All CPU/DMA/device accesses go through GuestMemory
//
//  PA Routing Table:
//    [0x0, ramBase+ramSize)      -> SafeMemory (all physical RAM, offset 0x0)
//    [mmioBase, mmioBase+mmioSize) -> MMIOManager (Typhoon PCI, PA direct)
//
//  SafeMemory Physical Address Layout:
//    PA 0x000000 - 0x1FFFFF       Decompressed SRM firmware (2 MB)
//       +-- 0x002000              HWRPB (built by SRM during boot)
//       +-- 0x008000              PAL entry (PC = 0x8001)
//       +-- 0x0xxxxx              PAL vectors, SRM console code
//    PA 0x600000                  PAL_BASE (dispatch table)
//    PA 0x900000                  Decompressor staging (temporary)
//    PA 0x80000000+               Main RAM (OS, applications)
//
//  Key Points:
//    - SRM firmware is decompressed into SafeMemory at PA 0x0 by SrmRomLoader
//    - No SRMFirmwareRegion -- firmware is just RAM after decompression
//    - HWRPB lives in SafeMemory at PA 0x2000
//    - PA = SafeMemory offset (no translation math)
//
// ============================================================================

#ifndef GUESTMEMORY_H
#define GUESTMEMORY_H

#include <QtGlobal>
#include <QString>
#include <QVector>
#include "coreLib/Axp_Attributes_core.h"
#include "memory_core.h"
#include "MemorySpan.h"

// Forward declarations
class SafeMemory;
class MMIOManager;

// ============================================================================
// CACHE OPERATION TYPES
// ============================================================================

enum class CacheOperation : quint8 {
    Prefetch_Read,
    Prefetch_Exclusive,
    Evict,
    Invalidate,
    Flush
};

// ============================================================================
// GUEST MEMORY - PA ROUTER
// ============================================================================

class GuestMemory final
{
public:
    // ========================================================================
    // ROUTE TARGET
    // ========================================================================

    enum class RouteTarget : quint8
    {
        None = 0,
        SafeMemory,     // RAM (single source of truth)
        MMIOManager     // Memory-mapped I/O devices
    };

    // ========================================================================
    // PA ROUTE ENTRY
    // ========================================================================

    /**
     * @brief PA routing table entry: [start, end) -> subsystem
     *
     * offsetBase is added to (pa - startPA) to get subsystem offset.
     *
     * Example:
     *   PA [0x0, 0x880000000) -> SafeMemory offset [0x0, 0x880000000)
     *     startPA = 0x0, offsetBase = 0x0
     *     offset = pa (identity mapping)
     */
    struct alignas(8) PARouteEntry
    {
        quint64     startPA{ 0 };
        quint64     endPA{ 0 };
        RouteTarget target{ RouteTarget::None };
        quint64     offsetBase{ 0 };

        AXP_HOT AXP_FLATTEN bool contains(quint64 pa) const noexcept
        {
            return (pa >= startPA) && (pa < endPA);
        }

        AXP_HOT AXP_FLATTEN bool containsRange(quint64 pa, quint64 len) const noexcept
        {
            if (len == 0) return false;
            const quint64 end = pa + len;
            if (end < pa) return false;
            return (pa >= startPA) && (end <= endPA);
        }

        bool overlaps(const PARouteEntry& other) const noexcept
        {
            return !(endPA <= other.startPA || startPA >= other.endPA);
        }

        AXP_HOT AXP_FLATTEN quint64 calculateOffset(quint64 pa) const noexcept
        {
            return (pa - startPA) + offsetBase;
        }
    };

    // ========================================================================
    // ACCESS KIND
    // ========================================================================

    enum class AccessKind : quint8
    {
        InstructionFetch,
        DataRead,
        DataWrite,
        DMARead,
        DMAWrite
    };

public:
    GuestMemory() = default;
    ~GuestMemory() = default;

    GuestMemory(const GuestMemory&) = delete;
    GuestMemory& operator=(const GuestMemory&) = delete;

    // ========================================================================
    // SUBSYSTEM ATTACHMENT
    // ========================================================================

    /**
     * @brief Attach subsystem backends
     *
     * Must be called before initDefaultPARoutes().
     *
     * @param sm SafeMemory instance (required)
     * @param mmio MMIOManager instance (required)
     */
    AXP_HOT AXP_FLATTEN void attachSubsystems(
        SafeMemory* sm,
        MMIOManager* mmio
    ) noexcept;

    // ========================================================================
    // ROUTING TABLE INITIALIZATION
    // ========================================================================

    /**
     * @brief Initialize PA routing table
     *
     * Creates routing table from EmulatorSettings:
     *   [0x0, ramBase+ramSize)         -> SafeMemory (all RAM, PA = offset)
     *   [mmioBase, mmioBase+mmioSize)  -> MMIOManager (Typhoon PCI space)
     *
     * Must be called after attachSubsystems().
     */
    AXP_HOT AXP_FLATTEN void initDefaultPARoutes() noexcept;

    /**
     * @brief Set custom routing table
     */
    AXP_HOT AXP_FLATTEN void setRoutes(const QVector<PARouteEntry>& routes) noexcept;

    // ========================================================================
    // PA CLASSIFICATION
    // ========================================================================

    AXP_HOT AXP_FLATTEN RouteTarget classifyPA(quint64 pa) const noexcept;
    AXP_HOT AXP_FLATTEN RouteTarget classifyPARange(quint64 pa, quint64 len) const noexcept;
    AXP_HOT AXP_FLATTEN bool isRAM(quint64 pa, quint64 len = 1) const noexcept;
    AXP_HOT AXP_FLATTEN bool isMMIO(quint64 pa, quint64 len = 1) const noexcept;
    AXP_HOT AXP_FLATTEN bool isValidPhysicalAddress(quint64 pa, quint64 len = 1) const noexcept;

    // ========================================================================
    // CORE ROUTED ACCESS
    // ========================================================================

    AXP_HOT AXP_FLATTEN MEM_STATUS readRouted(quint64 pa, quint8 width, quint64& outValue, AccessKind kind) const noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS writeRouted(quint64 pa, quint8 width, quint64 value, AccessKind kind) const noexcept;

    // ========================================================================
    // INSTRUCTION FETCH
    // ========================================================================

    AXP_HOT AXP_FLATTEN MEM_STATUS readInst32(quint64 pa, quint32& value) const noexcept;

    // ========================================================================
    // TYPED READS/WRITES
    // ========================================================================

    AXP_HOT AXP_FLATTEN MEM_STATUS read8(quint64 pa, quint8& value) const noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS read16(quint64 pa, quint16& value) const noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS read32(quint64 pa, quint32& value) const noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS read64(quint64 pa, quint64& value) const noexcept;

    AXP_HOT AXP_FLATTEN MEM_STATUS write8(quint64 pa, quint8 value) const noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS write16(quint64 pa, quint16 value) const noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS write32(quint64 pa, quint32 value) const noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS write64(quint64 pa, quint64 value) const noexcept;

    // ========================================================================
    // BLOCK ACCESS (RAM-ONLY BY DESIGN)
    // ========================================================================

    AXP_HOT AXP_FLATTEN MEM_STATUS readPA(quint64 pa, void* dst, qsizetype len) const noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS writePA(quint64 pa, const void* src, qsizetype len) const noexcept;

    // ========================================================================
    // SPAN ACCESS (PREFERRED FOR BUFFERS)
    // ========================================================================

    /**
     * @brief Get contiguous span to physical address
     *
     * Returns span up to page boundary or region end.
     * Used for safe buffer access (e.g., CSERVE PUTS/GETS).
     */
    MemorySpan getSpanToPA(quint64 pa, quint64 requestedLen, AccessIntent intent) const noexcept;

    // ========================================================================
    // CACHE MANAGEMENT (STUBS)
    // ========================================================================



    // ========================================================================
    // DMA COHERENCY
    // ========================================================================

    AXP_HOT AXP_FLATTEN void notifyDMAWriteComplete(quint64 pa, quint32 size) const noexcept;

    // ========================================================================
    // DIAGNOSTICS
    // ========================================================================

    AXP_HOT AXP_FLATTEN QString classifyPhysicalAddress(quint64 pa) const noexcept;

private:
    SafeMemory* m_safeMem{ nullptr };
    MMIOManager* m_mmio{ nullptr };
    QVector<PARouteEntry> m_routes;

    AXP_HOT AXP_FLATTEN const PARouteEntry* findRoute(quint64 pa) const noexcept;

};

#endif // GUESTMEMORY_H