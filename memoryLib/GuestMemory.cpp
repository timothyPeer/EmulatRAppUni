// ============================================================================
// GuestMemory.cpp - PA Router Implementation
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
//  GuestMemory.cpp - PA Router Implementation
//
//  Two-route architecture:
//    Route 1: [0x0, ramBase+ramSize)         -> SafeMemory (PA = offset)
//    Route 2: [mmioBase, mmioBase+mmioSize)  -> MMIOManager (PA direct)
//
//  SRM firmware is decompressed into SafeMemory at PA 0x0 by SrmRomLoader.
//  No SRMFirmwareRegion -- firmware is just bytes in RAM.
//
// ============================================================================

#include "GuestMemory.h"
#include "SafeMemory.h"
#include "../mmioLib/mmio_Manager.h"
#include "../coreLib/LoggingMacros.h"
#include <algorithm>

#include "configLib/global_EmulatorSettings.h"

#define COMPONENT_NAME "GuestMemory"

// ============================================================================
// SUBSYSTEM ATTACHMENT
// ============================================================================

AXP_HOT AXP_FLATTEN void GuestMemory::attachSubsystems(
    SafeMemory* sm,
    MMIOManager* mmio) noexcept
{
    m_safeMem = sm;
    m_mmio = mmio;

    DEBUG_LOG("GuestMemory: Subsystems attached");
    DEBUG_LOG(QString("  SafeMemory:  %1").arg(sm ? "YES" : "NO"));
    DEBUG_LOG(QString("  MMIOManager: %1").arg(mmio ? "YES" : "NO"));
}

// ============================================================================
// ROUTING TABLE INITIALIZATION
// ============================================================================

AXP_HOT AXP_FLATTEN void GuestMemory::initDefaultPARoutes() noexcept
{
    m_routes.clear();
    m_routes.reserve(2);

    // Get memory map from settings
    auto& settings = global_EmulatorSettings();
    const quint64 ramBase = settings.podData.memoryMap.ramBase;
    const quint64 ramSize = EmulatorSettingsInline::readMemorySize(
        "memoryMap/ramSize", 0x800000000ULL);   // 32 GB default
    const quint64 mmioBase = EmulatorSettingsInline::readMemorySize(
        "memoryMap/mmioBase", 0x1000000000ULL);
    const quint64 mmioSize = EmulatorSettingsInline::readMemorySize(
        "memoryMap/mmioSize", 0x1000000000ULL); // 64 GB default

    INFO_LOG("=== GuestMemory: Initializing PA Routing Table ===");

    // ========================================================================
    // Route 1: SafeMemory -- All physical RAM
    //
    // Covers the entire physical address space below MMIO:
    //   PA 0x000000 - 0x1FFFFF   SRM firmware (decompressed by SrmRomLoader)
    //   PA 0x600000              PAL_BASE dispatch table
    //   PA 0x900000              Decompressor staging area (temporary)
    //   PA 0x80000000+           Main RAM
    //
    // Identity mapping: PA = SafeMemory offset (offsetBase = 0)
    // ========================================================================
    m_routes.append({
        .startPA = 0x0,
        .endPA = ramBase + ramSize,
        .target = RouteTarget::SafeMemory,
        .offsetBase = 0x0
        });

    INFO_LOG(QString("  [0x%1 - 0x%2) -> SafeMemory (%3 GB, PA = offset)")
        .arg(quint64(0), 16, 16, QChar('0'))
        .arg(ramBase + ramSize, 16, 16, QChar('0'))
        .arg((ramBase + ramSize) / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2));

    // ========================================================================
    // Route 2: MMIOManager -- Typhoon PCI I/O space
    // ========================================================================
    m_routes.append({
        .startPA = mmioBase,
        .endPA = mmioBase + mmioSize,
        .target = RouteTarget::MMIOManager,
        .offsetBase = 0x0
        });

    INFO_LOG(QString("  [0x%1 - 0x%2) -> MMIOManager (%3 GB, PA direct)")
        .arg(mmioBase, 16, 16, QChar('0'))
        .arg(mmioBase + mmioSize, 16, 16, QChar('0'))
        .arg(mmioSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2));

    INFO_LOG("=== GuestMemory: PA Routing Table Initialized (2 routes) ===");

    // ========================================================================
    // Validate no overlaps
    // ========================================================================
    for (int i = 0; i < m_routes.size(); ++i) {
        for (int j = i + 1; j < m_routes.size(); ++j) {
            if (m_routes[i].overlaps(m_routes[j])) {
                ERROR_LOG(QString("Route overlap: [0x%1-0x%2) and [0x%3-0x%4)")
                    .arg(m_routes[i].startPA, 16, 16, QChar('0'))
                    .arg(m_routes[i].endPA, 16, 16, QChar('0'))
                    .arg(m_routes[j].startPA, 16, 16, QChar('0'))
                    .arg(m_routes[j].endPA, 16, 16, QChar('0')));
            }
        }
    }
}

AXP_HOT AXP_FLATTEN void GuestMemory::setRoutes(const QVector<PARouteEntry>& routes) noexcept
{
    m_routes = routes;
    INFO_LOG(QString("GuestMemory: Custom routing table set (%1 routes)").arg(routes.size()));
}

// ============================================================================
// PA CLASSIFICATION
// ============================================================================

AXP_HOT AXP_FLATTEN GuestMemory::RouteTarget GuestMemory::classifyPA(quint64 pa) const noexcept
{
    for (const auto& route : m_routes) {
        if (route.contains(pa)) {
            return route.target;
        }
    }
    return RouteTarget::None;
}

AXP_HOT AXP_FLATTEN GuestMemory::RouteTarget GuestMemory::classifyPARange(quint64 pa, quint64 len) const noexcept
{
    for (const auto& route : m_routes) {
        if (route.containsRange(pa, len)) {
            return route.target;
        }
    }
    return RouteTarget::None;
}

AXP_HOT AXP_FLATTEN bool GuestMemory::isRAM(quint64 pa, quint64 len) const noexcept
{
    return classifyPARange(pa, len) == RouteTarget::SafeMemory;
}

AXP_HOT AXP_FLATTEN bool GuestMemory::isMMIO(quint64 pa, quint64 len) const noexcept
{
    return classifyPARange(pa, len) == RouteTarget::MMIOManager;
}

AXP_HOT AXP_FLATTEN bool GuestMemory::isValidPhysicalAddress(quint64 pa, quint64 len) const noexcept
{
    return classifyPARange(pa, len) != RouteTarget::None;
}

// ============================================================================
// ROUTE FINDING
// ============================================================================

AXP_HOT AXP_FLATTEN const GuestMemory::PARouteEntry* GuestMemory::findRoute(quint64 pa) const noexcept
{
    for (const auto& route : m_routes) {
        if (route.contains(pa)) {
            return &route;
        }
    }
    return nullptr;
}

// ============================================================================
// CORE ROUTED ACCESS
//
// SafeMemory API:   load(offset, width, outValue)  / store(offset, width, value)
// MMIOManager API:  handleRead(pa, width, outValue) / handleWrite(pa, width, value)
// ============================================================================

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::readRouted(quint64 pa, quint8 width, quint64& outValue, AccessKind kind) const noexcept
{
    outValue = 0;

    if (!m_safeMem || !m_mmio) {
        ERROR_LOG("GuestMemory: Subsystems not attached");
        return MEM_STATUS::NotInitialized;
    }

    const PARouteEntry* route = findRoute(pa);
    if (!route) {
        TRACE_LOG(QString("GuestMemory: Unmapped PA 0x%1")
            .arg(pa, 16, 16, QChar('0')));
        return MEM_STATUS::AccessViolation;
    }

    // MMIO is never executable
    if (kind == AccessKind::InstructionFetch && route->target == RouteTarget::MMIOManager) {
        WARN_LOG(QString("GuestMemory: Attempt to execute from MMIO at PA 0x%1")
            .arg(pa, 16, 16, QChar('0')));
        return MEM_STATUS::AccessViolation;
    }

    const quint64 offset = route->calculateOffset(pa);

    switch (route->target)
    {
    case RouteTarget::SafeMemory:
        return m_safeMem->load(offset, width, outValue);

    case RouteTarget::MMIOManager:
        return m_mmio->handleRead(pa, width, outValue);

    default:
        return MEM_STATUS::AccessViolation;
    }
}

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::writeRouted(quint64 pa, quint8 width, quint64 value, AccessKind kind) const noexcept
{
    if (!m_safeMem || !m_mmio) {
        ERROR_LOG("GuestMemory: Subsystems not attached");
        return MEM_STATUS::NotInitialized;
    }

    const PARouteEntry* route = findRoute(pa);
    if (!route) {
        TRACE_LOG(QString("GuestMemory: Unmapped write PA 0x%1")
            .arg(pa, 16, 16, QChar('0')));
        return MEM_STATUS::AccessViolation;
    }

    const quint64 offset = route->calculateOffset(pa);

    switch (route->target)
    {
    case RouteTarget::SafeMemory:
        return m_safeMem->store(offset, width, value);

    case RouteTarget::MMIOManager:
        return m_mmio->handleWrite(pa, width, value);

    default:
        return MEM_STATUS::AccessViolation;
    }
}

// ============================================================================
// INSTRUCTION FETCH
// ============================================================================

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::readInst32(quint64 pa, quint32& value) const noexcept
{
    quint64 tmp = 0;
    const MEM_STATUS status = readRouted(pa, 4, tmp, AccessKind::InstructionFetch);
    value = static_cast<quint32>(tmp);
    return status;
}

// ============================================================================
// TYPED READS/WRITES
// ============================================================================

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::read8(quint64 pa, quint8& value) const noexcept
{
    quint64 tmp = 0;
    const MEM_STATUS status = readRouted(pa, 1, tmp, AccessKind::DataRead);
    value = static_cast<quint8>(tmp);
    return status;
}

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::read16(quint64 pa, quint16& value) const noexcept
{
    quint64 tmp = 0;
    const MEM_STATUS status = readRouted(pa, 2, tmp, AccessKind::DataRead);
    value = static_cast<quint16>(tmp);
    return status;
}

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::read32(quint64 pa, quint32& value) const noexcept
{
    quint64 tmp = 0;
    const MEM_STATUS status = readRouted(pa, 4, tmp, AccessKind::DataRead);
    value = static_cast<quint32>(tmp);
    return status;
}

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::read64(quint64 pa, quint64& value) const noexcept
{
    return readRouted(pa, 8, value, AccessKind::DataRead);
}

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::write8(quint64 pa, quint8 value) const noexcept
{
    return writeRouted(pa, 1, static_cast<quint64>(value), AccessKind::DataWrite);
}

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::write16(quint64 pa, quint16 value) const noexcept
{
    return writeRouted(pa, 2, static_cast<quint64>(value), AccessKind::DataWrite);
}

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::write32(quint64 pa, quint32 value) const noexcept
{
    return writeRouted(pa, 4, static_cast<quint64>(value), AccessKind::DataWrite);
}

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::write64(quint64 pa, quint64 value) const noexcept
{
    return writeRouted(pa, 8, value, AccessKind::DataWrite);
}

// ============================================================================
// BLOCK ACCESS (RAM-ONLY BY DESIGN)
// ============================================================================

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::readPA(quint64 pa, void* dst, qsizetype len) const noexcept
{
    if (!isRAM(pa, len)) {
        ERROR_LOG(QString("GuestMemory: Block read from non-RAM PA 0x%1")
            .arg(pa, 16, 16, QChar('0')));
        return MEM_STATUS::AccessViolation;
    }

    const PARouteEntry* route = findRoute(pa);
    if (!route) {
        return MEM_STATUS::AccessViolation;
    }

    const quint64 offset = route->calculateOffset(pa);
    return m_safeMem->readBlock(offset, dst, len);
}

AXP_HOT AXP_FLATTEN MEM_STATUS GuestMemory::writePA(quint64 pa, const void* src, qsizetype len) const noexcept
{
    if (!isRAM(pa, len)) {
        ERROR_LOG(QString("GuestMemory: Block write to non-RAM PA 0x%1")
            .arg(pa, 16, 16, QChar('0')));
        return MEM_STATUS::AccessViolation;
    }

    const PARouteEntry* route = findRoute(pa);
    if (!route) {
        return MEM_STATUS::AccessViolation;
    }

    const quint64 offset = route->calculateOffset(pa);
    return m_safeMem->writeBlock(offset, src, len);
}

// ============================================================================
// SPAN ACCESS (PREFERRED FOR BUFFERS)
// ============================================================================

MemorySpan GuestMemory::getSpanToPA(quint64 pa, quint64 requestedLen, AccessIntent intent) const noexcept
{
    const PARouteEntry* route = findRoute(pa);
    if (!route) {
        return { nullptr, 0, false };
    }

    // MMIO does not support direct memory spans
    if (route->target == RouteTarget::MMIOManager) {
        return { nullptr, 0, false };
    }

    const quint64 offset = route->calculateOffset(pa);
    const quint64 bytesAvailInRegion = route->endPA - pa;
    const quint64 truncatedLen = std::min(requestedLen, bytesAvailInRegion);

    if (!m_safeMem) {
        return { .data = nullptr, .len = 0, .writable = false };
    }
    return m_safeMem->getSpan(offset, truncatedLen, intent);
}

// ============================================================================
// DMA COHERENCY
// ============================================================================

AXP_HOT AXP_FLATTEN void GuestMemory::notifyDMAWriteComplete(quint64 pa, quint32 size) const noexcept
{
    TRACE_LOG(QString("GuestMemory: DMA write complete at PA 0x%1, size %2")
        .arg(pa, 16, 16, QChar('0'))
        .arg(size));
}

// ============================================================================
// DIAGNOSTICS
// ============================================================================

AXP_HOT AXP_FLATTEN QString GuestMemory::classifyPhysicalAddress(quint64 pa) const noexcept
{
    const PARouteEntry* route = findRoute(pa);
    if (!route) {
        return QString("Unmapped PA (no route)");
    }

    const quint64 offset = route->calculateOffset(pa);

    switch (route->target) {
    case RouteTarget::SafeMemory:
        return QString("SafeMemory (offset 0x%1)")
            .arg(offset, 8, 16, QChar('0'));

    case RouteTarget::MMIOManager:
        return QString("MMIOManager (device PA 0x%1)")
            .arg(pa, 16, 16, QChar('0'));

    default:
        return QString("Unknown target");
    }
}