// ============================================================================
// GuestPhysicalRegionRegistry.h -- Guest PA Space Registry
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025, 2026 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic)
// ============================================================================
//
// Single authoritative map of every guest physical address region.
// Instantiated at emulator top level (EmulatR_init / PlatformState).
// Passed by reference to SrmRomLoader, GuestMemory, and device managers.
//
// Ownership:
//   EmulatR_init owns the registry instance.
//   All subsystems hold a non-owning reference.
//
// Thread safety:
//   Population is single-threaded (init path only).
//   Reads during execution are safe as long as population is complete
//   before Phase 14c begins. SMP will require a shared_mutex -- deferred.
//
// See: GuestPhysicalRegionRegistry_Spec.md
// ============================================================================

#ifndef GUESTPHYSICALREGIONREGISTRY_H
#define GUESTPHYSICALREGIONREGISTRY_H

#include "GuestPhysicalRegion.h"
#include <vector>
#include <QString>

// Forward declaration -- avoid pulling SrmRomLoader into every consumer
struct SrmRomDescriptor;

// SrmSnapshotMemRegion forward -- defined in SrmRomLoader.h
// Registry produces these for saveSnapshot() / loadSnapshot()
struct SrmSnapshotMemRegion;

// ============================================================================
// GuestPhysicalRegionRegistry
// ============================================================================

class GuestPhysicalRegionRegistry
{
public:
    GuestPhysicalRegionRegistry() = default;
    ~GuestPhysicalRegionRegistry() = default;

    // Non-copyable -- single instance owned by EmulatR_init
    GuestPhysicalRegionRegistry(const GuestPhysicalRegionRegistry&) = delete;
    GuestPhysicalRegionRegistry& operator=(const GuestPhysicalRegionRegistry&) = delete;

    // -- Population ----------------------------------------------------------

    // Add a region. Returns false and logs ERROR on PA overlap.
    // Regions must not overlap -- any overlap is a hard configuration error.
    bool add(const GuestPhysicalRegion& region);

    // Typed convenience adders.
    // All call add() internally -- overlap detection applies to all.

    // Phase 14a -- from SrmRomDescriptor (loadFromFile / useEmbedded)
    bool addFirmware(quint64 basePA, quint64 size,
        const QString& desc,
        GuestRegionSource source = GuestRegionSource::FirmwareBinary);

    bool addDecompressedFW(quint64 basePA, quint64 size,
        const QString& desc,
        GuestRegionSource source = GuestRegionSource::FirmwareBinary);

    // Phase 14b -- post-decompress
    bool addPalcode(quint64 basePA, quint64 size,
        const QString& desc,
        GuestRegionSource source = GuestRegionSource::HwrpbPostBoot);

    bool addHwrpb(quint64 basePA = 0x2000, quint64 size = 0x4000);

    // Init -- MemoryMapConfig
    bool addRam(quint64 basePA, quint64 size);
    bool addMmio(quint64 basePA, quint64 size, const QString& desc);
    bool addPci(quint64 basePA, quint64 size, const QString& desc);

    // Seed Phase 14a entries from a populated SrmRomDescriptor.
    // Extracts Firmware and DecompressedFW entries from descriptor.regions.
    // loadPA required to correctly label the Firmware entry.
    void seedFromDescriptor(const SrmRomDescriptor& desc, quint64 loadPA);

    // -- Lifecycle -----------------------------------------------------------

    // Mark all regions of the given type as readOnly.
    // Call after Phase 14b to seal DecompressedFW and PALcode.
    void sealRegions(GuestRegionType type);

    // Mark a region as populated (written to guest memory).
    void markPopulated(quint64 basePA);

    // -- Query ---------------------------------------------------------------

    // Find the region containing pa. Returns nullptr if no region covers it.
    const GuestPhysicalRegion* findRegion(quint64 pa) const;

    // Find first region of a given type. Returns nullptr if none.
    const GuestPhysicalRegion* findByType(GuestRegionType type) const;

    // All regions of a given type.
    std::vector<const GuestPhysicalRegion*> allOfType(GuestRegionType type) const;

    // All regions flagged includeInSnapshot=true, as SrmSnapshotMemRegion vector.
    // Replaces the hardcoded region vector in EmulatR_init Phase 14b.
    std::vector<SrmSnapshotMemRegion> snapshotRegions() const;

    // All regions flagged hwrpbVisible=true.
    // Used by HWRPB MDP construction (Phase 14c / future).
    std::vector<GuestPhysicalRegion> hwrpbRegions() const;

    // -- Validation ----------------------------------------------------------

    // Check all registered regions for PA overlaps.
    // Returns true if clean. Populates errorOut with first overlap found.
    // Call before Phase 14c begins.
    bool validate(QString* errorOut = nullptr) const;

    // -- Diagnostics ---------------------------------------------------------

    // Dump complete registry to INFO_LOG.
    void logAll() const;

    // -- Access --------------------------------------------------------------

    const std::vector<GuestPhysicalRegion>& all() const { return m_regions; }
    size_t count() const { return m_regions.size(); }
    bool   empty() const { return m_regions.empty(); }

private:
    std::vector<GuestPhysicalRegion> m_regions;

    // Internal helper -- find mutable region by basePA
    GuestPhysicalRegion* findMutable(quint64 basePA);
};

#endif // GUESTPHYSICALREGIONREGISTRY_H