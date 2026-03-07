// ============================================================================
// SrmRomLoader.cpp -- Alpha SRM Firmware ROM Loader
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025, 2026 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// ============================================================================

#include "SrmRomLoader.h"
#include "coreLib/LoggingMacros.h"
#include "coreLib/cpuTrace.h"
#include <QFile>
#include <QSettings>
#include <QDataStream>
#include <cstring>

#define COMPONENT_NAME "SrmRomLoader"

#ifdef USE_EMBEDDED_SRM
#  include "romLib/SrmRomData_ES45.inc"
#endif

// ============================================================================
// SrmLoaderConfig::fromSettings
// ============================================================================

// Helper: read a quint64 from QSettings, supporting hex (0x...) and decimal strings
static quint64 readU64(const QSettings& s, const QString& key, quint64 defaultVal)
{
    const QVariant v = s.value(key);
    if (!v.isValid())
        return defaultVal;
    // Route through QString so toULongLong(nullptr, 0) auto-detects 0x prefix
    bool ok = false;
    const quint64 result = v.toString().toULongLong(&ok, 0);
    return ok ? result : defaultVal;
}

SrmLoaderConfig SrmLoaderConfig::fromSettings(const QSettings& s)
{
    SrmLoaderConfig cfg;
    cfg.loadPA   = readU64(s, "MemoryMap/SrmBase",      cfg.loadPA  );
    cfg.startPC  = readU64(s, "MemoryMap/SrmInitialPC", cfg.startPC );
    cfg.donePC   = readU64(s, "MemoryMap/SrmDonePC",    cfg.donePC  );
    cfg.mirrorPA = readU64(s, "MemoryMap/SrmMirrorPA",  cfg.mirrorPA);
    cfg.maxSteps = s.value("MemoryMap/SrmMaxSteps", cfg.maxSteps).toInt();
    cfg.palBase  = cfg.loadPA;
    return cfg;
}

// ============================================================================
// checksum64 -- simple FNV-1a 64-bit checksum
// ============================================================================

quint64 SrmRomLoader::checksum64(const quint8* data, size_t len) noexcept
{
    quint64 hash = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; ++i) {
        hash ^= static_cast<quint64>(data[i]);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

// ============================================================================
// findDecompressor
// ============================================================================

qint64 SrmRomLoader::findDecompressor(const quint8* data, size_t size)
{
    if (!data || size < kDecompSigLen)
        return -1;
    const size_t searchLimit = qMin(size - kDecompSigLen, kMaxHeaderScan);
    for (size_t offset = 0; offset <= searchLimit; offset += 4) {
        if (std::memcmp(data + offset, kDecompSig, kDecompSigLen) == 0)
            return static_cast<qint64>(offset);
    }
    return -1;
}

// ============================================================================
// useEmbedded
// ============================================================================

bool SrmRomLoader::useEmbedded()
{
#ifndef USE_EMBEDDED_SRM
    INFO_LOG("USE_EMBEDDED_SRM not set -- embedded ROM not compiled in");
    return false;
#else
    const qint64 sigOffset = findDecompressor(kES45SrmRomData, kES45SrmRomSize);
    if (sigOffset < 0) {
        ERROR_LOG("Decompressor signature not found in embedded ES45 ROM");
        return false;
    }
    m_romSize     = kES45SrmRomSize;
    m_headerSkip  = static_cast<size_t>(sigOffset);
    m_payloadData = kES45SrmRomData + m_headerSkip;
    m_payloadSize = kES45SrmRomSize - m_headerSkip;
    m_fileBuffer.clear();
    INFO_LOG(QString("Embedded ES45 V6.2 -- %1 bytes, header skip 0x%2, payload %3 bytes")
        .arg(m_romSize).arg(m_headerSkip, 0, 16).arg(m_payloadSize));
    return true;
#endif
}

// ============================================================================
// loadFromFile
// ============================================================================

bool SrmRomLoader::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        ERROR_LOG(QString("Cannot open ROM file: %1").arg(filePath));
        return false;
    }
    m_fileBuffer       = file.readAll();
    file.close();

    const auto*  raw     = reinterpret_cast<const quint8*>(m_fileBuffer.constData());
    const size_t rawSize = static_cast<size_t>(m_fileBuffer.size());

    const qint64 sigOffset = findDecompressor(raw, rawSize);
    if (sigOffset < 0) {
        ERROR_LOG(QString("No EV6 decompressor signature in '%1'").arg(filePath));
        m_fileBuffer.clear();
        return false;
    }

    m_romSize     = rawSize;
    m_headerSkip  = static_cast<size_t>(sigOffset);
    m_payloadData = raw + m_headerSkip;
    m_payloadSize = rawSize - m_headerSkip;

    INFO_LOG(QString("Loaded '%1' -- %2 bytes, header skip 0x%3, payload %4 bytes")
        .arg(filePath).arg(m_romSize).arg(m_headerSkip, 0, 16).arg(m_payloadSize));
    return true;
}

// ============================================================================
// decompress
// ============================================================================

SrmRomLoadResult SrmRomLoader::decompress(
    const SrmLoaderConfig& cfg,
    WritePhysicalFn        writeToPhysical,
    SingleStepFn           singleStep,
    SetU64Fn               setPC,
    SetU64Fn               setPalBase,
    GetU64Fn               getPalBase,
    ProgressFn             progress)
{
    SrmRomLoadResult result;
    result.headerSkip = m_headerSkip;

    if (!isLoaded()) {
        result.errorMessage = "No ROM image loaded";
        ERROR_LOG(result.errorMessage);
        return result;
    }
    if (!writeToPhysical || !singleStep || !setPC || !setPalBase || !getPalBase) {
        result.errorMessage = "Missing required callback(s)";
        ERROR_LOG(result.errorMessage);
        return result;
    }

    QString cfgError;
    if (!cfg.isValid(&cfgError)) {
        result.errorMessage = QString("Invalid SrmLoaderConfig: %1").arg(cfgError);
        ERROR_LOG(result.errorMessage);
        return result;
    }

    INFO_LOG("=== SRM Decompression starting ===");
    INFO_LOG(QString("  ROM payload : %1 bytes").arg(m_payloadSize));
    INFO_LOG(QString("  Mirror PA   : 0x%1").arg(cfg.mirrorPA, 0, 16));
    INFO_LOG(QString("  Load PA     : 0x%1").arg(cfg.loadPA,   0, 16));
    INFO_LOG(QString("  Start PC    : 0x%1").arg(cfg.startPC,  0, 16));
    INFO_LOG(QString("  Done when PC < 0x%1").arg(cfg.donePC,  0, 16));

    writeToPhysical(cfg.mirrorPA, m_payloadData, m_payloadSize);
    writeToPhysical(cfg.loadPA,   m_payloadData, m_payloadSize);
    setPC(cfg.startPC);
    setPalBase(cfg.palBase);

    QElapsedTimer timer;
    timer.start();
    CpuTrace::startElapsedTime();

    quint64 stepCount   = 0;
    quint64 currentPC   = cfg.startPC;
    int     lastPercent = -1;

    enum class Phase { Init, CopyLoop, PostCopy } phase = Phase::Init;
    quint64 copyLoopEnterStep = 0;
    qint64  copyLoopEnterMs   = 0;

    INFO_LOG(QString("[T+00:00:00.000] Decompressor started -- PC=0x%1, limit=%2 steps")
        .arg(cfg.startPC, 0, 16).arg(cfg.maxSteps));

    const quint64 kBatchSize  = 1800000ULL;
    const int     kMaxBatches = cfg.maxSteps / static_cast<int>(kBatchSize);

    for (int batch = 0; batch < kMaxBatches; ++batch)
    {
        for (quint64 i = 0; i < kBatchSize; ++i)
        {
            currentPC = singleStep();
            ++stepCount;

            if ((stepCount % 20) == 0)
                CpuTrace::flush();

            const quint64 cleanPC = currentPC & ~1ULL;

            // -- Phase transitions -------------------------------------------
            if (phase == Phase::Init && cleanPC == kCopyLoopPC)
            {
                phase             = Phase::CopyLoop;
                copyLoopEnterStep = stepCount;
                copyLoopEnterMs   = timer.elapsed();
                INFO_LOG(QString("[T+%1] Copy loop entered -- step %2")
                    .arg(CpuTrace::elapsedTime()).arg(stepCount));
            }
            else if (phase == Phase::CopyLoop && cleanPC == kCopyExitPC)
            {
                phase                = Phase::PostCopy;
                result.copyLoopSteps = stepCount - copyLoopEnterStep;
                result.copyLoopMs    = static_cast<double>(timer.elapsed() - copyLoopEnterMs);
                result.initSteps     = copyLoopEnterStep;
                INFO_LOG(QString("[T+%1] Copy loop complete -- %2 steps in %3 ms")
                    .arg(CpuTrace::elapsedTime())
                    .arg(result.copyLoopSteps)
                    .arg(result.copyLoopMs, 0, 'f', 1));
            }

            if (cleanPC < cfg.donePC)
                goto done;
        }

        // Progress between batches
        int percent = qMin(99, (batch + 1) * 2);
        if (progress && percent != lastPercent) {
            progress(percent);
            lastPercent = percent;
        }
    }

    result.errorMessage = QString("Decompression stalled after %1 steps (PC=0x%2)")
                              .arg(stepCount).arg(currentPC, 0, 16);
    ERROR_LOG(result.errorMessage);
    return result;

done:
    result.postCopySteps  = stepCount - (result.initSteps + result.copyLoopSteps);
    result.success        = true;
    result.finalPC        = currentPC;
    result.finalPalBase   = getPalBase();
    result.cyclesExecuted = stepCount;
    result.elapsedMs      = static_cast<double>(timer.elapsed());

    CpuTrace::writeElapsedToMachineLine();
    if (progress) progress(100);

    INFO_LOG(QString("[T+%1] Decompression complete").arg(CpuTrace::elapsedTime()));
    INFO_LOG(QString("  Final PC    : 0x%1").arg(result.finalPC,      0, 16));
    INFO_LOG(QString("  PAL_BASE    : 0x%1").arg(result.finalPalBase, 0, 16));
    INFO_LOG(QString("  Total steps : %1").arg(result.cyclesExecuted));
    INFO_LOG(QString("  Init steps  : %1").arg(result.initSteps));
    INFO_LOG(QString("  Copy steps  : %1 (%2 ms)").arg(result.copyLoopSteps).arg(result.copyLoopMs, 0, 'f', 1));
    INFO_LOG(QString("  Post steps  : %1").arg(result.postCopySteps));

    return result;
}

// ============================================================================
// saveSnapshot
// ============================================================================

bool SrmRomLoader::saveSnapshot(
    const QString&                           path,
    const SrmRomLoadResult&                  result,
    const std::vector<SrmSnapshotMemRegion>& regions,
    ReadPhysicalFn                           readFromPhysical,
    GetIntRegsFn                             getIntRegs,
    GetFpRegsFn                              getFpRegs,
    GetIprsFn                                getIPRs)
{
    if (!result.success) {
        ERROR_LOG("saveSnapshot: result is not successful -- nothing to save");
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        ERROR_LOG(QString("saveSnapshot: cannot open '%1' for writing").arg(path));
        return false;
    }

    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setVersion(QDataStream::Qt_6_0);

    // -- Header --------------------------------------------------------------
    quint64 romHash = m_payloadData
        ? checksum64(m_payloadData, m_payloadSize)
        : 0ULL;

    ds << kSnapshotMagic;
    ds << kSnapshotVersion;
    ds << romHash;
    ds << result.finalPC;
    ds << result.finalPalBase;
    ds << result.cyclesExecuted;
    ds << result.elapsedMs;

    INFO_LOG(QString("saveSnapshot: writing to '%1'").arg(path));
    INFO_LOG(QString("  Boot PC   : 0x%1").arg(result.finalPC,    0, 16));
    INFO_LOG(QString("  PAL_BASE  : 0x%1").arg(result.finalPalBase, 0, 16));
    INFO_LOG(QString("  ROM hash  : 0x%1").arg(romHash,           0, 16));

    // -- Memory regions ------------------------------------------------------
    ds << static_cast<quint32>(regions.size());

    quint64 totalBytes = 0;
    for (const auto& region : regions)
    {
        ds << region.basePA;
        ds << region.size;

        QByteArray buf(static_cast<qsizetype>(region.size), '\0');
        readFromPhysical(region.basePA,
                         reinterpret_cast<quint8*>(buf.data()),
                         region.size);
        ds.writeRawData(buf.constData(), static_cast<int>(region.size));
        totalBytes += region.size;

        INFO_LOG(QString("  Region PA=0x%1  size=0x%2 (%3 bytes)")
            .arg(region.basePA, 0, 16)
            .arg(region.size,   0, 16)
            .arg(region.size));
    }

    // -- Integer registers ---------------------------------------------------
    quint64 intRegs[32] = {};
    getIntRegs(intRegs);
    for (int i = 0; i < 32; ++i)
        ds << intRegs[i];

    // -- FP registers --------------------------------------------------------
    quint64 fpRegs[32] = {};
    getFpRegs(fpRegs);
    for (int i = 0; i < 32; ++i)
        ds << fpRegs[i];

    // -- IPRs ----------------------------------------------------------------
    auto iprs = getIPRs();
    ds << static_cast<quint32>(iprs.size());
    for (const auto& [id, val] : iprs) {
        ds << id;
        ds << val;
    }

    // -- Footer checksum -----------------------------------------------------
    // Checksum over the entire file content written so far
    file.flush();
    const qint64 fileSize = file.pos();
    file.seek(0);
    QByteArray allData = file.read(fileSize);
    quint64 cksum = checksum64(
        reinterpret_cast<const quint8*>(allData.constData()),
        static_cast<size_t>(allData.size()));
    file.seek(fileSize);
    ds << cksum;

    file.close();

    INFO_LOG(QString("saveSnapshot: saved %1 bytes total memory, file size %2 bytes")
        .arg(totalBytes).arg(file.size()));
    INFO_LOG(QString("  Checksum  : 0x%1").arg(cksum, 0, 16));

    return true;
}

// ============================================================================
// loadSnapshot
// ============================================================================

SrmRomLoadResult SrmRomLoader::loadSnapshot(
    const QString&   path,
    WritePhysicalFn  writeToPhysical,
    SetU64Fn         setPC,
    SetU64Fn         setPalBase,
    SetIntRegsFn     setIntRegs,
    SetFpRegsFn      setFpRegs,
    SetIprsFn        setIPRs)
{
    SrmRomLoadResult result;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = QString("loadSnapshot: cannot open '%1'").arg(path);
        ERROR_LOG(result.errorMessage);
        return result;
    }

    QElapsedTimer timer;
    timer.start();

    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setVersion(QDataStream::Qt_6_0);

    // -- Validate header -----------------------------------------------------
    quint64 magic    = 0;
    quint32 version  = 0;
    quint64 romHash  = 0;
    quint64 finalPC  = 0;
    quint64 palBase  = 0;
    quint64 cycles   = 0;
    double  elapsed  = 0.0;

    ds >> magic >> version >> romHash >> finalPC >> palBase >> cycles >> elapsed;

    if (magic != kSnapshotMagic) {
        result.errorMessage = QString("loadSnapshot: bad magic in '%1' -- not an .axpsnap file").arg(path);
        ERROR_LOG(result.errorMessage);
        return result;
    }
    if (version != kSnapshotVersion) {
        result.errorMessage = QString("loadSnapshot: version mismatch -- file=%1, expected=%2")
                                  .arg(version).arg(kSnapshotVersion);
        ERROR_LOG(result.errorMessage);
        return result;
    }

    // Warn if ROM hash doesn't match current loaded payload
    if (m_payloadData) {
        quint64 currentHash = checksum64(m_payloadData, m_payloadSize);
        if (currentHash != romHash) {
            WARN_LOG(QString("loadSnapshot: ROM hash mismatch "
                             "(snapshot=0x%1, current=0x%2) -- snapshot may be stale")
                         .arg(romHash,      0, 16)
                         .arg(currentHash,  0, 16));
        }
    }

    INFO_LOG(QString("loadSnapshot: loading '%1'").arg(path));
    INFO_LOG(QString("  Boot PC   : 0x%1").arg(finalPC, 0, 16));
    INFO_LOG(QString("  PAL_BASE  : 0x%1").arg(palBase, 0, 16));

    // -- Memory regions ------------------------------------------------------
    quint32 regionCount = 0;
    ds >> regionCount;

    quint64 totalBytes = 0;
    for (quint32 r = 0; r < regionCount; ++r)
    {
        quint64 basePA = 0, size = 0;
        ds >> basePA >> size;

        QByteArray buf(static_cast<qsizetype>(size), '\0');
        ds.readRawData(buf.data(), static_cast<int>(size));
        writeToPhysical(basePA,
                        reinterpret_cast<const quint8*>(buf.constData()),
                        static_cast<size_t>(size));
        totalBytes += size;

        INFO_LOG(QString("  Region PA=0x%1  size=0x%2 (%3 bytes)")
            .arg(basePA, 0, 16).arg(size, 0, 16).arg(size));
    }

    // -- Integer registers ---------------------------------------------------
    quint64 intRegs[32] = {};
    for (int i = 0; i < 32; ++i)
        ds >> intRegs[i];
    setIntRegs(intRegs);

    // -- FP registers --------------------------------------------------------
    quint64 fpRegs[32] = {};
    for (int i = 0; i < 32; ++i)
        ds >> fpRegs[i];
    setFpRegs(fpRegs);

    // -- IPRs ----------------------------------------------------------------
    quint32 iprCount = 0;
    ds >> iprCount;
    std::vector<IprPair> iprs;
    iprs.reserve(iprCount);
    for (quint32 i = 0; i < iprCount; ++i) {
        quint32 id  = 0;
        quint64 val = 0;
        ds >> id >> val;
        iprs.emplace_back(id, val);
    }
    setIPRs(iprs);

    // -- Validate footer checksum --------------------------------------------
    quint64 storedCksum = 0;
    ds >> storedCksum;

    const qint64 footerPos = file.pos() - static_cast<qint64>(sizeof(quint64));
    file.seek(0);
    QByteArray allData = file.read(footerPos);
    quint64 computedCksum = checksum64(
        reinterpret_cast<const quint8*>(allData.constData()),
        static_cast<size_t>(allData.size()));

    if (computedCksum != storedCksum) {
        result.errorMessage = QString("loadSnapshot: checksum mismatch "
                                      "(stored=0x%1, computed=0x%2) -- file corrupt")
                                  .arg(storedCksum,   0, 16)
                                  .arg(computedCksum, 0, 16);
        ERROR_LOG(result.errorMessage);
        return result;
    }

    // -- Restore CPU state ---------------------------------------------------
    setPC(finalPC);
    setPalBase(palBase);

    file.close();

    // -- Populate result -----------------------------------------------------
    result.success        = true;
    result.fromSnapshot   = true;
    result.snapshotPath   = path;
    result.finalPC        = finalPC;
    result.finalPalBase   = palBase;
    result.cyclesExecuted = cycles;
    result.elapsedMs      = static_cast<double>(timer.elapsed());

    INFO_LOG(QString("loadSnapshot: complete in %1 ms -- %2 bytes restored")
        .arg(result.elapsedMs, 0, 'f', 1).arg(totalBytes));
    INFO_LOG(QString("  Original decompression was %1 cycles").arg(cycles));

    return result;
}
