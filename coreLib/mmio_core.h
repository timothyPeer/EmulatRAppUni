// ============================================================================
// mmio_core.h - endianness (default LE)
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#pragma once
#include <QtGlobal>
#include <QChar>
#include <QString>
#include <QDateTime>

static const quint64 INVALID_PFN = 0xFFFFFFFFFFFFFFFFULL;  // mmio_coredata

enum class mmio_CachePolicy : quint8 {
	Uncacheable = 0,    // UC - no caching
	WriteThrough = 1,   // WT - write-through (not typical for MMIO)
	WriteBack = 2,      // WB - write-back (not typical for MMIO, but allowed)
};

// endianness (default LE)
enum class mmio_Endianness : quint8 { LITTLE = 0, BIG };
// ============================================================================
// MMIO STATUS CODES
// ============================================================================

enum class MMIOStatus : quint8 {
	OK = 0x0,                 // Success
	ALIGNMENT_FAULT,     // Unaligned access
	ALIGN_FAULT,         // Misaligned access (should never reach handler)
	BUS_ERROR,           // Fatal bus error (machine check)
	DEVICE_ERROR,        // IRQ failure is a device error
	IMR_FAULT,            // Interrupt masked (not really a fault)
	INVALID_ADDRESS,     // No device at address
	IRQ_FAULT,           // IRQ posting failed 
	PERMISSION_DENIED,   // Read-only/write-only violation
	READ_ONLY,           // Write attempted on read-only register
	SIZE_VIOLATION,      // Wrong access size
	TIMEOUT,             // Device did not respond
	UNIMPL,              // Register not implemented
	WIDTH_FAULT,         // Unsupported width (should never reach handler)
	WRITE_ONLY          // Read attempted on write-only register
};

enum class mmio_AllocationResult : quint8 {
	SUCCESS = 0x0,
	MMIO_EXHAUSTED,
	IRQ_EXHAUSTED,
	TEMPLATE_NOT_FOUND,
	CRITICAL_FAILURE,	// Critical Failure - Unspecified
	FATAL_BOOT_ABORT,	// Initialization failed
	DMA_NOT_SUPPORTED,	// DMA requirements cannot be met
	DEGRADED			// Cascading failure
};

enum class mmio_Reason : quint8 {
	OK = 0x0,					  // Message Alignment
	MMIO_EXHAUSTED,       // No MMIO32/64 space available
	IRQ_EXHAUSTED,        // No IRQ vectors available
	TEMPLATE_NOT_FOUND,   // Message Alignment
	DMA_UNSUPPORTED,      // DMA requirements cannot be met
	INIT_FAILED,          // Device init() returned error
	PARENT_DISABLED       // Parent controller disabled (cascading failure)
};

namespace mmioSpace_VectorPolicy {
	// ========================================================================
	// Alpha AXP Vector Space Allocation
	// ========================================================================
	// Based on Alpha Architecture:
	// - PAL exceptions:      0x000-0x0FF (machine check, faults, traps)
	// - Software interrupts: 0x100-0x10F (IPL levels 1-15, mapped to vectors)
	// - AST delivery:        0x200-0x20F (AST levels 0-15)
	// - Device interrupts:   0x400+      (hardware IRQs)
	// ========================================================================

	// PAL Exceptions (reserved by hardware/PALcode)
	static constexpr quint32 EXC_BASE = 0x000;
	static constexpr quint32 EXC_LIMIT = 0x100;   // 0x00-0xFF (256 vectors)

	// Software Interrupts (IPL 0-15 mapped to vectors)
	static constexpr quint32 SWI_BASE = 0x100;   //  Start AFTER exceptions
	static constexpr quint32 SWI_LIMIT = 0x110;   // Only 16 vectors (0x100-0x10F)

	// AST Delivery Vectors
	static constexpr quint32 AST_BASE = 0x200;
	static constexpr quint32 AST_LIMIT = 0x210;   // 16 AST levels (0x200-0x20F)

	// Device Interrupts (hardware)
	static constexpr quint32 DEVICE_BASE = 0x400;
	static constexpr quint32 DEVICE_LIMIT = 0x800;

	// ========================================================================
	// Reserved Vector Check
	// ========================================================================
	// Returns true if vector is reserved by hardware/firmware and cannot
	// be registered by user code
	// ========================================================================

	inline bool isReserved(quint32 vec) {
		// Only PAL exceptions are truly "reserved" (cannot be registered)
		// SWI, AST, and DEVICE vectors are allocatable
		return (vec >= EXC_BASE && vec < EXC_LIMIT);
	}

	// ========================================================================
	// Vector Type Classification
	// ========================================================================

	inline bool isException(quint32 vec) {
		return (vec >= EXC_BASE && vec < EXC_LIMIT);
	}

	inline bool isSoftwareInterrupt(quint32 vec) {
		return (vec >= SWI_BASE && vec < SWI_LIMIT);
	}

	inline bool isAST(quint32 vec) {
		return (vec >= AST_BASE && vec < AST_LIMIT);
	}

	inline bool isDeviceInterrupt(quint32 vec) {
		return (vec >= DEVICE_BASE && vec < DEVICE_LIMIT);
	}

	// ========================================================================
	// Vector Allocation Helpers
	// ========================================================================

	// Convert IPL level (1-15) to SWI vector
	inline quint32 iplToSwiVector(int ipl) {
		return SWI_BASE + static_cast<quint32>(ipl);
	}

	// Convert SWI vector back to IPL level
	inline int swiVectorToIpl(quint32 vec) {
		return static_cast<int>(vec - SWI_BASE);
	}
}



// ============================================================================
// REGION ATTRIBUTES (enforced by MMIOManager)
// ============================================================================

struct RegionAttributes {
	quint64 minAlignment = 1;                    // Minimum access alignment (bytes)
	quint32 supportedWidths = 0x0F;        // Bitmask: 1,2,4,8 bytes
	mmio_CachePolicy cachePolicy = mmio_CachePolicy::Uncacheable;
	bool sideEffectOnRead = false;               // Read-to-clear, FIFO pop, etc.
	bool sideEffectOnWrite = true;               // Typical for device registers
	bool stronglyOrdered = false;                // Force per-access serialization

	mmio_Endianness regEndian = mmio_Endianness::LITTLE;
};

// ============================================================================
// REGION DESCRIPTOR (registration payload)
// ============================================================================

struct RegionDescriptor {
	quint32 deviceUid;                           // Stable device UID
	quint64 basePA;                              // Physical address base
	quint64 size;                                // Region size (bytes)
	RegionAttributes attrs;                      // Access attributes
	QString debugName;                           // "PKA0_BAR0", "EWA0_CSR"
};


// ============================================================================
// REGION QUERY RESULT
// ============================================================================
struct RegionQueryResult {
	bool isMmio = false;
	quint16 deviceId = 0;
	quint64 localOffset = 0;
	RegionAttributes attrs;
};

// ============================================================================
// REGION ATTRIBUTES & CAPABILITIES
// ============================================================================

using mmio_AllowedWidths = quint32;  // 0x01,0x02,0x04,0x08



enum class mmio_PostingMode : quint8 {
	Synchronous = 0,  // All writes complete before returning
	Posted = 1,       // Writes buffered, must be drained explicitly
	Auto = 2,         // Per-region default (typically Synchronous)
};

enum class mmio_DeviceClass : quint8 {
	INVALID = 0,
	UNKNOWN,

	// Controllers
	SCSI_HBA,
	IDE_CONTROLLER,
	NIC,

	// Child devices
	SCSI_DISK,
	SCSI_TAPE,
	SCSI_CDROM,
	SCSI_CONTROLLER,
	IDE_DISK,
	IDE_CDROM,
	UART_CONSOLE,
	UART,
	// Infrastructure
	HOST_BRIDGE_NODE,
	BRIDGE
};

// ============================================================================
// LIFECYCLE FLAGS
// ============================================================================

struct LifecycleState {
	bool enabled = true;    // Config says to use this device
	bool probed = false;    // Resources assigned, registered
	bool started = false;   // Device init() completed successfully
};

// ============================================================================
// HOSE STRUCTURE (APERTURE + IRQ DOMAIN)
// ============================================================================

struct Hose {
	quint16 hoseId;

	// MMIO apertures (32-bit and 64-bit spaces)
	struct Aperture {
		quint64 base;        // Start of aperture
		quint64 size;        // Total size
		quint64 cursor;      // Current allocation pointer (monotonic)
		quint64 allocated;   // Bytes allocated so far

		bool hasSpace(quint64 requestedSize, quint64 alignment) const {
			quint64 alignedCursor = (cursor + alignment - 1) & ~(alignment - 1);
			return (alignedCursor + requestedSize) <= (base + size);
		}

		// Example:
		// cursor = 0xF8000100, alignment = 0x1000 (4KB)
		// alignedCursor = (0xF8000100 + 0xFFF) & ~0xFFF
		//               = 0xF80010FF & 0xFFFFF000
		//  
	};

	Aperture mmio32;  // 32-bit MMIO space
	Aperture mmio64;  // 64-bit MMIO space (optional)

	// IRQ domain (per-hose vector space)
	struct IrqDomain {
		quint32 base;        // Start of vector range (e.g., 0x300)
		quint32 limit;       // End of vector range (e.g., 0x400)
		quint32 cursor;      // Current allocation pointer
		int allocated;       // Vectors allocated so far

		bool hasSpace() const {
			return cursor < limit;
		}

		int remaining() const {
			return (cursor < limit) ? (limit - cursor) : 0;
		}
	};

	IrqDomain irqDomain;

	// Constructor
	Hose(quint16 id) : hoseId(id) {
		mmio32.base = 0;
		mmio32.size = 0;
		mmio32.cursor = 0;
		mmio32.allocated = 0;

		mmio64.base = 0;
		mmio64.size = 0;
		mmio64.cursor = 0;
		mmio64.allocated = 0;

		irqDomain.base = 0;
		irqDomain.limit = 0;
		irqDomain.cursor = 0;
		irqDomain.allocated = 0;
	}

	// Debug string
	QString toString() const {
		return QString("Hose %1: MMIO32=[0x%2-0x%3, used=%4], "
			"MMIO64=[0x%5-0x%6, used=%7], "
			"IRQ=[0x%8-0x%9, used=%10]")
			.arg(hoseId)
			.arg(mmio32.base, 16, 16, QChar('0'))
			.arg(mmio32.base + mmio32.size, 16, 16, QChar('0'))
			.arg(mmio32.allocated)
			.arg(mmio64.base, 16, 16, QChar('0'))
			.arg(mmio64.base + mmio64.size, 16, 16, QChar('0'))
			.arg(mmio64.allocated)
			.arg(irqDomain.base, 3, 16, QChar('0'))
			.arg(irqDomain.limit, 3, 16, QChar('0'))
			.arg(irqDomain.allocated);
	}
};


// ============================================================================
// MMIO WINDOW DESCRIPTOR (per BAR)
// ============================================================================

struct MMIOWindow {
	// ========================================================================
	// IDENTITY
	// ========================================================================
	quint8 barIndex;              // Physical BAR number (0-5 for PCI)
	QString name;                 // Optional alias ("registers", "buffers")

	// ========================================================================
	// ALLOCATED RESOURCES (FILLED BY RESOURCE ALLOCATOR)
	// ========================================================================
	quint64 basePA = 0;           // Assigned physical address
	quint64 size = 0;             // Actual allocated size
	quint64 endPA = 0;			  // nodeWindow.basePA + nodeWindow.size;
	// ========================================================================
	// ALLOCATION CONSTRAINTS
	// ========================================================================
	quint64 minAlignment = 4096;  // Minimum alignment requirement
	bool is64Bit = false;         // 64-bit BAR (uses 64-bit aperture)?
	bool prefetchable = false;    // Prefetchable memory (ROM BARs)

	// ========================================================================
	// ACCESS ATTRIBUTES (COPIED FROM BAR TEMPLATE)
	// ========================================================================
	quint8 allowedWidths = 0x0F;  // Bitmask: 0x01=byte, 0x02=word, 0x04=long, 0x08=quad
	bool stronglyOrdered = false; // Serialize all accesses in MMIOManager
	bool sideEffectOnRead = false;  // Read has side-effects (FIFO pop, clear-on-read)
	bool sideEffectOnWrite = false; // Write has side-effects (doorbell, FIFO push)
	mmio_Endianness regEndian = mmio_Endianness::LITTLE;  // Register endianness

	// ========================================================================
	// OPTIONAL/RARELY USED
	// ========================================================================
	mmio_CachePolicy cachePolicy = mmio_CachePolicy::Uncacheable;  // Cache policy hint
	bool ioSpace = false;         // Port I/O space (rare on Alpha)
};



struct DegradedDeviceEntry {
	QString deviceName;       // "PKB0", "EWA1", etc.
	QString location;         // "cab0/drw0/io0/hose0/bus2/slot3"
	mmio_DeviceClass deviceClass;
	mmio_Reason reason; 
	QString details;          // Human-readable explanation
	QDateTime   m_degradeDeviceTimestamp;
};


// ============================================================================
// DEGRADED DEVICE TRACKING
// ============================================================================

struct DegradedDeviceInfo {
	QString name;                    // Device name (e.g., "PKA0")
	QString location;                // Formatted location (e.g., "hose 0 @ 00:03.0")
	mmio_DeviceClass deviceClass;         // Device class
	QString resolvedTemplate;        // Template that was resolved (if any)
	QString reason;                  // Why degraded (e.g., "BAR allocation failed")
	QDateTime timestamp;             // When degraded
};

// ============================================================================
// RESOURCE ALLOCATION RESULT
// ============================================================================

struct AllocationStatus {
	mmio_AllocationResult result;
	QString errorMessage;
	quint32 failedDeviceUid;  // UID of device that failed (if any)

	bool isSuccess() const { return result == mmio_AllocationResult::SUCCESS; }
	bool isCriticalFailure() const { return result == mmio_AllocationResult::CRITICAL_FAILURE; }
	bool isDegraded() const { return result == mmio_AllocationResult::DEGRADED; }

	AllocationStatus()
		: result(mmio_AllocationResult::SUCCESS)
		, failedDeviceUid(0)
	{
	}


};