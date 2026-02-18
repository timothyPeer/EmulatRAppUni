#ifndef MemoryFaultInfo_h__
#define MemoryFaultInfo_h__

// ============================================================================
// ASA Emulator (c) 2025 Timothy Peer / eNVy Systems, Inc.  (Non-Commercial)
// ============================================================================
// -.H  -- Backward-compatible fault info record
//
// GOAL
// -----
// Provide a *layout-compatible* replacement for your legacy -
// while extending it with Alpha-specific context (ITB/DTB split, FP trap kind,
// opcode/permission detail, mode capture, etc.).  The first 8 fields keep
// the *exact* order, names, and types you used previously so brace-init sites
// keep compiling unchanged.
//
// LEGACY ORDER (kept):
//   1) MemoryFaultType faultType
//   2) quint64         faultAddress
//   3) quint64         physicalAddress
//   4) int             accessSize                // size in bytes
//   5) bool            isWrite
//   6) bool            isExecute
//   7) quint64         pc
//   8) quint32         instruction               // raw 32-bit instruction word
//
// EXTENSIONS (appended; optional for producers; useful to PAL/OS):
//   - MemoryAccessType, MemoryAccessSize enum mirror, TbDomain
//   - FP arithmetic trap kind (INV/DZE/OVF/UNF/INE), opcode fault kind
//   - PermissionDetail (which check failed), current CPU mode (K/E/S/U)
//   - translationValid, inPALmode, deviceId, errorCode
//
// AAH REFERENCES (generic, stable across printings):
//    AAH Vol. I: Exceptions & Interrupts (TB miss/fault vectors, EXC_SUM)
//    AAH: Memory Management (PTE perms KRE/KWE/ERE/EWE/SRE/SWE/URE/UWE)
//    AAH: Floating-point Exceptions & FPCR (INV/DZE/OVF/UNF/INE, FEN)
//
// ============================================================================

#include <QtGlobal>
#include "../memoryLib/memory_core.h"
#include "fault_core.h"
#include "memory_enums_structs.h"
#include "pteLib/alpha_pte_core.h"

// ============================================================================
//  Unified MemoryFaultInfo
//
//  - Contains all fields used in BOTH old structures.
//  - No duplicates.
//  - Stable binary layout.
//  - Used throughout memory system and PAL exception path.
// ============================================================================
struct alignas(8) MemoryFaultInfo {

	// ------------------------------------------------------------------------
	// LEGACY HEAD (kept EXACTLY so all brace-init sites still compile)
	// ------------------------------------------------------------------------
	MemoryFaultType faultType;
	quint64         faultAddress{ 0 };
	quint64			faultingVA{ 0 };
	quint64         physicalAddress{ 0 };

	int             accessSize{ 0 };        // BYTE/WORD/LONG/QUAD
	bool            isWrite{ false };
	bool            isExecute{ false };

	quint64         faultingPC{ 0 };
	quint32         instruction{ 0 };

	// ------------------------------------------------------------------------
	// EV6 EXTENDED TAIL
	// ------------------------------------------------------------------------
	MemoryAccessType   accessTypeEx{ MemoryAccessType::READ };
	MemoryAccessSize   accessSizeEx{ MemoryAccessSize::QUADWORD };

	Realm tbDomain;

	ArithmeticTrapKind arithmeticKind{ ArithmeticTrapKind::NONE };
	OpcodeFaultKind     opcodeKind{ OpcodeFaultKind::NONE };
	PermissionDetail    permDetail{ PermissionDetail::NONE };

	bool                translationValid{ false };
	bool                inPALmode{ false };
	quint8              currentMode{ 0 };   // K=0, E=1, S=2, U=3
	ASNType				asn{ 0 }; // ASN Value
	quint32             deviceId{ 0 };
	quint32             errorCode{ 0 };

	// ------------------------------------------------------------------------
	// Derived helpers
	// ------------------------------------------------------------------------

	MemoryAccessType canonicalAccessType() const {
		if (accessTypeEx != MemoryAccessType::READ)
			return accessTypeEx;
		if (isExecute) return MemoryAccessType::EXECUTE;
		return isWrite ? MemoryAccessType::WRITE : MemoryAccessType::READ;
	}

	MemoryAccessSize canonicalAccessSize() const {
		if (accessSizeEx != MemoryAccessSize::QUADWORD)
			return accessSizeEx;

		switch (accessSize) {
		case 1:  return MemoryAccessSize::BYTE;
		case 2:  return MemoryAccessSize::WORD;
		case 4:  return MemoryAccessSize::LONGWORD;
		case 8:  return MemoryAccessSize::QUADWORD;
		case 16: return MemoryAccessSize::OCTAWORD;
		default: return MemoryAccessSize::QUADWORD;
		}
	}

	Realm canonicalTBDomain() const {
		return isExecute
			? Realm::I
			: Realm::D;
	}

	bool isNaturallyAligned() const {
		const quint64 bytes = (accessSize > 0 ? accessSize : 8);
		return (faultAddress & (bytes - 1ULL)) == 0;
	}

	bool isTbEvent() const {
		return canonicalTBDomain() == Realm::I ||
			canonicalTBDomain() == Realm::D;
	}

	bool requiresPALcode() const {
		switch (faultType) {
		case MemoryFaultType::ITB_MISS:
        case MemoryFaultType::DTB_FAULT_READ:
        case MemoryFaultType::DTB_MISS_READ:
		case MemoryFaultType::DTB_FAULT_WRITE:
		case MemoryFaultType::ITB_FAULT:
		case MemoryFaultType::ALIGNMENT_FAULT:
		case MemoryFaultType::FEN_FAULT:
		case MemoryFaultType::OPCODE_FAULT:
		case MemoryFaultType::MACHINE_CHECK:
		case MemoryFaultType::BREAKPOINT:
		case MemoryFaultType::BUGCHECK:
			return true;
		default:
			return false;
		}
	}

	// ------------------------------------------------------------------------
	// Constructors
	// ------------------------------------------------------------------------
	static MemoryFaultInfo makeLegacy(
		MemoryFaultType t,
		quint64 va,
		int sizeBytes,
		bool write,
		bool exec,
		quint64 programCounter,
		quint32 instr,
		quint64 pa = 0
	) {
		MemoryFaultInfo info;
		info.faultType = t;
		info.faultAddress = va;
		info.physicalAddress = pa;
		info.accessSize = sizeBytes;
		info.isWrite = write;
		info.isExecute = exec;
		info.faultingPC = programCounter;
		info.instruction = instr;
		return info;
	}

	static MemoryFaultInfo makeExtended(
		MemoryFaultType t,
		quint64 va,
		quint64 pa,
		int sizeBytes,
		MemoryAccessType aType,
		MemoryAccessSize aSizeEnum,
		Realm domain,
		bool write,
		bool exec,
		quint64 programCounter,
		quint32 instr32,
		bool xValid,
		bool palMode,
		quint8 modeKESU,
		PermissionDetail perm,
		ArithmeticTrapKind fpKind,
		OpcodeFaultKind opcKind,
		quint32 devId = 0,
		quint32 err = 0
	) {
		MemoryFaultInfo info;
		info.faultType = t;
		info.faultAddress = va;
		info.physicalAddress = pa;
		info.accessSize = sizeBytes;
		info.isWrite = write;
		info.isExecute = exec;
		info.faultingPC = programCounter;
		info.instruction = instr32;

		info.accessTypeEx = aType;
		info.accessSizeEx = aSizeEnum;
		info.tbDomain = domain;
		info.arithmeticKind = fpKind;
		info.opcodeKind = opcKind;
		info.permDetail = perm;
		info.translationValid = xValid;
		info.inPALmode = palMode;
		info.currentMode = modeKESU;
		info.deviceId = devId;
		info.errorCode = err;
		return info;
	}
};




// ============================================================================
// MEMORY OPERATION RESULT - Complete result from memory operation
// ============================================================================
// Returned by all memory subsystem operations.
//
struct MemoryOperationResult
{
	MemoryStatus     status{ MemoryStatus::SUCCESS };
	MemoryFaultInfo  faultInfo;               // Valid when status == FAULT
	quint64          data{ 0 };              // Data read (for READ/EXECUTE)
	quint32          cyclesTaken{ 0 };       // Optional: emulated cycle budget

	// Quick checks
	bool isSuccess()  const { return status == MemoryStatus::SUCCESS; }
	bool isFault()    const { return status == MemoryStatus::FAULT; }
	bool needsRetry() const { return status == MemoryStatus::RETRY; }
	bool isPending()  const { return status == MemoryStatus::PENDING; }
};
#endif // MemoryFaultInfo_h__
