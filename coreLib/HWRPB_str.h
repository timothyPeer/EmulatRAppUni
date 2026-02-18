#ifndef HWRPB_STR_H
#define HWRPB_STR_H
#include <QtGlobal>

// Located at fixed address: 0x0000000000002000
struct HWRPB {
    // ========================================================================
    // Header (Identification)
    // ========================================================================
    quint64 physicalBase;          // Physical address of HWRPB (0x2000)
    quint64 signature;             // "HWRPB" magic number
    quint64 revision;              // HWRPB version
    quint64 size;                  // Total size of HWRPB

    // ========================================================================
    // System Configuration
    // ========================================================================
    quint64 cpuCount;              // Number of CPUs (1 for single-core)
    quint64 pageSize;              // Page size (8192 bytes = 8 KB)
    quint64 paSize;                // Physical address size (44 bits)
    quint64 asnMax;                // Max ASN value (255)

    // ========================================================================
    // Memory Configuration
    // ========================================================================
    quint64 memorySize;            // Total RAM (32 GB = 0x800000000)
    quint64 memoryBase;            // RAM base address (0x80000000)
    quint64 memoryCount;           // Number of memory descriptors
    quint64 memoryDescriptorOffset; // Offset to memory descriptor array

    // ========================================================================
    // Console Firmware (SRM)
    // ========================================================================
    quint64 consoleEntryPoint;     // Where to jump after PAL init
    quint64 consoleBasePA;         // SRM physical base (0x20000000)
    quint64 consoleSize;           // SRM size (2 MB)

    // ========================================================================
    // PALcode Information
    // ========================================================================
    quint64 palRevision;           // PAL version
    quint64 palBasePA;             // PAL physical base (0x0)
    quint64 palSize;               // PAL size (64 KB)

    // ========================================================================
    // System Identification
    // ========================================================================
    quint64 systemType;            // System type (DP264, CLIPPER, etc.)
    quint64 systemVariation;       // System variant
    quint64 systemRevision;        // System revision
    quint64 systemSerialNumber[2]; // 16-byte serial number

    // ========================================================================
    // Interrupt/Exception Vectors
    // ========================================================================
    quint64 intrEntryPoint;        // Interrupt handler entry
    quint64 callPalEntryPoint;     // CALL_PAL handler entry

    // ========================================================================
    // Processor-Specific Data
    // ========================================================================
    quint64 processorOffset;       // Offset to per-CPU data
    quint64 processorSize;         // Size of per-CPU data
    quint64 processorCount;        // Number of processor entries

    // ... and many more fields ...
};

#endif // HWRPB_STR_H
