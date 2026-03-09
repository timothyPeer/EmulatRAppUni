// ============================================================================
// GuestPhysicalRegion.h -- Guest Physical Address Space Region Descriptor
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025, 2026 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic)
// ============================================================================
//
// Platform layer header -- no subsystem dependencies.
//
// Consumed by:
//   SrmRomLoader          -- seeds Firmware and DecompressedFW entries
//   GuestPhysicalRegionRegistry -- owns the full registry
//   GuestMemory           -- write validation, snapshot integration
//   EmulatR_init          -- registry instantiation and population sequence
//   Future device managers-- MMIO and PCI region registration
//
// Do NOT add includes from any emulator subsystem to this file.
// This header must remain a leaf with no upward dependencies.
//
// Population sequence:
//   Phase 14a  loadFromFile() / useEmbedded()
//                  Firmware, DecompressedFW  (from SrmRomDescriptor)
//   Phase 14b  decompress() completion
//                  PALcode, HWRPB            (from post-decompress reads)
//   Init       MemoryMapConfig
//                  RAM, MMIO, PCI            (from static configuration)
//
// See: GuestPhysicalRegionRegistry_Spec.md for full design rationale.
// ============================================================================

#ifndef GUESTPHYSICALREGION_H
#define GUESTPHYSICALREGION_H

#include <QtTypes>
#include <QString>

// ============================================================================
// GuestRegionType
// ============================================================================

enum class GuestRegionType
{
    Firmware,        // decompressor stub staging area     (e.g. PA 0x900000)
    DecompressedFW,  // SRM console image after unpack     (e.g. PA 0x000000)
    HWRPB,           // Hardware Restart Parameter Block   (PA 0x2000, arch constant)
    PALcode,         // PAL dispatch image                 (at PAL_BASE, e.g. 0x600000)
    RAM,             // main system memory (DMA-accessible)
    MMIO,            // memory-mapped device registers
    PCI,             // PCI BAR space
    Reserved,        // holes / architecture-reserved / not populated
    Unknown          // placeholder during incremental population
};

// ============================================================================
// GuestRegionSource
// ============================================================================

enum class GuestRegionSource
{
    Architecture,    // Alpha AXP specification constant (e.g. HWRPB at 0x2000)
    FirmwareBinary,  // derived from decompressor stub analysis at load time
    HwrpbPostBoot,   // read from HWRPB memory descriptors after decompression
    MemoryMapConfig, // static INI / MemoryMapConfig struct
    DeviceProbe      // discovered by device enumeration at runtime
};

// ============================================================================
// GuestPhysicalRegion
// ============================================================================

struct GuestPhysicalRegion
{
    // -- Identity ------------------------------------------------------------
    quint64           basePA = 0;
    quint64           size = 0;
    GuestRegionType   type = GuestRegionType::Unknown;
    GuestRegionSource source = GuestRegionSource::MemoryMapConfig;
    QString           description;         // e.g. "ES40_V6_2.EXE decompressor stub"

    // -- Lifecycle -----------------------------------------------------------
    bool              populated = false;  // region has been written to
    bool              readOnly = false;  // writes blocked after sealing
    // (set true for DecompressedFW
    //  after Phase 14b completes)

// -- Snapshot ------------------------------------------------------------
// If true, this region is saved and restored in .axpsnap files.
// Default assignments:
//   Firmware, DecompressedFW, PALcode  = true
//   RAM                                = true  (large -- may be opt-in later)
//   HWRPB                              = true  (small, critical)
//   MMIO, PCI                          = false (device state handled separately)
    bool              includeInSnapshot = false;

    // -- HWRPB memory descriptor ---------------------------------------------
    // If true, this region appears in the HWRPB MDP (memory descriptor page).
    // RAM = true. Firmware/MMIO/PCI/HWRPB itself = false.
    // Populated during HWRPB construction in Phase 14c.
    bool              hwrpbVisible = false;

    // -- Helpers -------------------------------------------------------------
    quint64 endPA() const
    {
        return basePA + size;
    }

    bool contains(quint64 pa) const
    {
        return pa >= basePA && pa < endPA();
    }

    bool overlaps(const GuestPhysicalRegion& other) const
    {
        return basePA < other.endPA() && endPA() > other.basePA;
    }

    // Human-readable type string for logging
    static const char* typeName(GuestRegionType t)
    {
        switch (t) {
        case GuestRegionType::Firmware:       return "Firmware";
        case GuestRegionType::DecompressedFW: return "DecompressedFW";
        case GuestRegionType::HWRPB:          return "HWRPB";
        case GuestRegionType::PALcode:        return "PALcode";
        case GuestRegionType::RAM:            return "RAM";
        case GuestRegionType::MMIO:           return "MMIO";
        case GuestRegionType::PCI:            return "PCI";
        case GuestRegionType::Reserved:       return "Reserved";
        case GuestRegionType::Unknown:        return "Unknown";
        default:                              return "?";
        }
    }

    static const char* sourceName(GuestRegionSource s)
    {
        switch (s) {
        case GuestRegionSource::Architecture:    return "Architecture";
        case GuestRegionSource::FirmwareBinary:  return "FirmwareBinary";
        case GuestRegionSource::HwrpbPostBoot:   return "HwrpbPostBoot";
        case GuestRegionSource::MemoryMapConfig: return "MemoryMapConfig";
        case GuestRegionSource::DeviceProbe:     return "DeviceProbe";
        default:                                 return "?";
        }
    }

    const char* typeName()   const { return typeName(type); }
    const char* sourceName() const { return sourceName(source); }
};

#endif // GUESTPHYSICALREGION_H