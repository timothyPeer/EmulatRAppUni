// ============================================================================
// SrmRomLoader.h -- Alpha SRM Firmware ROM Loader
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025, 2026 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// ============================================================================
//
// Load paths:
//
//   A) Self-decompressing .EXE
//        decompress() runs the decompressor on the guest CPU.
//        Firmware lands at PA 0x0. Takes ~16 min in debug build.
//        INI: SrmBase=0x900000   (only platform topology value required)
//
//   B) Pre-decompressed flat .bin (QEMU export)
//        Binary copied to SrmBase, CPU starts at SrmInitialPC directly.
//        INI: SrmBase=0x0, SrmInitialPC=0x8001
//
//   C) Snapshot (fastest -- bypasses decompression entirely)
//        loadSnapshot() restores guest memory + full CPU state.
//        saveSnapshot() captures state at the done: point in decompress().
//        INI: SrmSnapshot=true
//
// SrmRomDescriptor (auto-populated from firmware binary):
//   All boot parameters are derived from the decompressor stub at load time.
//   No INI entries are required beyond SrmBase, SrmRomFile, and SrmSnapshot.
//   The following INI entries are OBSOLETE and silently ignored if present:
//     SrmInitialPC, SrmDonePC, SrmMirrorPA, SrmSize, SrmRomVariant
//
// Stub analysis -- LDA/JSR pair scan:
//   findDecompressor() locates stub entry via 12-byte signature scan.
//   populateDescriptor() scans the stub for the validated LDA/JSR pair:
//     LDA R0, disp(R26)  -- opcode 0x08, Ra=R0, Rb=R26, disp16=finalPC
//     JSR R31, (R0)      -- 0x6BE04000, always immediately follows LDA
//   Scan offset varies between firmware versions -- detection is version-agnostic.
//   PAL_BASE read from stub+0x10 (embedded by DEC/Compaq/HP at firmware build time).
//
//   Validation guards:
//     1. LDA opcode/Ra/Rb must match kLdaPattern exactly
//     2. finalPC candidate must be > 0 and < loadPA (in decompressed image)
//     3. JSR must immediately follow LDA (offset + 4)
//
// Snapshot file format (.axpsnap):
//   [Header]   magic(8) + version(4) + romHash(8) + finalPC(8) + palBase(8)
//              + cycles(8) + elapsedMs(8)
//   [Memory]   regionCount(4) + { pa(8) + size(8) + data }...
//   [IntRegs]  32 x quint64
//   [FpRegs]   32 x quint64
//   [IPRs]     count(4) + { id(4) + value(8) }...
//   [Footer]   checksum(8)  FNV-1a over all preceding bytes
//
// Attribution:
//   Decompression algorithm from ES40 Emulator (Camiel Vanderhoeven, GPL v2)
//   Firmware binaries property of DEC / Compaq / HP
// ============================================================================

#ifndef SRMROMLOADER_H
#define SRMROMLOADER_H

#include <QtTypes>
#include <QString>
#include <QElapsedTimer>
#include <functional>
#include <vector>

#include "coreLib/Axp_Attributes_core.h"
#include "GuestPhysicalRegion.h"    // GuestRegionType, GuestRegionSource, GuestPhysicalRegion

// ============================================================================
// SrmRomDescriptor
// ============================================================================
// Auto-populated from the firmware binary by loadFromFile() / useEmbedded().
// All values derived from decompressor stub analysis -- no INI required.
// ============================================================================

struct SrmRomDescriptor
{
    bool     valid = false;

    // -- Payload geometry ----------------------------------------------------
    size_t   sigOffset = 0;        // byte offset of decompressor sig in raw file
    size_t   headerSkip = 0;        // == sigOffset (0 for .EXE, 0x200 for .SYS)
    size_t   payloadSize = 0;        // rawFileSize - headerSkip

    // -- Boot values (from validated LDA/JSR pair scan) ----------------------
    quint64  palBase = 0;        // stub+0x10: PAL_BASE embedded at firmware build time
    quint64  finalPC = 0;        // LDA disp16: SRM console entry point (even address)
    size_t   jsrOffset = 0;        // stub offset where JSR was found (diagnostic)

    // Derived at runtime in decompress() -- require loadPA from SrmLoaderConfig:
    //   startPC  = loadPA + sigOffset + 1   (PAL mode bit set)
    //   donePC   = finalPC + 0x40           (safe margin above entry, below loadPA)
    //   mirrorPA = 0x0                      (Alpha AXP convention, always)

    // -- Phase detection offsets (relative to sigOffset, cosmetic only) ------
    // Valid for all ES40/ES45 V6.2-V7.2 variants (byte-identical stub layout).
    // Phase detection does NOT affect decompression correctness -- step counts only.
    quint64  copyLoopOff = 0x3EC;    // tight copy loop entry offset from sigOffset
    quint64  copyExitOff = 0x408;    // tight copy loop exit  offset from sigOffset

    // -- Source identification -----------------------------------------------
    QString  sourceDescription;         // e.g. "ES40_V6_2.EXE" or "embedded ES45 V6.2"

    // -- Derived helpers (require loadPA from SrmLoaderConfig) ---------------
    quint64 startPC(quint64 loadPA) const { return loadPA + static_cast<quint64>(sigOffset) + 1; }
    quint64 donePC()                const { return finalPC + 0x40; }

    // -- Guest PA region map -------------------------------------------------
    // Seed entries representing regions written to guest PA space during
    // decompression. Feed directly into saveSnapshot() regions vector and
    // the GuestPhysicalRegion registry.
    std::vector<GuestPhysicalRegion> regions;
};

// ============================================================================
// SrmLoaderConfig
// ============================================================================
// INI-driven values only. Everything derivable from the firmware binary has
// moved to SrmRomDescriptor, populated automatically at load time.
//
// Required INI entry:
//   [MemoryMap]
//   SrmBase=0x900000         ; where decompressor stub loads in guest PA space
//                            ; must be above decompressed image ceiling (0x400000)
//                            ; and below RAM base -- platform topology, not derivable
//
// Optional INI entry:
//   SrmMaxSteps=200000000    ; decompressor step limit safety valve
//
// Obsolete INI entries (silently ignored if present):
//   SrmInitialPC  -- derived: LDA disp16 at JSR-4 in stub
//   SrmDonePC     -- derived: finalPC + 0x40
//   SrmMirrorPA   -- always 0x0 by Alpha AXP convention
//   SrmSize       -- derived: rawFileSize - headerSkip
//   SrmRomVariant -- obsolete: platform ID from HWRPB sys_type post-decompress
// ============================================================================

struct SrmLoaderConfig
{
    quint64  loadPA = 0x900000;    // SrmBase -- decompressor stub load address
    int      maxSteps = 200000000;   // safety limit on decompressor step count

    static SrmLoaderConfig fromSettings(const class QSettings& s);
    static SrmLoaderConfig defaultExe() { return {}; }
};

// ============================================================================
// SrmRomLoadResult
// ============================================================================

struct SrmRomLoadResult
{
    bool     success = false;
    quint64  finalPC = 0;
    quint64  finalPalBase = 0;
    quint64  cyclesExecuted = 0;
    double   elapsedMs = 0.0;
    size_t   headerSkip = 0;

    // Phase timing (cosmetic -- step counts per decompressor phase)
    quint64  initSteps = 0;
    quint64  copyLoopSteps = 0;
    quint64  postCopySteps = 0;
    double   copyLoopMs = 0.0;

    // Snapshot metadata
    bool     fromSnapshot = false;
    QString  snapshotPath;

    QString  errorMessage;

    quint64 bootPC()    const { return finalPC; }
    quint64 cleanPC()   const { return finalPC & ~1ULL; }
    bool    isPalMode() const { return (finalPC & 1ULL) != 0; }
    quint64 palBase()   const { return finalPalBase; }
};

// ============================================================================
// SrmSnapshotMemRegion -- physical memory region to include in snapshot
// ============================================================================

struct SrmSnapshotMemRegion
{
    quint64  basePA;
    quint64  size;
};

// ============================================================================
// SrmRomLoader
// ============================================================================

class SrmRomLoader final
{
public:
    // Decompressor signature -- first 12 bytes of every known EV6 SRM stub
    static constexpr quint8  kDecompSig[12] = {
        0x04, 0x04, 0x3F, 0x44,   // BIS R1,  R31, R4
        0x05, 0x04, 0x5F, 0x44,   // BIS R2,  R31, R5
        0x0E, 0x04, 0x9F, 0x47    // BIS R28, R31, R14
    };
    static constexpr size_t  kDecompSigLen = sizeof(kDecompSig);
    static constexpr size_t  kMaxHeaderScan = 0x1000;

    // LDA/JSR pair constants for finalPC detection (version-agnostic scan)
    static constexpr quint32 kLdaMask = 0xFFFF0000;  // opcode+Ra+Rb mask
    static constexpr quint32 kLdaPattern = 0x201A0000;  // LDA R0, disp(R26)
    static constexpr quint32 kJsrToFinalPC = 0x6BE04000;  // JSR R31, (R0)
    static constexpr size_t  kJsrScanLimit = 0x1000;      // scan first 4KB only

    // Snapshot format constants
    static constexpr quint64 kSnapshotMagic = 0x415850534E415001ULL;
    static constexpr quint32 kSnapshotVersion = 1;

    SrmRomLoader() = default;

    // -- Load ROM image ------------------------------------------------------
    // loadPA required for finalPC range validation in populateDescriptor().
    bool useEmbedded(quint64 loadPA = 0x900000);
    bool loadFromFile(const QString& filePath, quint64 loadPA = 0x900000);

    // -- State queries -------------------------------------------------------
    bool                    isLoaded()    const { return m_payloadData != nullptr && m_payloadSize > 0; }
    size_t                  romSize()     const { return m_romSize; }
    size_t                  headerSkip()  const { return m_headerSkip; }
    size_t                  payloadSize() const { return m_payloadSize; }
    const SrmRomDescriptor& descriptor()  const { return m_descriptor; }

    // -- Callback types ------------------------------------------------------

    using WritePhysicalFn = std::function<void(quint64 pa, const quint8* data, size_t len)>;
    using ReadPhysicalFn = std::function<void(quint64 pa, quint8* buf, size_t len)>;
    using SingleStepFn = std::function<quint64()>;
    using SetU64Fn = std::function<void(quint64)>;
    using GetU64Fn = std::function<quint64()>;
    using ProgressFn = std::function<void(int percent)>;
    using GetIntRegsFn = std::function<void(quint64 regs[32])>;
    using SetIntRegsFn = std::function<void(const quint64 regs[32])>;
    using GetFpRegsFn = std::function<void(quint64 regs[32])>;
    using SetFpRegsFn = std::function<void(const quint64 regs[32])>;
    using IprPair = std::pair<quint32, quint64>;
    using GetIprsFn = std::function<std::vector<IprPair>()>;
    using SetIprsFn = std::function<void(const std::vector<IprPair>&)>;

    // -- Decompression (Load path A) -----------------------------------------
    // cfg supplies loadPA and maxSteps only.
    // All other boot parameters taken from m_descriptor (populated at load time).

    AXP_FLATTEN SrmRomLoadResult decompress(
        const SrmLoaderConfig& cfg,
        WritePhysicalFn        writeToPhysical,
        SingleStepFn           singleStep,
        SetU64Fn               setPC,
        SetU64Fn               setPalBase,
        GetU64Fn               getPalBase,
        ProgressFn             progress = nullptr
    );

    // -- Snapshot save -------------------------------------------------------
    bool saveSnapshot(
        const QString& path,
        const SrmRomLoadResult& result,
        const std::vector<SrmSnapshotMemRegion>& regions,
        ReadPhysicalFn                           readFromPhysical,
        GetIntRegsFn                             getIntRegs,
        GetFpRegsFn                              getFpRegs,
        GetIprsFn                                getIPRs
    );

    // -- Snapshot load -------------------------------------------------------
    SrmRomLoadResult loadSnapshot(
        const QString& path,
        WritePhysicalFn  writeToPhysical,
        SetU64Fn         setPC,
        SetU64Fn         setPalBase,
        SetIntRegsFn     setIntRegs,
        SetFpRegsFn      setFpRegs,
        SetIprsFn        setIPRs
    );

private:
    const quint8* m_payloadData = nullptr;
    size_t           m_payloadSize = 0;
    size_t           m_romSize = 0;
    size_t           m_headerSkip = 0;
    QByteArray       m_fileBuffer;
    SrmRomDescriptor m_descriptor;

    static qint64  findDecompressor(const quint8* data, size_t size);
    static quint64 checksum64(const quint8* data, size_t len) noexcept;

    // Populate m_descriptor from stub LDA/JSR scan and PAL_BASE read.
    // loadPA required for finalPC candidate range validation.
    bool populateDescriptor(const QString& sourceDescription, quint64 loadPA);
};

#endif // SRMROMLOADER_H