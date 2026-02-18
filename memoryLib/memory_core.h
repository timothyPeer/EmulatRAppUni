#ifndef PLATFORMADDRESSMAP_CORE_H
#define PLATFORMADDRESSMAP_CORE_H

#include "../coreLib/LoggingMacros.h"
#include <QtGlobal>
#include <QStringList>


// ============================================================================
// CONSTANTS
// ============================================================================

static constexpr quint64 KB = 1ULL << 10;  // 1024
static constexpr quint64 MB = 1ULL << 20;  // 1048576
static constexpr quint64 GB = 1ULL << 30;  // 1073741824
static constexpr quint64 TB = 1ULL << 40;  // 1099511627776

static constexpr quint64 MAX_RAM_SIZE = 32ULL * 1024 * 1024 * 1024; // 32 GB
enum class MEM_STATUS {
	Ok,
	OutOfRange,
	Misaligned,
	TlbMiss,
	AccessViolation,
	BusError,
	Un_Aligned,
	Time_Out,
    WriteProtected,
    TranslationFault,
    NotInitialized,
    TargetMisDirect,
    IllegalInstruction
};

// ============================================================================
// MEMORY STATUS - Lightweight operation return codes
// ============================================================================
// Used by: GuestMemory, MMIO, AlphaMemorySystem
// Purpose: Quick status return from memory operations
// Usage: Immediate return value, checked by caller
//
enum class MemoryStatus : quint8
{
    SUCCESS = 0,              // Operation completed successfully

    // Generic failures (caller may retry or escalate)
    FAULT,                    // A fault occurred; see -        // AAH Vol. I: Exceptions
    RETRY,                    // Temporary condition; operation should retry  // e.g., transient contention
    PENDING,                  // Asynchronous op in-flight                    // DMA-backed paths, etc.

    // Specific non-exceptional statuses that guide the caller
    PAGE_BOUNDARY,            // Access crosses page boundary; split needed   //  8KB/other page sizes
    DEVICE_BUSY,              // I/O device temporarily unavailable
    WOULD_BLOCK,              // Non-blocking I/O would block

    // Reservation/atomic status (Alpha-specific helper for STx_C sequences)
    RESERVATION_LOST,         // LDx_L/STx_C: store-conditional failed        //  Load-locked/Store-conditional
};

// ============================================================================
// MEMORY ACCESS KIND - What kind of access caused/was attempted
// ============================================================================
// Keep EXECUTE distinct; Alpha treats instruction fetch via ITB as "read"
// architecturally, but emulators benefit from explicit EXECUTE classification.
//
enum class MemoryAccessType : quint8
{
    READ = 0,              // Load/read operation
    WRITE = 1,             // Store/write operation
    EXECUTE = 2,           // Instruction fetch
    READ_MODIFY_WRITE = 3, // Atomic RMW (LL/SC, interlocked sequences)
};

// ============================================================================
// MEMORY ACCESS SIZE - Size of the access on the guest bus
// ============================================================================
enum class MemoryAccessSize : quint8
{
    UNKNOWN = 0,
    BYTE = 1,            // 8-bit
    WORD = 2,            // 16-bit
    LONGWORD = 4,        // 32-bit
    QUADWORD = 8,        // 64-bit
    OCTAWORD = 16,       // 128-bit (vector ops, device FIFOs)
};




// ============================================================================
// PERMISSION DETAILS (for PROTECTION_VIOLATION or ACCESS_VIOLATION)
// ============================================================================
// Encodes which check failed (R/W by mode, exec if you model it).
// Note: Early Alpha treats execute as a read; EXECUTE flag is for emulator policy.
//
enum class PermissionDetail : quint8
{
    NONE = 0,

    // Read permission failures
    READ_DISALLOWED_KERNEL = 1,      // KRE=0
    READ_DISALLOWED_EXECUTIVE = 2,   // ERE=0
    READ_DISALLOWED_SUPERVISOR = 3,  // SRE=0
    READ_DISALLOWED_USER = 4,        // URE=0

    // Write permission failures
    WRITE_DISALLOWED_KERNEL = 5,     // KWE=0
    WRITE_DISALLOWED_EXECUTIVE = 6,  // EWE=0
    WRITE_DISALLOWED_SUPERVISOR = 7, // SWE=0
    WRITE_DISALLOWED_USER = 8,       // UWE=0

    // Execute permission failures
    EXECUTE_DISALLOWED = 9,          // Attempt to execute from non-executable page

    // Special permissions
    PALSPACE_ONLY = 10,              // Access to PALcode-only address space
    PRIVILEGE_VIOLATION = 11,        // Insufficient privilege level
    FAULT_ON_WRITE = 12,             // FOW bit set (copy-on-write)
    FAULT_ON_READ = 13,              // FOR bit set (rare, used for demand-paging)
    FAULT_ON_EXECUTE = 14,           // FOE bit set (used for code page-in)
    READ_DISALLOWED,          // Mode cannot read this page (K/E/S/U mask)     //  PTE KRE/ERE/SRE/URE
    WRITE_DISALLOWED         // Mode cannot write this page (KWE/EWE/...)     //  PTE *WE bits + FOW
};

// ============================================================================
// Memory Access Status (Exceptions) Enumeration
// ============================================================================
enum class MemAccessStatus {
    SUCCESS,
    INVALID_ADDRESS,
    ALIGNMENT_FAULT,
    ACCESS_VIOLATION,
    PAGE_BOUNDARY,
    HARDWARE_ERROR
};


// ============================================================================
// SYSTEM TYPE ENUMERATION
// ============================================================================

/**
 * @brief Alpha system types (chipset families).
 */
enum class SystemType_EmulatR : quint8 {
    DS10,       // Tsunami chipset, 2 GB RAM max
    DS20,       // Tsunami chipset, 2 GB RAM max
    ES40,       // Clipper chipset, 64 GB RAM max
    ES45,       // Clipper chipset, 64 GB RAM max
    GS80,       // Wildfire chipset, 128 GB RAM max
    GS160,      // Wildfire chipset, 256 GB RAM max
    GS320,      // Wildfire chipset, 512 GB RAM max
};


// ============================================================================
// Helper Functions for Permission Checking
// ============================================================================

// Convert mode (0=K, 1=E, 2=S, 3=U) to string
inline const char* modeToString(quint8 mode) {
    switch (mode) {
    case 0: return "Kernel";
    case 1: return "Executive";
    case 2: return "Supervisor";
    case 3: return "User";
    default: return "Unknown";
    }
}

// Get permission detail for a read failure based on current mode
inline PermissionDetail readPermissionFault(quint8 mode) {
    switch (mode) {
    case 0: return PermissionDetail::READ_DISALLOWED_KERNEL;
    case 1: return PermissionDetail::READ_DISALLOWED_EXECUTIVE;
    case 2: return PermissionDetail::READ_DISALLOWED_SUPERVISOR;
    case 3: return PermissionDetail::READ_DISALLOWED_USER;
    default: return PermissionDetail::NONE;
    }
}

// Get permission detail for a write failure based on current mode
inline PermissionDetail writePermissionFault(quint8 mode) {
    switch (mode) {
    case 0: return PermissionDetail::WRITE_DISALLOWED_KERNEL;
    case 1: return PermissionDetail::WRITE_DISALLOWED_EXECUTIVE;
    case 2: return PermissionDetail::WRITE_DISALLOWED_SUPERVISOR;
    case 3: return PermissionDetail::WRITE_DISALLOWED_USER;
    default: return PermissionDetail::NONE;
    }
}


// ============================================================================
// ADDRESS SPACE STRUCTURES
// ============================================================================

/**
 * @brief Physical address aperture (region descriptor).
 */
struct Aperture {
    quint64 base;       // Starting physical address
    quint64 size;       // Size in bytes
    QString name;       // Human-readable name (e.g., "Hose0 MMIO32")

    /**
     * @brief Get limit address (base + size).
     */
    quint64 limit() const {
        return base + size;
    }

    /**
     * @brief Check if PA is within this aperture.
     */
    bool contains(quint64 pa) const {
        return pa >= base && pa < limit();
    }
};


/**
 * @brief Platform-specific physical address space layout.
 *
 * Defines the memory map for different Alpha chipsets:
 *  - DS10/DS20 (Tsunami): 2 GB RAM, MMIO at 0x8000_0000
 *  - ES40/ES45 (Clipper): 64 GB RAM, MMIO at 0xF800_0000_0000
 *  - GS80/160/320 (Wildfire): 128-512 GB RAM, MMIO at 0x8000_0000_0000_0000
 */
struct PlatformAddressMap {
    // ========================================================================
    // RAM CONFIGURATION
    // ========================================================================

    quint64 ramBase;        // RAM start address (usually 0x0)
    quint64 ramMaxSize;     // Maximum RAM capacity for this platform
    quint64 ramActualSize;  // Actual RAM installed (? ramMaxSize)

    // ========================================================================
    // MMIO CONFIGURATION
    // ========================================================================

    quint64 mmioBase;       // Start of MMIO aperture space
    quint64 mmioSize;       // Total MMIO aperture size

    // Detailed MMIO apertures (per-hose, per-device-type)
    QVector<Aperture> mmioApertures;

    // ========================================================================
    // SYSTEM IDENTIFICATION
    // ========================================================================

    QString chipsetName;    // "Tsunami", "Clipper", "Wildfire"
    SystemType_EmulatR systemType;  // Enum for programmatic checks

    // ========================================================================
    // FACTORY METHOD
    // ========================================================================

    /**
     * @brief Create platform-specific address map.
     *
     * @param type System type (DS10, ES40, GS80, etc.)
     * @param installedRAM Amount of RAM installed (bytes)
     * @return Configured address map for this platform
     */

    // ========================================================================
    // 	// HELPER METHODS
    // 	// ========================================================================
    //
    // 	/**
    // 	 * @brief Check if PA range is entirely in RAM.
    // 	 *
    // 	 * @param pa Physical address
    // 	 * @param len Length in bytes
    // 	 * @return true if [pa, pa+len) is entirely in RAM
    // 	 */
    //
    // 	 // ============================================================================
    // 	 // PLATFORM ADDRESS MAP - HELPER METHODS
    // 	 // ============================================================================
    bool isRAM(quint64 pa, qsizetype len = 1) const
    {
        // Check overflow
        if (pa + len < pa) {
            return false;
        }

        // Check if entirely in RAM range
        if (pa < ramBase) {
            return false;
        }

        if (pa >= ramBase + ramActualSize) {
            return false;
        }

        if (pa + len > ramBase + ramActualSize) {
            return false;
        }

        // Check if overlaps MMIO (fast path)
        if (pa >= mmioBase) {
            return false;
        }

        return true;
    }
    //
    // 	/**
    // 	 * @brief Check if PA range overlaps MMIO.
    // 	 *
    // 	 * @param pa Physical address
    // 	 * @param len Length in bytes
    // 	 * @return true if any part of [pa, pa+len) is MMIO
    // 	 */
    //
    bool isMMIO(quint64 pa, qsizetype len = 1) const
    {
        // Check overflow
        quint64 endPA = pa + len;
        if (endPA < pa) {
            return true;  // Treat overflow as invalid/MMIO
        }

        // Fast path: check global MMIO range
        if (pa >= mmioBase && pa < mmioBase + mmioSize) {
            return true;
        }

        // Slow path: check specific apertures
        for (const Aperture& aperture : mmioApertures) {
            // Check for overlap: [pa, endPA) vs [aperture.base, aperture.limit())
            if (pa < aperture.limit() && endPA > aperture.base) {
                return true;  // Overlaps this aperture
            }
        }

        return false;
    }
    //
    // 	/**
    // 	 * @brief Find MMIO aperture containing PA.
    // 	 *
    // 	 * @param pa Physical address
    // 	 * @return Pointer to aperture, or nullptr if not MMIO
    // 	 */
    //
    const Aperture* findMMIOAperture(quint64 pa) const
    {
        for (const Aperture& aperture : mmioApertures) {
            if (aperture.contains(pa)) {
                return &aperture;
            }
        }
        return nullptr;
    }
    //
    // 	/**
    // 	 * @brief Get human-readable address space summary.
    // 	 */
    //
    QString toString() const
    {
        QStringList lines;
        lines << QString("Platform: %1").arg(chipsetName);
        lines << QString("  RAM: 0x%1 - 0x%2 (%3 GB / %4 GB max)")
            .arg(ramBase, 16, 16, QChar('0'))
            .arg(ramBase + ramActualSize, 16, 16, QChar('0'))
            .arg(ramActualSize / (double)GB, 0, 'f', 2)
            .arg(ramMaxSize / (double)GB, 0, 'f', 0);

        lines << QString("  MMIO: 0x%1 - 0x%2")
            .arg(mmioBase, 16, 16, QChar('0'))
            .arg(mmioBase + mmioSize, 16, 16, QChar('0'));

        for (const Aperture& aperture : mmioApertures) {
            lines << QString("    %1: 0x%2 - 0x%3 (%4 MB)")
                .arg(aperture.name)
                .arg(aperture.base, 16, 16, QChar('0'))
                .arg(aperture.limit(), 16, 16, QChar('0'))
                .arg(aperture.size / (double)MB, 0, 'f', 0);
        }

        return lines.join('\n');
    }

};

class IMemoryBuffer {
public:
    virtual quint8* getPointer(quint64 offset) = 0;
    virtual quint64 size() const = 0;
};

#endif // PLATFORMADDRESSMAP_CORE_H
