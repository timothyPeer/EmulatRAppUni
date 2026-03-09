# GuestPhysicalRegion Registry — Design Specification
**Project:** ASA-EMulatR — Alpha AXP Architecture Emulator  
**Author:** Timothy Peer / Claude (Anthropic)  
**Status:** Draft — pending implementation  
**Date:** 2026-03-08  

---

## 1. Problem Statement

EmulatR currently manages guest physical address (PA) space through a
combination of ad-hoc INI values, hardcoded constants, and implicit
assumptions scattered across multiple subsystems. As the emulator matures,
the following questions must be answerable at runtime from a single
authoritative source:

- Is this PA range available for RAM allocation?
- Which regions must be included in a snapshot?
- Where did the decompressor write the firmware image?
- What are the HWRPB memory descriptor entries?
- Do any two subsystems claim overlapping PA ranges?
- What is the platform memory topology for a given firmware variant?

No single data structure currently answers all of these. The result is
tight coupling between subsystems, brittle INI-driven configuration, and
risk of silent PA overlap bugs as new devices and memory regions are added.

---

## 2. Goals

1. **Single source of truth** — one registry owns the complete guest PA map.
2. **Self-describing** — every entry carries its origin, type, and lifecycle.
3. **Incremental population** — entries are added as subsystems come online,
   not all at once at startup.
4. **Snapshot-aware** — the registry drives region selection for `.axpsnap`
   save/load, replacing the current hardcoded `SrmSnapshotMemRegion` vector.
5. **HWRPB-ready** — registry entries map directly to Alpha HWRPB memory
   descriptor (`MDP`) construction.
6. **Overlap detection** — the registry validates that no two entries claim
   the same PA range.
7. **Zero INI dependency** — firmware-derived entries (stub, decompressed
   image, PALcode, HWRPB) are populated from binary analysis and post-
   decompress reads, not from configuration files.

---

## 3. Current State (as of 2026-03-08)

| Region | Current handling |
|--------|-----------------|
| SRM decompressor stub | `SrmLoaderConfig::loadPA` from INI `SrmBase` |
| Decompressed firmware | Implicit — written to PA `0x0`, size assumed `0x400000` |
| HWRPB | `HwrpbBase=0x2000` (arch constant), `HwrpbSize=0x4000` in INI |
| PALcode | `PAL_BASE=0x600000` from stub+0x10, not registered anywhere |
| RAM | `RamBase` + computed size from `System.MemorySizeGB` |
| MMIO | `MmioBase` + `MmioSize` from INI |
| PCI | `PciMemBase` + `PciMemSize` from INI |
| Snapshot regions | Hardcoded vector in `EmulatR_init.cpp` Phase 14b |

---

## 4. Proposed Data Structures

### 4.1 GuestRegionType

```cpp
enum class GuestRegionType {
    Firmware,        // decompressor stub staging area
    DecompressedFW,  // SRM console + PAL dispatch image
    HWRPB,           // Hardware Restart Parameter Block (PA 0x2000)
    PALcode,         // PAL dispatch image (at PAL_BASE)
    RAM,             // main system memory (DMA-accessible)
    MMIO,            // memory-mapped device registers
    PCI,             // PCI BAR space
    Reserved,        // holes, not populated, or architecture-reserved
    Unknown          // placeholder during incremental population
};
```

### 4.2 GuestRegionSource

```cpp
enum class GuestRegionSource {
    Architecture,    // Alpha AXP spec constant (e.g. HWRPB at 0x2000)
    FirmwareBinary,  // derived from decompressor stub analysis
    HwrpbPostBoot,   // read from HWRPB after decompression completes
    MemoryMapConfig, // static INI / MemoryMapConfig struct
    DeviceProbe,     // discovered by device enumeration at runtime
};
```

### 4.3 GuestPhysicalRegion

```cpp
struct GuestPhysicalRegion
{
    // -- Identity ------------------------------------------------------------
    quint64           basePA               = 0;
    quint64           size                 = 0;
    GuestRegionType   type                 = GuestRegionType::Unknown;
    GuestRegionSource source               = GuestRegionSource::MemoryMapConfig;
    QString           description;         // human-readable, e.g. "ES40_V6_2 stub"

    // -- Lifecycle -----------------------------------------------------------
    bool              populated            = false;  // region has been written
    bool              readOnly             = false;  // ROM regions

    // -- Snapshot ------------------------------------------------------------
    bool              includeInSnapshot    = false;
    // If true, this region is saved/restored in .axpsnap files.
    // Firmware, DecompressedFW, PALcode = true
    // RAM = true (future -- large, may be optional)
    // MMIO, PCI = false (device state handled separately)

    // -- HWRPB memory descriptor ---------------------------------------------
    bool              hwrpbVisible         = false;
    // If true, this region appears in the HWRPB memory descriptor (MDP).
    // RAM = true, Firmware/MMIO/PCI = false, HWRPB itself = false

    // -- Helpers -------------------------------------------------------------
    quint64 endPA()   const { return basePA + size; }
    bool    contains(quint64 pa) const { return pa >= basePA && pa < endPA(); }
    bool    overlaps(const GuestPhysicalRegion& other) const {
        return basePA < other.endPA() && endPA() > other.basePA;
    }
};
```

### 4.4 GuestPhysicalRegionRegistry

```cpp
class GuestPhysicalRegionRegistry
{
public:
    // -- Population ----------------------------------------------------------

    // Add a region. Returns false and logs error on PA overlap.
    bool add(const GuestPhysicalRegion& region);

    // Convenience adders for common region types
    bool addFirmware(quint64 basePA, quint64 size, const QString& desc);
    bool addDecompressedFW(quint64 basePA, quint64 size, const QString& desc);
    bool addHwrpb(quint64 basePA, quint64 size);
    bool addPalcode(quint64 basePA, quint64 size, const QString& desc);
    bool addRam(quint64 basePA, quint64 size);
    bool addMmio(quint64 basePA, quint64 size, const QString& desc);
    bool addPci(quint64 basePA, quint64 size, const QString& desc);

    // Seed from SrmRomDescriptor (called after loadFromFile / useEmbedded)
    void seedFromDescriptor(const SrmRomDescriptor& desc, quint64 loadPA);

    // -- Query ---------------------------------------------------------------

    // Find region containing a given PA (nullptr if none)
    const GuestPhysicalRegion* findRegion(quint64 pa) const;

    // Find region by type (first match)
    const GuestPhysicalRegion* findByType(GuestRegionType type) const;

    // All regions of a given type
    std::vector<const GuestPhysicalRegion*> allOfType(GuestRegionType type) const;

    // All regions flagged for snapshot inclusion
    std::vector<SrmSnapshotMemRegion> snapshotRegions() const;

    // All regions flagged for HWRPB MDP inclusion
    std::vector<GuestPhysicalRegion> hwrpbRegions() const;

    // Validate -- check for overlaps across all registered regions
    bool validate(QString* errorOut = nullptr) const;

    // Dump full registry to log
    void logAll() const;

    // -- Access --------------------------------------------------------------
    const std::vector<GuestPhysicalRegion>& all() const { return m_regions; }
    size_t count() const { return m_regions.size(); }

private:
    std::vector<GuestPhysicalRegion> m_regions;
};
```

---

## 5. Population Sequence

### Phase 14a — ROM load (`loadFromFile` / `useEmbedded`)
```
populateDescriptor() seeds:
  GuestRegionType::Firmware        basePA=loadPA   size=payloadSize
  GuestRegionType::DecompressedFW  basePA=0x0      size=0x400000
```

### Phase 14b — Post-decompress
```
After decompress() succeeds, add:
  GuestRegionType::PALcode         basePA=descriptor.palBase  size=TBD
  GuestRegionType::HWRPB           basePA=0x2000              size=0x4000
```
PALcode size and HWRPB size confirmed by reading HWRPB fields
(`phys_base`, memory descriptor offsets) post-decompress.

### Init — MemoryMapConfig
```
  GuestRegionType::RAM             basePA=ramBase   size=ramSize
  GuestRegionType::MMIO            basePA=mmioBase  size=mmioSize
  GuestRegionType::PCI             basePA=pciBase   size=pciSize
```

### Validation gate
Before Phase 14c begins, `registry.validate()` confirms no overlaps.
Any overlap is a hard failure — log and halt.

---

## 6. GuestMemory Retrofit Requirements

`GuestMemory` currently uses a flat allocation model with no awareness of
region types. The following changes are required:

### 6.1 Registry reference
`GuestMemory` holds a reference (or pointer) to the shared
`GuestPhysicalRegionRegistry`. It does not own it — the registry lives at
the emulator top level (e.g. `EmulatR_init` or a new `PlatformState` struct).

### 6.2 Write validation (debug builds only)
On `writePhysical()`, if `AXP_DEBUG` is defined, check that the target PA
falls within a registered region. Log a warning if writing to `Reserved`
or `Unknown` space. This catches decompressor writes to unexpected addresses
early.

### 6.3 Read-only enforcement
`GuestMemory::writePhysical()` checks `region.readOnly` and returns early
(with error log) if the target region is marked read-only. ROM regions
(`DecompressedFW` after Phase 14b completes) should be marked read-only
before Phase 14c begins.

### 6.4 Snapshot integration
`saveSnapshot()` and `loadSnapshot()` call `registry.snapshotRegions()`
to get the region list dynamically, replacing the hardcoded
`std::vector<SrmSnapshotMemRegion>` passed from `EmulatR_init.cpp`.

### 6.5 HWRPB memory descriptor
A new `buildHwrpbMemoryDescriptors()` function uses `registry.hwrpbRegions()`
to construct the Alpha HWRPB MDP (memory descriptor page) entries,
replacing any hardcoded or INI-driven memory sizing.

---

## 7. INI Entries Eliminated by This Design

| INI Key | Replacement |
|---------|-------------|
| `SrmInitialPC` | `SrmRomDescriptor::finalPC` (LDA/JSR scan) |
| `SrmDonePC` | `SrmRomDescriptor::donePC()` |
| `SrmMirrorPA` | Always `0x0` by convention |
| `SrmSize` | `payloadSize` from file load |
| `SrmRomVariant` | `sys_type` from HWRPB post-decompress |
| `HwrpbBase` | Architecture constant `0x2000` |
| `HwrpbSize` | Read from HWRPB `mdp_size` field post-decompress |

---

## 8. Remaining INI Entries (irreducible)

| INI Key | Reason |
|---------|--------|
| `SrmBase` | Platform topology — where stub loads in guest PA space |
| `SrmRomFile` | User-specified firmware path |
| `SrmSnapshot` | User preference |
| `RamBase` | Platform topology |
| `MemorySizeGB` | System configuration |
| `MmioBase` / `MmioSize` | Platform topology |
| `PciMemBase` / `PciMemSize` | Platform topology |

---

## 9. Dependencies and Sequencing

```
loadFromFile()
    └─ populateDescriptor()
        └─ SrmRomDescriptor.regions (seed entries)
            └─ registry.seedFromDescriptor()
                └─ Phase 14b decompress()
                    └─ registry.add(PALcode)
                    └─ registry.add(HWRPB)
                        └─ registry.validate()
                            └─ Phase 14c (CPU execution begins)
```

`GuestMemory` retrofit is not required before Phase 14c. The registry
can coexist with the current flat memory model during the transition.
Write validation and read-only enforcement are additive and can be
enabled incrementally behind `#ifdef AXP_DEBUG` guards.

---

## 10. Open Questions

1. **PALcode region size** — `PAL_BASE=0x600000` but upper bound unknown
   until HWRPB MDP analysis is complete. Placeholder size `0x200000` (2MB)
   is conservative. Confirm from HWRPB session.

2. **RAM ceiling** — does RAM start at `RamBase=0x80000000` or is there a
   lower RAM region below the firmware? Alpha systems typically have a hole
   from `0x400000` to `RamBase`. Confirm from HWRPB MDP entries.

3. **Snapshot for RAM** — including full RAM in snapshots makes them very
   large. Consider a flag `SrmSnapshotIncludeRam=false` (default) with
   opt-in for full state capture.

4. **Registry ownership** — lives in `EmulatR_init`, passed by reference to
   `SrmRomLoader`, `GuestMemory`, and future device subsystems. Alternatively
   a singleton via `global_EmulatR_init()` pattern already used in the project.

5. **Thread safety** — registry is written during init (single-threaded) and
   read during execution (potentially multi-CPU). A `QReadWriteLock` or
   `std::shared_mutex` will be needed once SMP is introduced.

---

## 11. Files to Create / Modify

| File | Action |
|------|--------|
| `GuestPhysicalRegionRegistry.h` | New — struct + class definitions |
| `GuestPhysicalRegionRegistry.cpp` | New — implementation |
| `SrmRomLoader.h` | Done — `GuestPhysicalRegion` stub + `SrmRomDescriptor::regions` |
| `SrmRomLoader.cpp` | Done — `populateDescriptor()` seeds two entries |
| `GuestMemory.h` | Retrofit — registry reference, write validation |
| `GuestMemory.cpp` | Retrofit — region-aware read/write, snapshot integration |
| `EmulatR_init.cpp` | Modify — registry instantiation, population sequence |
| `MemoryMapConfig` | Modify — remove obsolete fields, add registry seed calls |
| `EmulatR.ini` | Modify — remove obsolete keys per Section 7 |

---

*End of specification.*
