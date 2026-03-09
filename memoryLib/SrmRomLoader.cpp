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
#include <QFileInfo>
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
// Reads only SrmBase (loadPA) and SrmMaxSteps.
// All other boot parameters are derived from the firmware binary and are
// available via SrmRomLoader::descriptor() after loadFromFile() / useEmbedded().
//
// Obsolete keys (SrmInitialPC, SrmDonePC, SrmMirrorPA, SrmSize, SrmRomVariant)
// are silently ignored -- no INI edits required to remove them.
// ============================================================================

static quint64 readU64(const QSettings& s, const QString& key, quint64 defaultVal)
{
    const QVariant v = s.value(key);
    if (!v.isValid())
        return defaultVal;
    bool ok = false;
    const quint64 result = v.toString().toULongLong(&ok, 0);
    return ok ? result : defaultVal;
}

SrmLoaderConfig SrmLoaderConfig::fromSettings(const QSettings& s)
{
    SrmLoaderConfig cfg;
    cfg.loadPA = readU64(s, "MemoryMap/SrmBase", cfg.loadPA);
    cfg.maxSteps = s.value("MemoryMap/SrmMaxSteps", cfg.maxSteps).toInt();
    return cfg;
}

// ============================================================================
// checksum64 -- FNV-1a 64-bit
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
// populateDescriptor
// ============================================================================
// Derives all boot parameters from the decompressor stub:
//
//   PAL_BASE  -- read directly from stub+0x10 (little-endian quint64)
//                Embedded by DEC/Compaq/HP at firmware build time.
//                Identical across all ES40/ES45/DS10/DS20 V6.x-V7.x variants.
//
//   finalPC   -- found by scanning stub for validated LDA/JSR pair:
//                  LDA R0, disp(R26)  -- kLdaPattern, disp16 = finalPC
//                  JSR R31, (R0)      -- kJsrToFinalPC, immediately follows LDA
//                Scan offset varies between firmware versions.
//                Validation guards:
//                  1. LDA opcode/Ra/Rb must match kLdaPattern exactly
//                  2. finalPC candidate must be > 0 and < loadPA
//                  3. JSR must immediately follow LDA
//
//   GuestPhysicalRegion seed entries populated for snapshot and registry use.
// ============================================================================

bool SrmRomLoader::populateDescriptor(const QString& sourceDescription, quint64 loadPA)
{
    m_descriptor = SrmRomDescriptor{};  // reset

    if (!m_payloadData || m_payloadSize < 0x20) {
        ERROR_LOG("populateDescriptor: payload too small for stub analysis");
        return false;
    }

    // -- PAL_BASE from stub+0x10 ---------------------------------------------
    quint64 palBase = 0;
    std::memcpy(&palBase, m_payloadData + 0x10, sizeof(quint64));
    if (palBase == 0) {
        ERROR_LOG("populateDescriptor: PAL_BASE at stub+0x10 is zero -- unexpected");
        return false;
    }

    // -- finalPC from validated LDA/JSR pair scan ----------------------------
    // Scan first kJsrScanLimit bytes of payload for JSR R31,(R0) = kJsrToFinalPC.
    // For each candidate JSR, validate the preceding instruction is
    // LDA R0, disp(R26) and that disp16 (finalPC) is in a plausible range.

    quint64 finalPC = 0;
    size_t  jsrOff = 0;
    bool    foundJsr = false;

    const size_t scanLimit = qMin(m_payloadSize, kJsrScanLimit);

    for (size_t off = 4; off + 4 <= scanLimit; off += 4)
    {
        quint32 instr = 0;
        std::memcpy(&instr, m_payloadData + off, sizeof(quint32));

        if (instr != kJsrToFinalPC)
            continue;

        // Candidate JSR found -- validate preceding LDA R0, disp(R26)
        quint32 ldaInstr = 0;
        std::memcpy(&ldaInstr, m_payloadData + off - 4, sizeof(quint32));

        if ((ldaInstr & kLdaMask) != kLdaPattern) {
            WARN_LOG(QString("populateDescriptor: JSR at stub+0x%1 -- "
                "preceding instr 0x%2 is not LDA R0,disp(R26) -- skipping")
                .arg(off, 0, 16)
                .arg(ldaInstr, 8, 16, QChar('0')));
            continue;
        }

        const quint64 candidate = static_cast<quint64>(ldaInstr & 0xFFFF);

        if (candidate == 0) {
            WARN_LOG(QString("populateDescriptor: JSR at stub+0x%1 -- "
                "finalPC candidate is 0x0 -- skipping")
                .arg(off, 0, 16));
            continue;
        }

        if (candidate >= loadPA) {
            WARN_LOG(QString("populateDescriptor: JSR at stub+0x%1 -- "
                "finalPC candidate 0x%2 >= loadPA 0x%3 -- skipping")
                .arg(off, 0, 16)
                .arg(candidate, 0, 16)
                .arg(loadPA, 0, 16));
            continue;
        }

        // Validated
        finalPC = candidate;
        jsrOff = off;
        foundJsr = true;

        INFO_LOG(QString("populateDescriptor: LDA/JSR pair validated at "
            "stub+0x%1 / stub+0x%2  finalPC=0x%3")
            .arg(off - 4, 0, 16)
            .arg(off, 0, 16)
            .arg(finalPC, 0, 16));
        break;
    }

    if (!foundJsr) {
        ERROR_LOG(QString("populateDescriptor: LDA/JSR pair not found in first "
            "0x%1 bytes of stub -- cannot derive finalPC")
            .arg(scanLimit, 0, 16));
        return false;
    }

    // -- Populate descriptor -------------------------------------------------
    m_descriptor.valid = true;
    m_descriptor.sigOffset = m_headerSkip;
    m_descriptor.headerSkip = m_headerSkip;
    m_descriptor.payloadSize = m_payloadSize;
    m_descriptor.palBase = palBase;
    m_descriptor.finalPC = finalPC;
    m_descriptor.jsrOffset = jsrOff;
    m_descriptor.copyLoopOff = 0x3EC;
    m_descriptor.copyExitOff = 0x408;
    m_descriptor.sourceDescription = sourceDescription;

    // -- Seed GuestPhysicalRegion entries ------------------------------------
    // Field order: basePA, size, type, source, description,
    //              populated, readOnly, includeInSnapshot, hwrpbVisible

    // Firmware staging area (decompressor stub at loadPA)
    m_descriptor.regions.push_back({
        loadPA,
        static_cast<quint64>(m_payloadSize),
        GuestRegionType::Firmware,
        GuestRegionSource::FirmwareBinary,
        QString("SRM decompressor stub -- %1").arg(sourceDescription),
        false,   // populated
        false,   // readOnly
        true,    // includeInSnapshot
        false    // hwrpbVisible
        });

    // Decompressed firmware image (always written to PA 0x0)
    // Size = 0x400000 (4MB) -- from R30 at stub exit (BIS R31,R0,R30)
    // Conservative: use 0x400000 for all known ES40/ES45 variants.
    m_descriptor.regions.push_back({
        0x0ULL,
        0x400000ULL,
        GuestRegionType::DecompressedFW,
        GuestRegionSource::FirmwareBinary,
        QString("SRM decompressed firmware -- %1").arg(sourceDescription),
        false,   // populated
        false,   // readOnly
        true,    // includeInSnapshot
        false    // hwrpbVisible
        });

    INFO_LOG(QString("SrmRomDescriptor populated -- source='%1'").arg(sourceDescription));
    INFO_LOG(QString("  sigOffset    : 0x%1").arg(m_descriptor.sigOffset, 0, 16));
    INFO_LOG(QString("  headerSkip   : 0x%1").arg(m_descriptor.headerSkip, 0, 16));
    INFO_LOG(QString("  payloadSize  : %1 bytes (0x%2)")
        .arg(m_descriptor.payloadSize)
        .arg(m_descriptor.payloadSize, 0, 16));
    INFO_LOG(QString("  palBase      : 0x%1").arg(m_descriptor.palBase, 0, 16));
    INFO_LOG(QString("  finalPC      : 0x%1").arg(m_descriptor.finalPC, 0, 16));
    INFO_LOG(QString("  jsrOffset    : stub+0x%1").arg(m_descriptor.jsrOffset, 0, 16));
    INFO_LOG(QString("  startPC      : 0x%1  [loadPA + sigOffset + 1]")
        .arg(m_descriptor.startPC(loadPA), 0, 16));
    INFO_LOG(QString("  donePC       : 0x%1  [finalPC + 0x40]")
        .arg(m_descriptor.donePC(), 0, 16));

    return true;
}

// ============================================================================
// useEmbedded
// ============================================================================

bool SrmRomLoader::useEmbedded(quint64 loadPA)
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
    m_romSize = kES45SrmRomSize;
    m_headerSkip = static_cast<size_t>(sigOffset);
    m_payloadData = kES45SrmRomData + m_headerSkip;
    m_payloadSize = kES45SrmRomSize - m_headerSkip;
    m_fileBuffer.clear();

    if (!populateDescriptor("embedded ES45 V6.2", loadPA))
        return false;

    INFO_LOG(QString("Embedded ES45 V6.2 -- %1 bytes, header skip 0x%2, payload %3 bytes")
        .arg(m_romSize).arg(m_headerSkip, 0, 16).arg(m_payloadSize));
    return true;
#endif
}

// ============================================================================
// loadFromFile
// ============================================================================

bool SrmRomLoader::loadFromFile(const QString& filePath, quint64 loadPA)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        ERROR_LOG(QString("Cannot open ROM file: %1").arg(filePath));
        return false;
    }
    m_fileBuffer = file.readAll();
    file.close();

    const auto* raw = reinterpret_cast<const quint8*>(m_fileBuffer.constData());
    const size_t rawSize = static_cast<size_t>(m_fileBuffer.size());

    const qint64 sigOffset = findDecompressor(raw, rawSize);
    if (sigOffset < 0) {
        ERROR_LOG(QString("No EV6 decompressor signature in '%1'").arg(filePath));
        m_fileBuffer.clear();
        return false;
    }

    m_romSize = rawSize;
    m_headerSkip = static_cast<size_t>(sigOffset);
    m_payloadData = raw + m_headerSkip;
    m_payloadSize = rawSize - m_headerSkip;

    const QString baseName = QFileInfo(filePath).fileName();
    if (!populateDescriptor(baseName, loadPA)) {
        m_fileBuffer.clear();
        m_payloadData = nullptr;
        m_payloadSize = 0;
        return false;
    }

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
    if (!m_descriptor.valid) {
        result.errorMessage = "SrmRomDescriptor not populated -- call loadFromFile() first";
        ERROR_LOG(result.errorMessage);
        return result;
    }
    if (!writeToPhysical || !singleStep || !setPC || !setPalBase || !getPalBase) {
        result.errorMessage = "Missing required callback(s)";
        ERROR_LOG(result.errorMessage);
        return result;
    }

    // -- Derive runtime boot values from descriptor --------------------------
    const quint64 startPC = m_descriptor.startPC(cfg.loadPA);
    const quint64 donePC = m_descriptor.donePC();
    const quint64 mirrorPA = 0x0ULL;
    const quint64 palBase = m_descriptor.palBase;

    // Phase detection -- absolute PAs
    const quint64 kCopyLoopPA = cfg.loadPA
        + static_cast<quint64>(m_descriptor.sigOffset)
        + m_descriptor.copyLoopOff;
    const quint64 kCopyExitPA = cfg.loadPA
        + static_cast<quint64>(m_descriptor.sigOffset)
        + m_descriptor.copyExitOff;

    INFO_LOG("=== SRM Decompression starting ===");
    INFO_LOG(QString("  Source      : %1").arg(m_descriptor.sourceDescription));
    INFO_LOG(QString("  ROM payload : %1 bytes").arg(m_payloadSize));
    INFO_LOG(QString("  Load PA     : 0x%1").arg(cfg.loadPA, 0, 16));
    INFO_LOG(QString("  Mirror PA   : 0x%1").arg(mirrorPA, 0, 16));
    INFO_LOG(QString("  Start PC    : 0x%1").arg(startPC, 0, 16));
    INFO_LOG(QString("  PAL_BASE    : 0x%1").arg(palBase, 0, 16));
    INFO_LOG(QString("  Final PC    : 0x%1").arg(m_descriptor.finalPC, 0, 16));
    INFO_LOG(QString("  Done when PC < 0x%1").arg(donePC, 0, 16));

    // -- Load firmware into guest memory -------------------------------------
    writeToPhysical(mirrorPA, m_payloadData, m_payloadSize);
    writeToPhysical(cfg.loadPA, m_payloadData, m_payloadSize);

    // -- Set initial CPU state -----------------------------------------------
    setPC(startPC);
    setPalBase(palBase);

    // -- Run decompressor ----------------------------------------------------
    QElapsedTimer timer;
    timer.start();
    CpuTrace::startElapsedTime();

    quint64 stepCount = 0;
    quint64 currentPC = startPC;
    int     lastPercent = -1;

    enum class Phase { Init, CopyLoop, PostCopy } phase = Phase::Init;
    quint64 copyLoopEnterStep = 0;
    qint64  copyLoopEnterMs = 0;

    INFO_LOG(QString("[T+00:00:00.000] Decompressor started -- PC=0x%1, limit=%2 steps")
        .arg(startPC, 0, 16).arg(cfg.maxSteps));

    const quint64 kBatchSize = 1800000ULL;
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
            if (phase == Phase::Init && cleanPC == kCopyLoopPA)
            {
                phase = Phase::CopyLoop;
                copyLoopEnterStep = stepCount;
                copyLoopEnterMs = timer.elapsed();
                INFO_LOG(QString("[T+%1] Copy loop entered -- step %2")
                    .arg(CpuTrace::elapsedTime()).arg(stepCount));
            }
            else if (phase == Phase::CopyLoop && cleanPC == kCopyExitPA)
            {
                phase = Phase::PostCopy;
                result.copyLoopSteps = stepCount - copyLoopEnterStep;
                result.copyLoopMs = static_cast<double>(timer.elapsed() - copyLoopEnterMs);
                result.initSteps = copyLoopEnterStep;
                INFO_LOG(QString("[T+%1] Copy loop complete -- %2 steps in %3 ms")
                    .arg(CpuTrace::elapsedTime())
                    .arg(result.copyLoopSteps)
                    .arg(result.copyLoopMs, 0, 'f', 1));
            }

            if (cleanPC < donePC)
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
    result.postCopySteps = stepCount - (result.initSteps + result.copyLoopSteps);
    result.success = true;
    result.finalPC = currentPC;
    result.finalPalBase = getPalBase();
    result.cyclesExecuted = stepCount;
    result.elapsedMs = static_cast<double>(timer.elapsed());

    CpuTrace::writeElapsedToMachineLine();
    if (progress) progress(100);

    INFO_LOG(QString("[T+%1] Decompression complete").arg(CpuTrace::elapsedTime()));
    INFO_LOG(QString("  Final PC    : 0x%1").arg(result.finalPC, 0, 16));
    INFO_LOG(QString("  PAL_BASE    : 0x%1").arg(result.finalPalBase, 0, 16));
    INFO_LOG(QString("  Total steps : %1").arg(result.cyclesExecuted));
    INFO_LOG(QString("  Init steps  : %1").arg(result.initSteps));
    INFO_LOG(QString("  Copy steps  : %1 (%2 ms)")
        .arg(result.copyLoopSteps).arg(result.copyLoopMs, 0, 'f', 1));
    INFO_LOG(QString("  Post steps  : %1").arg(result.postCopySteps));

    return result;
}

// ============================================================================
// saveSnapshot
// ============================================================================

bool SrmRomLoader::saveSnapshot(
    const QString& path,
    const SrmRomLoadResult& result,
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
    INFO_LOG(QString("  Boot PC   : 0x%1").arg(result.finalPC, 0, 16));
    INFO_LOG(QString("  PAL_BASE  : 0x%1").arg(result.finalPalBase, 0, 16));
    INFO_LOG(QString("  ROM hash  : 0x%1").arg(romHash, 0, 16));

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
            .arg(region.size, 0, 16)
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

    // -- Footer checksum -------------------------------------------------
    // Detach QDataStream first to flush its internal buffer to the QFile,
    // then close and reopen read-only to compute checksum over all bytes,
    // then append the 8-byte checksum.
    ds.setDevice(nullptr);  // detach -- flushes QDataStream internal buffer
    file.flush();
    file.close();

    // Read complete file content for checksum
    if (!file.open(QIODevice::ReadOnly)) {
        ERROR_LOG(QString("saveSnapshot: cannot reopen '%1' for checksum").arg(path));
        return false;
    }
    const QByteArray allData = file.readAll();
    file.close();

    const quint64 cksum = checksum64(
        reinterpret_cast<const quint8*>(allData.constData()),
        static_cast<size_t>(allData.size()));

    // Append checksum
    if (!file.open(QIODevice::Append)) {
        ERROR_LOG(QString("saveSnapshot: cannot append checksum to '%1'").arg(path));
        return false;
    }
    {
        QDataStream dsFooter(&file);
        dsFooter.setByteOrder(QDataStream::LittleEndian);
        dsFooter.setVersion(QDataStream::Qt_6_0);
        dsFooter << cksum;
    }
    const qint64 finalSize = file.size();
    file.close();

    INFO_LOG(QString("saveSnapshot: saved %1 bytes total memory, file size %2 bytes")
        .arg(totalBytes).arg(finalSize));
    INFO_LOG(QString("  Checksum  : 0x%1").arg(cksum, 0, 16));

    return true;
}

// ============================================================================
// loadSnapshot
// ============================================================================

SrmRomLoadResult SrmRomLoader::loadSnapshot(
    const QString& path,
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
    quint64 magic = 0;
    quint32 version = 0;
    quint64 romHash = 0;
    quint64 finalPC = 0;
    quint64 palBase = 0;
    quint64 cycles = 0;
    double  elapsed = 0.0;

    ds >> magic >> version >> romHash >> finalPC >> palBase >> cycles >> elapsed;

    if (magic != kSnapshotMagic) {
        result.errorMessage = QString("loadSnapshot: bad magic in '%1' -- not an .axpsnap file")
            .arg(path);
        ERROR_LOG(result.errorMessage);
        return result;
    }
    if (version != kSnapshotVersion) {
        result.errorMessage = QString("loadSnapshot: version mismatch -- file=%1, expected=%2")
            .arg(version).arg(kSnapshotVersion);
        ERROR_LOG(result.errorMessage);
        return result;
    }

    // ROM hash validation -- hard fail on mismatch (stale snapshot)
    if (m_payloadData) {
        quint64 currentHash = checksum64(m_payloadData, m_payloadSize);
        if (currentHash != romHash) {
            result.errorMessage = QString("loadSnapshot: ROM hash mismatch "
                "(snapshot=0x%1, current=0x%2) -- snapshot is stale")
                .arg(romHash, 0, 16)
                .arg(currentHash, 0, 16);
            ERROR_LOG(result.errorMessage);
            return result;
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
        quint32 id = 0;
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
            .arg(storedCksum, 0, 16)
            .arg(computedCksum, 0, 16);
        ERROR_LOG(result.errorMessage);
        return result;
    }

    // -- Restore CPU state ---------------------------------------------------
    setPC(finalPC);
    setPalBase(palBase);

    file.close();

    // -- Populate result -----------------------------------------------------
    result.success = true;
    result.fromSnapshot = true;
    result.snapshotPath = path;
    result.finalPC = finalPC;
    result.finalPalBase = palBase;
    result.cyclesExecuted = cycles;
    result.elapsedMs = static_cast<double>(timer.elapsed());

    INFO_LOG(QString("loadSnapshot: complete in %1 ms -- %2 bytes restored")
        .arg(result.elapsedMs, 0, 'f', 1).arg(totalBytes));
    INFO_LOG(QString("  Original decompression was %1 cycles").arg(cycles));

    return result;
}