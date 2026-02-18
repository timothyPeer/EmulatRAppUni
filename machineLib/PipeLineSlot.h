// ============================================================================
// PipeLineSlot.h - Pipeline Slot Structure
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// 
// Description:
//   Pipeline slot structure for decoded instruction processing.
//   Contains all state and side effects during instruction execution
//   through the 6-stage Alpha pipeline.
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef PIPELINE_SLOT_H
#define PIPELINE_SLOT_H

#include "../coreLib/types_core.h"
#include "../faultLib/fault_core.h"
#include "../CBoxLib/cBox_core.h"
#include "../cpuCoreLib/registerBank_coreFramework.h"
#include "../grainFactoryLib/DecodedInstruction.h"
#include "../coreLib/MemoryBarrierKind_enum.h"
#include "../faultLib/FaultDispatcher.h"
#include "../faultLib/PendingEvent_Refined.h"

#include "../palLib_EV6/PAL_core.h"
#include "coreLib/global_RegisterMaster_hot.h"
#include "faultLib/GlobalFaultDispatcherBank.h"


// ============================================================================
// Forward Declarations
// ============================================================================

struct InstructionGrain;
struct DecodedInstruction;
class CBox;
class MBox;
class EBox;
class FBox;
class PalBox;


// ============================================================================
// PAL Decode Structure
// ============================================================================
struct decoded_Pal {
	quint32 palFunction; // PAL function code for CALL_PAL instructions
};

// ============================================================================
// Pipeline Enums
// ============================================================================
enum class PipelineStage : quint8
{
	Empty = 0,
	IFetch = 1,
	Decode = 2,
	Issue = 3,
	Execute = 4,
	MemAccess = 5,
	Writeback = 6,
	Retire = 7
};

enum class ExecUnit : quint8
{
	None = 0,
	EBOX = 1,
	MBOX = 2,
	FBOX = 3,
	PALBOX = 4,
	IBOX = 5,
	CBOX = 7
};


#pragma region Deferred WriteBack


// ============================================================================
// DEFERRED WRITEBACK - Pipeline Hazard Avoidance
// ============================================================================
//
// DESIGN:
//   Register writes are DEFERRED by one cycle. The instruction that
//   computes a result in EX stores it in m_pending. The NEXT cycle,
//   commitPending() writes it to the register file BEFORE any new
//   instruction reads registers in EX.
//
//   Cycle N:   EX executes instr A -> result stored in m_pending
//   Cycle N+1: commitPending() writes A's result -> register file updated
//              EX executes instr B -> reads correct value from A
//              B's result stored in m_pending
//
// WHY:
//   - No back-scanning the pipeline in WB
//   - No forwarding muxes or scoreboard
//   - O(1) cost per cycle: one compare + one register write
//   - On flush: pending from the older (valid) instruction is committed,
//     the faulting instruction never reached deferWriteback(), so nothing
//     is corrupted
//
// RULE:
//   Only stage_WB() calls commitPending() (top of cycle, runs first).
//   Only stage_EX() calls deferWriteback() (end of execution).
//   flush() calls commitPending() then clears m_pending.
//
// ============================================================================

// --- Add to AlphaPipeline private members ---

struct PendingCommit
{
	bool    intValid{ false };
	quint8  intReg{ 0 };
	quint64 intValue{ 0 };
	bool    intClearDirty{ false };   // EBox scoreboard

	bool    fpValid{ false };
	quint8  fpReg{ 0 };
	quint64 fpValue{ 0 };
	bool    fpClearDirty{ false };    // FBox scoreboard
	bool    isValid() const noexcept { return (intValid || fpValid); }

#if AXP_INSTRUMENTATION_TRACE

	// Instrumentation
	quint8  destReg{ 0 };
	quint64 value{ 0 };
	quint64 instrPC{ 0 };
#endif
};


#pragma endregion Deferred WriteBack

// ============================================================================
// Pipeline Slot Structure
// ============================================================================
struct  PipelineSlot
{
	// ====================================================================
	// Instruction Fields
	// ====================================================================
	DecodedInstruction di;
	const InstructionGrain* grain = nullptr;
	quint32 instructionWord{ 0 };
	decoded_Pal palDecoded;

	// ====================================================================
	// Execution Context
	// ====================================================================
	ExecUnit execUnit = ExecUnit::None;
	CPUIdType cpuId{ 0 };
	PendingEvent faultEvent{};

	// ====================================================================
	// Stage Tracking
	// ====================================================================
	PipelineStage stage = PipelineStage::Empty;
	bool valid = false;
	bool stalled = false;
	bool enterPalMode = false;
	bool needsWriteback{ false };
	bool dualIssued = false;
	quint8 currentStage = 0;

	// ====================================================================
	// Exception / Trap / PAL State
	// ====================================================================
	bool faultPending = false;
	TrapCode_Class trapCode;
	quint64 faultVA = 0;
	quint64 targetPALVector = 0;
	quint64 reiTarget{ 0 };
	bool serialized{ false };
	bool mustComplete{ false };

	// ====================================================================
	// Address State
	// ====================================================================
	quint64 va{ 0 };
	quint64 pa{ 0 };
	quint64 ra{ 0 };
	quint64 outPAData{ 0 };
	bool pcModified{ false };		// flag to track PC changes in the pipeline
	bool writeRa{ false };			// used by PalBox::execute_MFPR
	bool writeFa{ false };			// Write to float register, not Ra

	// ====================================================================
	// Branch / Control Flow
	// ====================================================================
	bool branchTaken = false;
	bool predictionTaken = false;
	bool predictionValid = false;
	quint64 predictionTarget{ 0 };
	quint64 branchTarget = 0;
	quint64 physicalAddr = 0;

	// ====================================================================
	// PAL Transfer State
	// ====================================================================
	bool palTransferPending = false;
	PalResult palResult{};
	bool halted{ false };
	// ====================================================================
	// Memory Barriers
	// ====================================================================
	bool memoryBarrierCompleted = false; // Completion flag
	bool writeBufferDrained = false; // Completion flag
	bool needsMemoryBarrier{ false }; // Request flag
	bool needsWriteBufferDrain{ false }; // Request flag  
	SerializationType serializeType;

	// ====================================================================
	// Result Holding
	// ====================================================================
	quint64 payLoad = 0;	// PayLoad for Box side effect only
	quint64 ra_value{ 0 };	// Payload for RA register only
	quint64 slotSequence = 0;
	RegisterBankInteger::RegIndex registerIndex;

	// ====================================================================
	// Memory Pipeline State
	// ====================================================================
	bool memResultValid = false;
	bool flushPipeline{ false };

	// ====================================================================
	// Fault Handling
	// ====================================================================
	FaultDispatcher* m_faultDispatcher;

	// BOXes 

	EBox* m_eBox{ nullptr };
	FBox* m_fBox{ nullptr };
	MBox* m_mBox{ nullptr };
	PalBox* m_palBox{ nullptr };
	CBox* m_cBox{ nullptr };
	AXP_HOT AXP_ALWAYS_INLINE void injectOtherBoxes(EBox* eBox, FBox* fBox, MBox* mBox, PalBox* pBox, CBox* cBox) noexcept {
		m_eBox = eBox;
		m_fBox = fBox;
		m_mBox = mBox;
		m_palBox = pBox;
		m_cBox = cBox;
	}

	// Pending Commit Status 

	PendingCommit m_pending{};

	// ====================================================================
	// Constructors
	// ====================================================================

	PipelineSlot()
	: palDecoded{}
	, trapCode{}
	, serializeType{}
	, registerIndex{}
	, m_faultDispatcher(&globalFaultDispatcher(cpuId))
		, m_iprGlobalMaster(getCPUStateView(cpuId))
	{

	}

	// Register Global Accessors
	AXP_HOT AXP_ALWAYS_INLINE   void writeIntReg(quint8 index, quint64 argValue) const noexcept
	{
		return m_iprGlobalMaster->i->write(index, argValue);
	}

	AXP_HOT AXP_ALWAYS_INLINE  quint64 readIntReg( quint8 index) const noexcept
	{
		return m_iprGlobalMaster->i->read(index);
	}
	// Register Global Accessors
	AXP_HOT AXP_ALWAYS_INLINE   void writeFpReg(quint8 index, quint64 argValue) const noexcept
	{
		return m_iprGlobalMaster->f->write(index, argValue);
	}

	AXP_HOT AXP_ALWAYS_INLINE  quint64 readFpReg( quint8 index) const noexcept
	{
		return m_iprGlobalMaster->f->read(index);
	}


	// PalBox 

	MemoryBarrierKind barrierKind{ MemoryBarrierKind::FETCH }; // least restrictive


	// ====================================================================
	// Methods
	// ====================================================================
	AXP_HOT AXP_ALWAYS_INLINE  void clear()
	{
		di = DecodedInstruction{};
		grain = nullptr;
		execUnit = ExecUnit::None;
		stage = PipelineStage::Empty;
		valid = false;
		stalled = false;
		dualIssued = false;
		faultPending = false;
		trapCode = TrapCode_Class::ILLEGAL_INSTRUCTION;
		faultVA = 0;
		targetPALVector = 0;
		branchTaken = false;
		branchTarget = 0;
		palTransferPending = false;
		payLoad = 0;
		memResultValid = false;
		instructionWord = static_cast<quint32>(-1);
		registerIndex = RegisterBankInteger::NONE;
		va = 0;
		pa = 0;
		outPAData = 0;
		physicalAddr = 0;
		enterPalMode = false;
		memoryBarrierCompleted = false;
		writeBufferDrained = false;

		nextPC = 0;
		predictedPC = 0;
		linkValue = 0;
		jumpTarget = 0;
		branchTestValue = 0;
		mispredict = false;
		pcReason = static_cast<PCReason>(0xFF);  // sentinel - "not set"

	}

	// Context accessors
// 	AlphaProcessorContext* apc() noexcept;
// 	PalBox* getPBox() noexcept;
// 	EBox* getEBox() noexcept;
// 	MBox* getMBox() noexcept;
// 	FBox* getFBox() noexcept;

	// registers

	CPUStateView* m_iprGlobalMaster{ nullptr };

//     // PC Path Tracing fields (populated during EX/MEM stages)
     uint64_t nextPC;            // actual next PC after this instruction retires
     uint64_t predictedPC;       // branch predictor's target (if branch/jump)
     uint64_t linkValue;         // value written to Ra for BSR/JSR (return addr)
     uint64_t jumpTarget;        // actual resolved target for JSR/JMP/RET
     uint64_t branchTestValue;   // register value tested for conditional branches
     uint32_t cycle;             // cycle number when instruction was fetched
     uint32_t retireCycle;       // cycle number when instruction retired (WB)

     enum class PCReason : uint8_t {
         Sequential = 0,         // PC + 4
         Fallthrough,            // branch not taken, PC + 4
         BranchTaken,            // conditional branch taken
         BSR,                    // subroutine branch (unconditional)
         BR,                     // unconditional branch
         JSR,                    // jump to subroutine via register
         RET,                    // return via register
         JMP,                    // jump via register
         JSR_COROUTINE,          // coroutine jump
         PALEntry,               // entered PAL mode
         PALExit,                // exited PAL mode
         Mispredict,             // pipeline flush, redirect
         Exception,              // exception taken
     } pcReason;
//
     bool mispredict;            // true if pipeline was flushed

};

#endif // PIPELINE_SLOT_H
