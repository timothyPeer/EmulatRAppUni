#ifndef settings_h__
#define settings_h__

// ============================================================================
//  settings.h - CORRECTED VERSION (Flattened Structure)
//
//  Purpose: Complete emulator configuration structures
//
//  Aligned with:
//  - ASAEmulatr.ini (flattened structure)
//  - EmulatorSettingsInline.h (rewritten loader)
// ============================================================================

#include <QMap>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QtGlobal>
#include "../memoryLib/memory_core.h"




// ============================================================================
// CACHE CONFIGURATION (PROPER STRUCT - NOT QMAP)
// ============================================================================

struct CacheConfig {
    int numSets{ 256 };
    int associativity{ 2 };
    int lineSize{ 64 };
    int totalSize{ 0 };
    bool enablePrefetch{ true };
    bool enableStatistics{ true };
    bool enableCoherency{ true };
    QString coherencyProtocol{ "MESI" };
    int statusUpdateInterval{ 1000 };
    QString replacementPolicy{ "MRU" };
    int evictionThreshold{ 1000 };
    int cacheSize{ 48 };
};

// ============================================================================
// DEVICE CONFIGURATION (FLATTENED - No subBlocks)
// ============================================================================

struct DeviceConfig {
    QString name;
    QString classType;
    QString parent;
    
    // All properties stored here with dot notation:
    // "container.deviceType", "geometry.logical_sector", "Irq.irqStr", etc.
    QMap<QString, QVariant> fields;
};

// ============================================================================
// CONTROLLER CONFIGURATION
// ============================================================================

struct ControllerConfig {
    QString name;
    QString classType;
    
    // All properties including PCI, MMIO, IRQ
    QMap<QString, QVariant> fields;
};



// ============================================================================
// OPA CONSOLE CONFIGURATION
// ============================================================================

struct OPAConsoleConfig {
    QString name;                      // "OPA0", "OPA1"
    QString classType{ "UART" };       // Always "UART"
    QString location;                  // "cab0/drw0"
    QString iface{ "Net" };            // "Net", "Serial", "File"
    quint16 ifacePort{ 0 };            // Network port
    QString application;               // Launch command
    quint32 rxBufferSize{ 256 };
    quint32 txBufferSize{ 1024 };
    bool dropOnOverflow{ true };
    bool autoReconnect{ true };
};

// ============================================================================
// SYSTEM CONFIGURATION
// ============================================================================

struct SystemConfig {
    int memorySizeGB{ 32 };
    QString hwModel{ "ES40" };
    QString hwSerialNumber;
    SystemType_EmulatR sysType{ SystemType_EmulatR::ES40 };
    int coherencyCache{ 2048 };
    int platformEv{ 6 };
    int ptePageSize{ 8192 };
    int threadCount{ 4 };
    int processorCount{ 2 };
    quint64 cpuFrequencyHz{ 500000000 };
};

// ============================================================================
// LOGGING CONFIGURATION
// ============================================================================

struct LoggingConfig {
    // Output backends
    bool enableDiskLogging{ true };
    bool enableConsole{ true };

    // Log rotation
    quint64 maxLogFileSizeBytes{ 104857600 };  // 100 MB
    quint32 maxLogFileCount{ 10 };

    // File settings
    QString logFileName{ "logs/es40_instance.log" };
    quint8 logLevel{ 0 };
    quint32 flushInterval{ 10 };  // Flush every N messages (0 = always)
    // Behavior
    bool appendToExisting{ true };
    bool enableTimestamps{ true };
    bool useHighPerfTimestamps{ true };
    
    // Register state logging
    bool logRegisterState{ true };
    bool regEnableDiskLogging{ true };
    bool regEnableConsole{ true };
};

// ============================================================================
// EXECTRACE CONFIGURATION
// ============================================================================

struct ExecTraceConfig {
    // Enable/disable
    bool execTraceEnabled{ false };
    bool immediateFlush{ true };
    QString traceFormat{ "csv" };
    QString traceOutputDir;
    // Mode
    QString execTraceMode{ "triggered" };  // "triggered", "continuous", "sampled"

    // Output
    bool perCpuTraceFiles{ true };
    QString traceFilePattern{ "traces/es40_instance.cpu{cpu}.trace" };
    quint64 maxTraceFileSizeBytes{ 1073741824 };  // 1 GB
    quint32 maxTraceFileCount{ 10 };

    // Ring buffer
    quint32 traceRingRecordsPerCpu{ 4096 };
    quint32 traceDumpPreRecords{ 32 };
    quint32 traceDumpPostRecords{ 32 };

    // CPU filter
    quint32 cpuMask{ 0xF };

    // Triggers
    bool triggerOnException{ true };
    bool triggerOnIpi{ true };
    bool triggerOnPalEntry{ false };
    bool triggerOnPalExit{ false };
    bool pcRangeEnabled{ false };
    quint64 pcRangeStart{ 0 };
    quint64 pcRangeEnd{ 0 };

    // Record content
    bool includeIntRegWrites{ true };
    bool includeFpRegWrites{ false };
    bool includeIprWrites{ true };
    bool includeMemVA{ true };
    bool includeMemPA{ false };
    bool includeOpcodeWord{ true };

    // Performance
    quint32 flushIntervalMs{ 200 };
    bool fullTraceDebugBuildOnly{ true };
};

// ============================================================================
// TLB SHOOTDOWN CONFIGURATION
// ============================================================================

struct TLBShootdownConfig {
    bool enableACKs{ false };
    bool enablePreciseInvalidation{ false };
    bool enableShootdownLogging{ false };
    quint32 maxShootdownSeq{ 255 };
};

// ============================================================================
// INTERRUPT CONFIGURATION
// ============================================================================

struct InterruptConfig {
    bool enableInterruptMaskingLog{ false };
    QString criticalInterruptVectors;
};

// ============================================================================
// FLOATING POINT CONFIGURATION
// ============================================================================

struct FloatingPointConfig {
    bool useSSEForF_Float{ false };
    bool useSSEForG_Float{ false };
    bool useSSEForD_Float{ false };
    bool useSSEForS_Float{ true };
    bool useSSEForT_Float{ true };
};

// ============================================================================
// Physical Memory Address Space CONFIGURATION 
// ============================================================================

struct MemoryMapConfig {
    // HWRPB - Hardware Restart Parameter Block
    quint64 hwrpbBase{ 0x2000 };
    quint64 hwrpbSize{ 0x4000 };  // 16 KB

    // PAL - Privileged Architecture Library
    quint64 palBase{ 0x0 };
    quint64 palSize{ 0x10000 };  // 64 KB
    // RAM - Main system memory
    quint64 ramBase{ 0x80000000 };
    // ramSize computed from System.memorySizeGB

    // MMIO - Memory-mapped I/O (device registers)
    quint64 mmioBase{ 0xF0000000 };
    quint64 mmioSize{ 0x10000000 };  // 256 MB

    // PCI Memory - PCI device BARs
    quint64 pciMemBase{ 0x200000000 };
    quint64 pciMemSize{ 0x100000000 };  // 4 GB
};

// ============================================================================
// ROM CONFIGURATION
// ============================================================================

struct RomConfig {
    // TODO
    QString hostProcessorModuleFirmwareFile;
    QString pciBusModuleFirmWare;
    QString systemModuleFirmwareFile;
    QString intelHexLoaderFile;
    /// <summary>
    /// / SRM Production QSetting Properties
    /// </summary>
    QString srmRomFile;
    QString srmIncRomFile{ "ES45" };
};

// ============================================================================
// SESSION CONFIGURATION
// ============================================================================

struct SessionConfig {
    QString fName;
    int logLevel{ 0 };
    QString method;
    QString logFileName;
    QString updateMethod;
};

// ============================================================================
// COMPLETE EMULATOR SETTINGS (POD - Plain Old Data)
// ============================================================================

struct EmulatorSettings {
    // Core settings
    SystemConfig system;
    LoggingConfig logging;
    ExecTraceConfig execTrace;

    // Hardware configuration
    QMap<QString, CacheConfig> caches;            // "CACHE/l1", "CACHE/L2", "CACHE/L3"
    QMap<QString, ControllerConfig> controllers;  // "PKB0", "PKC0", "EWA0", "PQA0"
    QMap<QString, DeviceConfig> devices;          // "DKA0", "DKA1", "DKB1", "DQA0", "MKA600"
    QMap<QString, OPAConsoleConfig> opaConsoles;  // "OPA0", "OPA1"

    // Feature flags
    TLBShootdownConfig tlbShootdown;
    InterruptConfig interrupts;
    FloatingPointConfig floatingPoint;
    RomConfig rom;
    SessionConfig session;
    MemoryMapConfig memoryMap;
};

#endif // settings_h__
