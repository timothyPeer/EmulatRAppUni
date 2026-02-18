#ifndef SRMROMLOADER_H
#define SRMROMLOADER_H
// ============================================================================
// SrmRomLoader.h -- Alpha SRM Firmware ROM Loader
// ============================================================================
//
// Loads and decompresses DEC/Compaq/HP Alpha SRM console firmware images.
//
// Supported EV6-family firmware images:
//
//   Image              Platform          CPU           Header
//   -----------------  ----------------  ------------  ------
//   ES45_V6_2.EXE     AlphaServer ES45  EV68/21264C   None (embedded default)
//   DS10_V6_2.EXE     AlphaServer DS10  EV6/21264     None
//   DS20_V6_2.EXE     AlphaServer DS20  EV67/21264A   None
//   ES40_V6_2.EXE     AlphaServer ES40  EV6/21264     None
//   GS320_V62.EXE     AlphaServer GS320 EV67/21264A   None
//   cl67srmrom.exe     DS20/DS20E        EV67/21264A   0x240
//   clsrmrom.exe       DS10/DS10L        EV6/21264     0x240
//
// Algorithm:
//   All images contain a self-decompressing Alpha PALcode binary. The
//   decompressor is identified by a 12-byte signature at its entry point.
//   The loader scans for this signature to find the payload start,
//   eliminating per-image header size configuration.
//
//   Decompression procedure (from AxpBox/ES40 by Camiel Vanderhoeven):
//     1. Locate decompressor via signature scan
//     2. Copy from signature onward into guest physical memory at PA 0x900000
//     3. Set CPU: PC = 0x900001 (PAL mode), PAL_BASE = 0x900000
//     4. Single-step the CPU until PC < 0x200000 (decompression complete)
//     5. Read final PC and PAL_BASE from CPU state
//     6. Firmware is now resident at PA 0x0+
//
// Attribution:
//   Decompression-via-execution algorithm derived from:
//     ES40 Emulator -- Copyright (C) 2007-2008 Camiel Vanderhoeven
//     AxpBox fork   -- Copyright (C) 2020 Tomas Glozar
//     CSystem::LoadROM() in System.cpp
//     Licensed under GNU General Public License v2
//
// Firmware binaries are property of Digital Equipment Corporation /
// Compaq Computer Corporation / Hewlett-Packard Company.
//
// ============================================================================



#include <QtTypes>
#include <QString>
#include <QElapsedTimer>
#include <functional>

#include "coreLib/Axp_Attributes_core.h"

// ============================================================================
// Decompression Result
// ============================================================================

struct SrmRomLoadResult
{
    bool     success        = false;
    quint64  finalPC        = 0;        // Boot PC (with PAL bit)
    quint64  finalPalBase   = 0;        // PAL_BASE after decompression
    quint64  cyclesExecuted = 0;        // Decompressor instruction count
    double   elapsedMs      = 0.0;      // Wall-clock decompression time
    size_t   headerSkip     = 0;        // Bytes skipped before decompressor
    QString  errorMessage;

    quint64 bootPC()    const { return finalPC; }
    quint64 cleanPC()   const { return finalPC & ~1ULL; }
    bool    isPalMode() const { return (finalPC & 1ULL) != 0; }
    quint64 palBase()   const { return finalPalBase; }
};

// ============================================================================
// SrmRomLoader
// ============================================================================

class SrmRomLoader final
{
public:
    // -- Constants -----------------------------------------------------------

    // Decompressor signature: first 3 instructions shared by all EV6 SRM images
    //   0x443F0404  SEXTL R1, R4
    //   0x445F0405  SEXTL R2, R5
    //   0x479F040E  CLR   R14
    static constexpr quint8 kDecompSig[12] = {
        0x04, 0x04, 0x3F, 0x44,
        0x05, 0x04, 0x5F, 0x44,
        0x0E, 0x04, 0x9F, 0x47
    };
    static constexpr size_t  kDecompSigLen   = sizeof(kDecompSig);
    static constexpr size_t  kMaxHeaderScan  = 0x1000;      // Search first 4KB for signature
    static constexpr quint64 kDecompLoadPA   = 0x900000;    // Guest PA for decompressor
    static constexpr quint64 kDecompStartPC  = 0x900001;    // PA + PAL mode bit
    static constexpr quint64 kDecompPalBase  = 0x900000;    // Decompressor is its own PAL
    static constexpr quint64 kDecompDonePC   = 0x200000;    // PC below here = done
    static constexpr int     kMaxSteps       = 200000000;   // Safety limit

    // -- Construction --------------------------------------------------------

    SrmRomLoader() = default;

    /// Use the embedded ES45 V6.2 ROM (no file I/O)
    bool useEmbedded();

    /// Load any EV6 SRM ROM from file (auto-detects header)
    bool loadFromFile(const QString& filePath);

    // -- Query ---------------------------------------------------------------

    bool    isLoaded()    const { return m_payloadData != nullptr && m_payloadSize > 0; }
    size_t  romSize()     const { return m_romSize; }
    size_t  headerSkip()  const { return m_headerSkip; }
    size_t  payloadSize() const { return m_payloadSize; }

    // -- Decompression -------------------------------------------------------

    using WritePhysicalFn = std::function<void(quint64 pa, const quint8* data, size_t len)>;
    using SingleStepFn    = std::function<quint64()>;          // returns PC after step
    using SetU64Fn        = std::function<void(quint64)>;
    using GetU64Fn        = std::function<quint64()>;
    using ProgressFn      = std::function<void(int percent)>;  // optional

    AXP_FLATTEN SrmRomLoadResult decompress(
        WritePhysicalFn  writeToPhysical,
        SingleStepFn     singleStep,
        SetU64Fn         setPC,
        SetU64Fn         setPalBase,
        GetU64Fn         getPalBase,
        ProgressFn       progress = nullptr
    );

private:
    const quint8*  m_payloadData  = nullptr;   // Points past header to decompressor
    size_t         m_payloadSize  = 0;
    size_t         m_romSize      = 0;         // Total file size
    size_t         m_headerSkip   = 0;         // Bytes before decompressor
    QByteArray     m_fileBuffer;               // Owns file data (loadFromFile path)

    /// Scan buffer for decompressor signature, return offset or -1
    static qint64 findDecompressor(const quint8* data, size_t size);
};
#endif
