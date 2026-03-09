// ============================================================================
// GuestPhysicalRegionRegistry.cpp -- Guest PA Space Registry
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025, 2026 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic)
// ============================================================================

#include "GuestPhysicalRegionRegistry.h"
#include "SrmRomLoader.h"           // SrmRomDescriptor, SrmSnapshotMemRegion
#include "coreLib/LoggingMacros.h"
#include <algorithm>

#define COMPONENT_NAME "GuestPhysRegReg"

// ============================================================================
// Internal helpers
// ============================================================================

GuestPhysicalRegion* GuestPhysicalRegionRegistry::findMutable(quint64 basePA)
{
    for (auto& r : m_regions)
        if (r.basePA == basePA)
            return &r;
    return nullptr;
}

// ============================================================================
// add
// ============================================================================

bool GuestPhysicalRegionRegistry::add(const GuestPhysicalRegion& region)
{
    if (region.size == 0) {
        WARN_LOG(QString("GuestPhysicalRegionRegistry::add -- zero-size region "
            "'%1' at PA 0x%2 -- ignored")
            .arg(region.description)
            .arg(region.basePA, 0, 16));
        return false;
    }

    // Overlap detection
    for (const auto& existing : m_regions) {
        if (region.overlaps(existing)) {
            ERROR_LOG(QString("GuestPhysicalRegionRegistry: PA overlap detected!"));
            ERROR_LOG(QString("  New     : [0x%1 - 0x%2)  %3  '%4'")
                .arg(region.basePA, 0, 16)
                .arg(region.endPA(), 0, 16)
                .arg(region.typeName())
                .arg(region.description));
            ERROR_LOG(QString("  Existing: [0x%1 - 0x%2)  %3  '%4'")
                .arg(existing.basePA, 0, 16)
                .arg(existing.endPA(), 0, 16)
                .arg(existing.typeName())
                .arg(existing.description));
            return false;
        }
    }

    m_regions.push_back(region);

    // Keep sorted by basePA for log readability and binary search (future)
    std::sort(m_regions.begin(), m_regions.end(),
        [](const GuestPhysicalRegion& a, const GuestPhysicalRegion& b) {
            return a.basePA < b.basePA;
        });

    INFO_LOG(QString("GuestPhysRegReg: added [0x%1 - 0x%2)  %-14s  %3  '%4'")
        .arg(region.basePA, 0, 16)
        .arg(region.endPA(), 0, 16)
        .arg(region.typeName())
        .arg(region.sourceName())
        .arg(region.description));

    return true;
}

// ============================================================================
// Typed convenience adders
// ============================================================================

bool GuestPhysicalRegionRegistry::addFirmware(
    quint64 basePA, quint64 size, const QString& desc, GuestRegionSource source)
{
    GuestPhysicalRegion r;
    r.basePA = basePA;
    r.size = size;
    r.type = GuestRegionType::Firmware;
    r.source = source;
    r.description = desc;
    r.includeInSnapshot = true;
    r.hwrpbVisible = false;
    return add(r);
}

bool GuestPhysicalRegionRegistry::addDecompressedFW(
    quint64 basePA, quint64 size, const QString& desc, GuestRegionSource source)
{
    GuestPhysicalRegion r;
    r.basePA = basePA;
    r.size = size;
    r.type = GuestRegionType::DecompressedFW;
    r.source = source;
    r.description = desc;
    r.includeInSnapshot = true;
    r.hwrpbVisible = false;
    return add(r);
}

bool GuestPhysicalRegionRegistry::addPalcode(
    quint64 basePA, quint64 size, const QString& desc, GuestRegionSource source)
{
    GuestPhysicalRegion r;
    r.basePA = basePA;
    r.size = size;
    r.type = GuestRegionType::PALcode;
    r.source = source;
    r.description = desc;
    r.includeInSnapshot = true;
    r.hwrpbVisible = false;
    return add(r);
}

bool GuestPhysicalRegionRegistry::addHwrpb(quint64 basePA, quint64 size)
{
    GuestPhysicalRegion r;
    r.basePA = basePA;
    r.size = size;
    r.type = GuestRegionType::HWRPB;
    r.source = GuestRegionSource::Architecture;
    r.description = "Hardware Restart Parameter Block (Alpha arch constant)";
    r.includeInSnapshot = true;
    r.hwrpbVisible = false;  // HWRPB itself is not in its own MDP
    return add(r);
}

bool GuestPhysicalRegionRegistry::addRam(quint64 basePA, quint64 size)
{
    GuestPhysicalRegion r;
    r.basePA = basePA;
    r.size = size;
    r.type = GuestRegionType::RAM;
    r.source = GuestRegionSource::MemoryMapConfig;
    r.description = QString("RAM  0x%1 + %2 MB")
        .arg(basePA, 0, 16)
        .arg(size / (1024 * 1024));
    r.includeInSnapshot = true;   // large -- may become opt-in, see spec §10.3
    r.includeInSnapshot = false;   // RAM excluded -- note: Region [3] is RAM at 32GB with includeInSnapshot=true. That snapshot save will try to write a ~42GB file.
    r.hwrpbVisible = true;
    return add(r);
}

bool GuestPhysicalRegionRegistry::addMmio(
    quint64 basePA, quint64 size, const QString& desc)
{
    GuestPhysicalRegion r;
    r.basePA = basePA;
    r.size = size;
    r.type = GuestRegionType::MMIO;
    r.source = GuestRegionSource::MemoryMapConfig;
    r.description = desc;
    r.includeInSnapshot = false;  // device state handled separately
    r.hwrpbVisible = false;
    return add(r);
}

bool GuestPhysicalRegionRegistry::addPci(
    quint64 basePA, quint64 size, const QString& desc)
{
    GuestPhysicalRegion r;
    r.basePA = basePA;
    r.size = size;
    r.type = GuestRegionType::PCI;
    r.source = GuestRegionSource::MemoryMapConfig;
    r.description = desc;
    r.includeInSnapshot = false;
    r.hwrpbVisible = false;
    return add(r);
}

// ============================================================================
// seedFromDescriptor
// ============================================================================

void GuestPhysicalRegionRegistry::seedFromDescriptor(
    const SrmRomDescriptor& desc, quint64 loadPA)
{
    if (!desc.valid) {
        WARN_LOG("GuestPhysRegReg::seedFromDescriptor -- descriptor not valid, skipping");
        return;
    }

    for (const auto& region : desc.regions)
        add(region);

    // PALcode region -- basePA known from descriptor, size TBD until HWRPB analysis.
    // Register with conservative placeholder size. Update after Phase 14b.
    // Size 0x200000 (2MB) covers all known ES40/ES45 variants conservatively.
    addPalcode(
        desc.palBase,
        0x200000ULL,
        QString("PALcode -- %1  [size provisional, confirm from HWRPB MDP]")
        .arg(desc.sourceDescription),
        GuestRegionSource::FirmwareBinary
    );

    // HWRPB -- Alpha architecture constant, always PA 0x2000
    // Size confirmed after HWRPB analysis session. 0x4000 is conservative.
    addHwrpb(0x2000, 0x4000);

    INFO_LOG(QString("GuestPhysRegReg: seeded %1 entries from descriptor '%2'")
        .arg(m_regions.size())
        .arg(desc.sourceDescription));
}

// ============================================================================
// Lifecycle
// ============================================================================

void GuestPhysicalRegionRegistry::sealRegions(GuestRegionType type)
{
    int count = 0;
    for (auto& r : m_regions) {
        if (r.type == type) {
            r.readOnly = true;
            ++count;
        }
    }
    INFO_LOG(QString("GuestPhysRegReg: sealed %1 region(s) of type %2 as read-only")
        .arg(count)
        .arg(GuestPhysicalRegion::typeName(type)));
}

void GuestPhysicalRegionRegistry::markPopulated(quint64 basePA)
{
    if (auto* r = findMutable(basePA))
        r->populated = true;
}

// ============================================================================
// Query
// ============================================================================

const GuestPhysicalRegion* GuestPhysicalRegionRegistry::findRegion(quint64 pa) const
{
    for (const auto& r : m_regions)
        if (r.contains(pa))
            return &r;
    return nullptr;
}

const GuestPhysicalRegion* GuestPhysicalRegionRegistry::findByType(
    GuestRegionType type) const
{
    for (const auto& r : m_regions)
        if (r.type == type)
            return &r;
    return nullptr;
}

std::vector<const GuestPhysicalRegion*>
GuestPhysicalRegionRegistry::allOfType(GuestRegionType type) const
{
    std::vector<const GuestPhysicalRegion*> result;
    for (const auto& r : m_regions)
        if (r.type == type)
            result.push_back(&r);
    return result;
}

std::vector<SrmSnapshotMemRegion>
GuestPhysicalRegionRegistry::snapshotRegions() const
{
    std::vector<SrmSnapshotMemRegion> result;
    for (const auto& r : m_regions)
        if (r.includeInSnapshot)
            result.push_back({ r.basePA, r.size });
    return result;
}

std::vector<GuestPhysicalRegion>
GuestPhysicalRegionRegistry::hwrpbRegions() const
{
    std::vector<GuestPhysicalRegion> result;
    for (const auto& r : m_regions)
        if (r.hwrpbVisible)
            result.push_back(r);
    return result;
}

// ============================================================================
// validate
// ============================================================================

bool GuestPhysicalRegionRegistry::validate(QString* errorOut) const
{
    for (size_t i = 0; i < m_regions.size(); ++i) {
        for (size_t j = i + 1; j < m_regions.size(); ++j) {
            if (m_regions[i].overlaps(m_regions[j])) {
                const QString msg = QString(
                    "GuestPhysRegReg: overlap [0x%1-0x%2) '%3' <-> [0x%4-0x%5) '%6'")
                    .arg(m_regions[i].basePA, 0, 16)
                    .arg(m_regions[i].endPA(), 0, 16)
                    .arg(m_regions[i].description)
                    .arg(m_regions[j].basePA, 0, 16)
                    .arg(m_regions[j].endPA(), 0, 16)
                    .arg(m_regions[j].description);
                ERROR_LOG(msg);
                if (errorOut) *errorOut = msg;
                return false;
            }
        }
    }
    INFO_LOG(QString("GuestPhysRegReg: validation passed -- %1 regions, no overlaps")
        .arg(m_regions.size()));
    return true;
}

// ============================================================================
// logAll
// ============================================================================

void GuestPhysicalRegionRegistry::logAll() const
{
    INFO_LOG(QString("GuestPhysRegReg: %1 region(s) registered:").arg(m_regions.size()));
    for (const auto& r : m_regions) {
        INFO_LOG(QString("  [0x%1 - 0x%2)  %-14s  snap=%3  ro=%4  '%5'")
            .arg(r.basePA, 0, 16)
            .arg(r.endPA(), 0, 16)
            .arg(r.typeName())
            .arg(r.includeInSnapshot ? "Y" : "N")
            .arg(r.readOnly ? "Y" : "N")
            .arg(r.description));
    }
}