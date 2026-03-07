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
//        INI: SrmBase=0x900000, SrmInitialPC=0x900001
//
//   B) Pre-decompressed flat .bin (QEMU export)
//        Binary copied to SrmBase, CPU starts at SrmInitialPC directly.
//        INI: SrmBase=0x0, SrmInitialPC=0x8001
//
//   C) Snapshot (fastest -- bypasses decompression entirely)
//        loadSnapshot() restores guest memory + full CPU state.
//        saveSnapshot() captures state at the done: point in decompress().
//        INI: SrmSnapshot=path/to/snapshot.axpsnap
//
// Snapshot file format (.axpsnap):
//   [Header]   magic(8) + version(4) + romHash(8) + finalPC(8) + palBase(8)
//   [Memory]   regionCount(4) + { pa(8) + size(8) + data }...
//   [IntRegs]  32 x quint64
//   [FpRegs]   32 x quint64
//   [IPRs]     count(4) + { id(4) + value(8) }...
//   [Footer]   checksum(8)
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

// ============================================================================
// SrmLoaderConfig
// ============================================================================

struct SrmLoaderConfig
{
    quint64  loadPA      = 0x900000;
    quint64  startPC     = 0x900001;
    quint64  palBase     = 0x900000;
    quint64  donePC      = 0x200000;
    quint64  mirrorPA    = 0x000000;
    int      maxSteps    = 200000000;

    static SrmLoaderConfig fromSettings(const class QSettings& s);
    static SrmLoaderConfig defaultExe() { return {}; }

    bool isValid(QString* errorOut = nullptr) const
    {
        if (startPC != (loadPA | 1ULL)) {
            if (errorOut)
                *errorOut = QString("SrmInitialPC 0x%1 != SrmBase 0x%2 | PAL bit")
                                .arg(startPC, 0, 16).arg(loadPA, 0, 16);
            return false;
        }
        if (loadPA == mirrorPA) {
            if (errorOut)
                *errorOut = QString("SrmBase and mirrorPA both 0x%1 -- would alias")
                                .arg(loadPA, 0, 16);
            return false;
        }
        return true;
    }
};

// ============================================================================
// SrmRomLoadResult
// ============================================================================

struct SrmRomLoadResult
{
    bool     success          = false;
    quint64  finalPC          = 0;
    quint64  finalPalBase     = 0;
    quint64  cyclesExecuted   = 0;
    double   elapsedMs        = 0.0;
    size_t   headerSkip       = 0;

    // Phase timing
    quint64  initSteps        = 0;
    quint64  copyLoopSteps    = 0;
    quint64  postCopySteps    = 0;
    double   copyLoopMs       = 0.0;

    // Snapshot metadata
    bool     fromSnapshot     = false;
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
    static constexpr quint8  kDecompSig[12] = {
        0x04, 0x04, 0x3F, 0x44,
        0x05, 0x04, 0x5F, 0x44,
        0x0E, 0x04, 0x9F, 0x47
    };
    static constexpr size_t  kDecompSigLen    = sizeof(kDecompSig);
    static constexpr size_t  kMaxHeaderScan   = 0x1000;
    static constexpr quint64 kCopyLoopPC      = 0x9002AC;
    static constexpr quint64 kCopyExitPC      = 0x9002CC;
    static constexpr quint64 kSnapshotMagic   = 0x415850534E415001ULL;
    static constexpr quint32 kSnapshotVersion = 1;

    SrmRomLoader() = default;

    bool useEmbedded();
    bool loadFromFile(const QString& filePath);

    bool    isLoaded()    const { return m_payloadData != nullptr && m_payloadSize > 0; }
    size_t  romSize()     const { return m_romSize; }
    size_t  headerSkip()  const { return m_headerSkip; }
    size_t  payloadSize() const { return m_payloadSize; }

    // -- Callback types ------------------------------------------------------

    using WritePhysicalFn = std::function<void(quint64 pa, const quint8* data, size_t len)>;
    using ReadPhysicalFn  = std::function<void(quint64 pa, quint8* buf,        size_t len)>;
    using SingleStepFn    = std::function<quint64()>;
    using SetU64Fn        = std::function<void(quint64)>;
    using GetU64Fn        = std::function<quint64()>;
    using ProgressFn      = std::function<void(int percent)>;
    using GetIntRegsFn    = std::function<void(quint64 regs[32])>;
    using SetIntRegsFn    = std::function<void(const quint64 regs[32])>;
    using GetFpRegsFn     = std::function<void(quint64 regs[32])>;
    using SetFpRegsFn     = std::function<void(const quint64 regs[32])>;
    using IprPair         = std::pair<quint32, quint64>;
    using GetIprsFn       = std::function<std::vector<IprPair>()>;
    using SetIprsFn       = std::function<void(const std::vector<IprPair>&)>;

    // -- Decompression (Load path A) -----------------------------------------

    AXP_FLATTEN SrmRomLoadResult decompress(
        const SrmLoaderConfig& cfg,
        WritePhysicalFn        writeToPhysical,
        SingleStepFn           singleStep,
        SetU64Fn               setPC,
        SetU64Fn               setPalBase,
        GetU64Fn               getPalBase,
        ProgressFn             progress = nullptr
    );

    // -- Snapshot save (call after decompress() succeeds) --------------------
    //
    // regions: list of PA ranges to include -- typically:
    //   { {0x0, 0x400000}, {0x900000, 0x2AB800} }
    //   (decompressed firmware + PALcode space)
    //
    bool saveSnapshot(
        const QString&                           path,
        const SrmRomLoadResult&                  result,
        const std::vector<SrmSnapshotMemRegion>& regions,
        ReadPhysicalFn                           readFromPhysical,
        GetIntRegsFn                             getIntRegs,
        GetFpRegsFn                              getFpRegs,
        GetIprsFn                                getIPRs
    );

    // -- Snapshot load (bypasses decompress() entirely) ----------------------
    //
    // Returns SrmRomLoadResult with fromSnapshot=true on success.
    // Caller should check result.success before proceeding.
    //
    SrmRomLoadResult loadSnapshot(
        const QString&   path,
        WritePhysicalFn  writeToPhysical,
        SetU64Fn         setPC,
        SetU64Fn         setPalBase,
        SetIntRegsFn     setIntRegs,
        SetFpRegsFn      setFpRegs,
        SetIprsFn        setIPRs
    );

private:
    const quint8*  m_payloadData = nullptr;
    size_t         m_payloadSize = 0;
    size_t         m_romSize     = 0;
    size_t         m_headerSkip  = 0;
    QByteArray     m_fileBuffer;

    static qint64  findDecompressor(const quint8* data, size_t size);
    static quint64 checksum64(const quint8* data, size_t len) noexcept;
};

#endif // SRMROMLOADER_H
