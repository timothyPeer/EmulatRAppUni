#ifndef BOXREQUEST_H
#define BOXREQUEST_H
#include <QtGlobal>
#include "../faultLib/fault_core.h"

// ============================================================================
// BOX RESULT FLAGS (Bitwise)
// ============================================================================

enum BoxResultFlags : quint16 {
	BOX_NONE = 0x0000,
	BOX_REQUEST_MEMORY_BARRIER = 0x0001,  // Full MB
	BOX_DRAIN_WRITE_BUFFERS = 0x0002,  // WMB (lighter)
	BOX_FLUSH_PIPELINE = 0x0004,  // Branch mispredict, exception
	BOX_CLEAR_BRANCH_PREDICTOR = 0x0008,  // Context switch, PAL entry
	BOX_STALL_PIPELINE = 0x0010,  // Wait for resource
	BOX_SYNC_LLSC_RESERVATIONS = 0x0020,  // LDx_L/STx_C coordination
	BOX_FLUSH_MEMORY_BUFFERS = 0x0040,  // Flush all pending memory ops
	BOX_COMMIT_STAGED_PTE = 0x0080,
	BOX_FAULT_DISPATCHED = 0x0100,		
	BOX_ENTER_PALMODE = 0x0200,
	BOX_MISPREDICT_BRANCH_TARGET = 0x0400,
	BOX_ADVANCE = 0x0800,
	BOX_HALT_EXECUTION = 0x1000,	 // PAL HALT requested
	BOX_RETRY_INSTRUCTION = 0x2000,// PAL retry requested
	BOX_FAULT_RETIRED = 0x4000,	// Flag - faulted instruction (retired- faultStatus=true) stage_wb
	BOX_REQUEST_HALTED = 0x8000
};


// ============================================================================
// BOX RESULT STRUCTURE
// ============================================================================

struct BoxResult {
	bool pcModified = false;
	quint16 flags{ BOX_NONE };

	bool redirect{ false };
	quint64 redirectPC {0};
	quint8 palFunction{ 0 };
	// ========================================================================
	// FAULT TRACKING
	// ========================================================================
	TrapCode_Class faultClass = TrapCode_Class::NONE;
	quint64 faultingPC = 0;
	quint64 faultingVA = 0;

	// ========================================================================
	// CONSTRUCTORS
	// ========================================================================

	BoxResult() = default;

	explicit BoxResult(quint16 f) : flags(f) {}

	// ========================================================================
	// FLAG SETTERS (Fluent Interface)
	// ========================================================================
	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& requestEnterPalMode() noexcept {
		flags |= BOX_ENTER_PALMODE;
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& requestMemoryBarrier() noexcept {
		flags |= BOX_REQUEST_MEMORY_BARRIER;
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& commitStagedPTE() noexcept {
		flags |= BOX_COMMIT_STAGED_PTE;
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& faultDispatched() noexcept {
		flags |= BOX_FAULT_DISPATCHED;
		flags |= BOX_FAULT_RETIRED;					// we need to set a flag to ensure stage_WB executes.
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& drainWriteBuffers() noexcept {
		flags |= BOX_DRAIN_WRITE_BUFFERS;
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& flushPipeline() noexcept {
		flags |= BOX_FLUSH_PIPELINE;
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& misPredictBranchTarget() noexcept {
		flags |= BOX_MISPREDICT_BRANCH_TARGET;
		return *this;
	}
	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& advance() noexcept {
		flags |= BOX_ADVANCE;
		return *this;
	}
	
	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& clearBranchPredictor() noexcept {
		flags |= BOX_CLEAR_BRANCH_PREDICTOR;
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& stallPipeline() noexcept {
		flags |= BOX_STALL_PIPELINE;
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& syncLLSCReservations() noexcept {
		flags |= BOX_SYNC_LLSC_RESERVATIONS;
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& flushMemoryBuffers() noexcept {
		flags |= BOX_FLUSH_MEMORY_BUFFERS;
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& requestHalted() noexcept {
		flags |= BOX_REQUEST_HALTED;
		return *this;
	}
	//AXP_HOT AXP_ALWAYS_INLINE  BoxResult& faultedBoxRetired() noexcept { flags |= BOX_FAULT_RETIRED; return *this; }

	// ========================================================================
	// FAULT SETTERS (Fluent Interface)
	// ========================================================================
	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& setTrapCodeFaultClass(TrapCode_Class fc) noexcept {
		faultClass = fc;
		flags |= BOX_FAULT_DISPATCHED;
		flags |= BOX_FAULT_RETIRED;
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& setFaultPC(quint64 pc) noexcept {
		faultingPC = pc;
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& setFaultVA(quint64 va) noexcept {
		faultingVA = va;
		return *this;
	}

	// Convenience method to set all fault info at once
	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& setFaultInfo(TrapCode_Class fc, quint64 pc, quint64 va = 0) noexcept {
		faultClass = fc;
		faultingPC = pc;
		faultingVA = va;
		flags |= BOX_FAULT_DISPATCHED;
		flags |= BOX_FAULT_RETIRED;
		return *this;
	}
	AXP_HOT AXP_ALWAYS_INLINE BoxResult& setFaultDispatched() noexcept
	{
		clearFlag(BOX_FAULT_RETIRED);
		return *this;
	}

	// ========================================================================
	// FLAG QUERIES
	// ========================================================================

	
	AXP_HOT AXP_ALWAYS_INLINE  bool needsEnterPalmode() const noexcept {
		return (flags & BOX_ENTER_PALMODE) != 0;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool needsMemoryBarrier() const noexcept {
		return (flags & BOX_REQUEST_MEMORY_BARRIER) != 0;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool needsWriteDrain() const noexcept {
		return (flags & BOX_DRAIN_WRITE_BUFFERS) != 0;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool needsPipelineFlush() const noexcept {
		return (flags & BOX_FLUSH_PIPELINE) != 0;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool needsBranchPredictorClear() const noexcept {
		return (flags & BOX_CLEAR_BRANCH_PREDICTOR) != 0;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool needsPipelineStall() const noexcept {
		return (flags & BOX_STALL_PIPELINE) != 0;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool needsLLSCSync() const noexcept {
		return (flags & BOX_SYNC_LLSC_RESERVATIONS) != 0;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool needsMisPredictBranchTarget() const noexcept
	{
		return (flags & BOX_CLEAR_BRANCH_PREDICTOR) != 0;
	}
	AXP_HOT AXP_ALWAYS_INLINE  bool needsMemoryBufferFlush() const noexcept {
		return (flags & BOX_FLUSH_MEMORY_BUFFERS) != 0;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool needsHalted() const noexcept {
		return (flags & BOX_REQUEST_HALTED) != 0;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool needsAdvance() const noexcept
	{
		return (flags & BOX_ADVANCE) != 0;
	}
	AXP_HOT AXP_ALWAYS_INLINE  bool hasAnyFlags() const noexcept {
		return flags != BOX_NONE;
	}

	// CLEAR functions

	AXP_HOT AXP_ALWAYS_INLINE void clearFlag(quint16 flg) noexcept
	{
		flags &= ~flg;  // Clear the bit
	}
	
	// ========================================================================
	// FAULT QUERIES
	// ========================================================================

	AXP_HOT AXP_ALWAYS_INLINE  bool hasFault() const noexcept {
		return faultClass != TrapCode_Class::NONE;
	}

	AXP_HOT AXP_ALWAYS_INLINE  TrapCode_Class getFaultClass() const noexcept {
		return faultClass;
	}

	AXP_HOT AXP_ALWAYS_INLINE  quint64 getFaultPC() const noexcept {
		return faultingPC;
	}

	AXP_HOT AXP_ALWAYS_INLINE  quint64 getFaultVA() const noexcept {
		return faultingVA;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool faultWasDispatched() const noexcept { return (flags & BOX_FAULT_RETIRED) == 0; }
	// ========================================================================
	// MERGE OPERATIONS
	// ========================================================================

	AXP_HOT AXP_ALWAYS_INLINE  void merge(const BoxResult& other) noexcept {
		flags |= other.flags;
		// Fault info: keep first fault encountered
		if (faultClass == TrapCode_Class::NONE && other.faultClass != TrapCode_Class::NONE) {
			faultClass = other.faultClass;
			faultingPC = other.faultingPC;
			faultingVA = other.faultingVA;
		}
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult operator|(const BoxResult& other) const noexcept {
		BoxResult result(flags | other.flags);
		result.pcModified = pcModified || other.pcModified;

		// Keep first fault
		if (faultClass != TrapCode_Class::NONE) {
			result.faultClass = faultClass;
			result.faultingPC = faultingPC;
			result.faultingVA = faultingVA;
		}
		else if (other.faultClass != TrapCode_Class::NONE) {
			result.faultClass = other.faultClass;
			result.faultingPC = other.faultingPC;
			result.faultingVA = other.faultingVA;
		}

		return result;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& operator|=(const BoxResult& other) noexcept {
		merge(other);
		pcModified = pcModified || other.pcModified;
		return *this;
	}

	// ========================================================================
	// UTILITIES
	// ========================================================================

	AXP_HOT AXP_ALWAYS_INLINE  void clear() noexcept {
		flags = BOX_NONE;
		pcModified = false;
		faultClass = TrapCode_Class::NONE;
		faultingPC = 0;
		faultingVA = 0;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool isEmpty() const noexcept {
		return flags == BOX_NONE && faultClass == TrapCode_Class::NONE;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool needsHalt() const noexcept {
		return (flags & BOX_HALT_EXECUTION) != 0;
	}

	AXP_HOT AXP_ALWAYS_INLINE  bool needsRetry() const noexcept {
		return (flags & BOX_RETRY_INSTRUCTION) != 0;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& requestHalt() noexcept {
		flags |= BOX_HALT_EXECUTION;
		return *this;
	}

	AXP_HOT AXP_ALWAYS_INLINE  BoxResult& requestRetry() noexcept {
		flags |= BOX_RETRY_INSTRUCTION;
		return *this;
	}
};

#endif // BOXREQUEST_H