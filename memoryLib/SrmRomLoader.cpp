// ============================================================================
// SrmRomLoader.cpp -- Alpha SRM Firmware ROM Loader
// ============================================================================
//
// Attribution:
//   Decompression-via-execution algorithm derived from:
//     ES40 Emulator -- Copyright (C) 2007-2008 Camiel Vanderhoeven
//     AxpBox fork   -- Copyright (C) 2020 Tomas Glozar
//     CSystem::LoadROM() in System.cpp
//     Licensed under GNU General Public License v2
//
// ============================================================================

#include "SrmRomLoader.h"
#include "../coreLib/LoggingMacros.h"
#include <QFile>
#include <cstring>

#define COMPONENT_NAME "SrmRomLoader"

// ============================================================================
// Embedded ROM -- ES45 V6.2 (default, no file I/O required)
//
// For local builds with .inc files in romLib/.
// Public builds use loadFromFile() only.
// ============================================================================

#include "romLib/SrmRomData_ES45.inc"

// ============================================================================
// findDecompressor -- Scan for the universal EV6 decompressor signature
// ============================================================================

qint64 SrmRomLoader::findDecompressor(const quint8* data, size_t size)
{
    if (!data || size < kDecompSigLen)
        return -1;

    const size_t searchLimit = qMin(size - kDecompSigLen, kMaxHeaderScan);

    // Alpha instructions are 4-byte aligned
    for (size_t offset = 0; offset <= searchLimit; offset += 4)
    {
        if (std::memcmp(data + offset, kDecompSig, kDecompSigLen) == 0)
            return static_cast<qint64>(offset);
    }

    return -1;
}

// ============================================================================
// useEmbedded -- Select embedded ES45 V6.2 ROM
// ============================================================================

bool SrmRomLoader::useEmbedded()
{
    const qint64 sigOffset = findDecompressor(kES45SrmRomData, kES45SrmRomSize);

    if (sigOffset < 0) {
        ERROR_LOG("Decompressor signature not found in embedded ROM");
        return false;
    }

    m_romSize      = kES45SrmRomSize;
    m_headerSkip   = static_cast<size_t>(sigOffset);
    m_payloadData  = kES45SrmRomData + m_headerSkip;
    m_payloadSize  = kES45SrmRomSize - m_headerSkip;
    m_fileBuffer.clear();

    INFO_LOG(QString("Embedded ES45 V6.2 -- %1 bytes, header skip %2")
        .arg(m_romSize).arg(m_headerSkip));
    return true;
}

// ============================================================================
// loadFromFile -- Load any EV6 SRM ROM, auto-detect header
// ============================================================================

bool SrmRomLoader::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        ERROR_LOG(QString("Cannot open: %1").arg(filePath));
        return false;
    }

    m_fileBuffer = file.readAll();
    file.close();

    const auto* raw = reinterpret_cast<const quint8*>(m_fileBuffer.constData());
    const size_t rawSize = static_cast<size_t>(m_fileBuffer.size());

    const qint64 sigOffset = findDecompressor(raw, rawSize);

    if (sigOffset < 0) {
        ERROR_LOG(QString("No EV6 decompressor signature in %1 -- "
                          "not a supported SRM ROM image").arg(filePath));
        m_fileBuffer.clear();
        return false;
    }

    m_romSize      = rawSize;
    m_headerSkip   = static_cast<size_t>(sigOffset);
    m_payloadData  = raw + m_headerSkip;
    m_payloadSize  = rawSize - m_headerSkip;

    INFO_LOG(QString("Loaded %1 -- %2 bytes, header skip 0x%3, payload %4 bytes")
        .arg(filePath)
        .arg(m_romSize)
        .arg(m_headerSkip, 0, 16)
        .arg(m_payloadSize));
    return true;
}

// ============================================================================
// decompress -- Execute the decompressor on the guest CPU
// ============================================================================
//
// Replicates CSystem::LoadROM() from AxpBox/ES40:
//
//   1. Copy payload -> guest PA 0x900000
//   2. PC = 0x900001 (PAL mode), PAL_BASE = 0x900000
//   3. Single-step CPU via runOneInstruction() until PC < 0x200000
//   4. Return final {PC, PAL_BASE}
//
// The decompressor is real Alpha PALcode. It uses HW_MFPR, HW_MTPR,
// HW_LD, HW_ST, HW_REI and CALL_PAL (CSERVE, WRFEN, SWPCTX, LDQP).
// All must be implemented in the pipeline for decompression to succeed.
//
// ============================================================================

SrmRomLoadResult SrmRomLoader::decompress(
    WritePhysicalFn  writeToPhysical,
    SingleStepFn     singleStep,
    SetU64Fn         setPC,
    SetU64Fn         setPalBase,
    GetU64Fn         getPalBase,
    ProgressFn       progress)
{
    SrmRomLoadResult result;
    result.headerSkip = m_headerSkip;

    // -- Validate ------------------------------------------------------------

    if (!isLoaded()) {
        result.errorMessage = QStringLiteral("No ROM image loaded");
        ERROR_LOG(result.errorMessage);
        return result;
    }

    if (!writeToPhysical || !singleStep || !setPC || !setPalBase || !getPalBase) {
        result.errorMessage = QStringLiteral("Missing required callback(s)");
        ERROR_LOG(result.errorMessage);
        return result;
    }

    INFO_LOG(QString("Decompressing %1 byte payload").arg(m_payloadSize));
    INFO_LOG(QString("  Load PA:   0x%1").arg(kDecompLoadPA, 0, 16));
    INFO_LOG(QString("  Start PC:  0x%1").arg(kDecompStartPC, 0, 16));
    INFO_LOG(QString("  PAL_BASE:  0x%1").arg(kDecompPalBase, 0, 16));

    // -- Step 1: Copy payload into guest memory ------------------------------

    writeToPhysical(kDecompLoadPA, m_payloadData, m_payloadSize);

    // -- Step 2: Set CPU initial state ---------------------------------------

    setPC(kDecompStartPC);
    setPalBase(kDecompPalBase);

    // -- Step 3: Execute decompressor ----------------------------------------

    QElapsedTimer timer;
    timer.start();

    quint64 stepCount = 0;
    quint64 currentPC = kDecompStartPC;
    int     lastPercent = -1;

    // Batch to reduce progress callback overhead
    constexpr quint64 kBatchSize  = 1800000;
    constexpr int     kMaxBatches = kMaxSteps / static_cast<int>(kBatchSize);

    for (int batch = 0; batch < kMaxBatches; ++batch)
    {
        for (quint64 i = 0; i < kBatchSize; ++i)
        {
            currentPC = singleStep();
            stepCount++;

            if ((currentPC & ~1ULL) < kDecompDonePC)
                goto done;
        }

        // Report progress between batches
        int percent = qMin(99, (batch + 1) * 2);
        if (progress && percent != lastPercent) {
            progress(percent);
            lastPercent = percent;
        }
    }

    // Timeout -- decompression did not complete
    result.errorMessage = QString("Decompression stalled after %1 steps (PC=0x%2)")
                              .arg(stepCount)
                              .arg(currentPC, 0, 16);
    ERROR_LOG(result.errorMessage);
    return result;

done:
    // -- Step 4: Capture final state -----------------------------------------

    result.success        = true;
    result.finalPC        = currentPC;
    result.finalPalBase   = getPalBase();
    result.cyclesExecuted = stepCount;
    result.elapsedMs      = timer.elapsed();

    if (progress)
        progress(100);

    INFO_LOG(QString("Done -- %1 cycles, %2 ms")
        .arg(stepCount).arg(result.elapsedMs, 0, 'f', 1));
    INFO_LOG(QString("  PC=0x%1  PAL_BASE=0x%2")
        .arg(result.finalPC, 0, 16)
        .arg(result.finalPalBase, 0, 16));

    return result;
}
