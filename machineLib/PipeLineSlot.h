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

#include "coreLib/types_core.h"
#include "faultLib/fault_core.h"
#include "CBoxLib/cBox_core.h"
#include "cpuCoreLib/registerBank_coreFramework.h"
#include "grainFactoryLib/DecodedInstruction.h"
#include "coreLib/MemoryBarrierKind_enum.h"
#include "faultLib/FaultDispatcher.h"
#include "faultLib/PendingEvent_Refined.h"

#include "palLib_EV6/PAL_core.h"
#include "coreLib/global_RegisterMaster_hot.h"
#include "faultLib/GlobalFaultDispatcherBank.h"
#include <grainFactoryLib/InstructionGrain.h>


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
	//quint64 ra{ 0 };
	quint64 outPAData{ 0 };
	bool pcModified{ false };		// flag to track PC changes in the pipeline
	//bool writeRa{ false };			// used by PalBox::execute_MFPR
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
	quint64 rb_value{ 0 };	// Payload for RB register only
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


	// =====================================================================
	// Trace Accessors
	// =====================================================================

	// ============================================================================
	// PipelineSlot trace accessor methods
	// Add these inside the PipelineSlot struct, after the existing Methods region.
	// Guard with AXP_EXEC_TRACE so they compile away in release builds.
	// ============================================================================

#ifdef AXP_EXEC_TRACE

	// ========================================================================
	// getOperandsString()
	// Returns a DEC ASM style operand string based on instruction format.
	//
	// Memory:   "R16, 0x8(R2)"
	// Operate:  "R1, R2, R3"       (register form)
	//           "R1, #0x20, R3"    (literal form)
	// Branch:   "R4, 0x900260"
	// PAL:      "func=0x0083"
	// ========================================================================
	QString getOperandsString() const noexcept
	{
		if (!grain)
			return QStringLiteral("?");

		const quint32 raw = di.rawBits();
		const quint8  ra = di.ra;
		const quint8  rb = di.rb;
		const quint8  rc = di.rc;
		const quint8  flags = grain->flags();

		// PAL format
		if (flags & GF_PALFormat)
		{
			// HW_LD / HW_ST: Ra, disp(Rb)
			const quint8 op = grain->opcode();
			if (op == 0x1B || op == 0x1F)
			{
				const qint64 disp = static_cast<qint64>(
					static_cast<qint32>((raw & 0xFFF) << 20) >> 20);
				return QString("R%1, 0x%2(R%3)")
					.arg(ra)
					.arg(static_cast<quint64>(disp) & 0xFFF, 3, 16, QChar('0'))
					.arg(rb);
			}
			// HW_MFPR / HW_MTPR
			if (op == 0x19 || op == 0x1D)
			{
				const quint16 ipr = static_cast<quint16>(raw & 0xFF);
				return QString("R%1, IPR=0x%2")
					.arg(ra)
					.arg(ipr, 2, 16, QChar('0'));
			}
			// CALL_PAL
			if (op == 0x00)
				return QString("func=0x%1").arg(raw & 0x03FFFFFF, 8, 16, QChar('0'));

			return QString("0x%1").arg(raw, 8, 16, QChar('0'));
		}

		// Branch format: Ra, target
		if (flags & GF_BranchFormat)
		{
			// JSR/JMP/RET -- Ra, (Rb)
			if (grain->opcode() == 0x1A)
			{
				return QString("R%1, (R%2)").arg(ra).arg(rb);
			}
			// BSR/BR write link -- show target
			return QString("R%1, 0x%2")
				.arg(ra)
				.arg(branchTarget, 16, 16, QChar('0'));
		}

		// Memory format: Ra, disp(Rb)
		if (flags & GF_MemoryFormat)
		{
			const qint16 disp = static_cast<qint16>(raw & 0xFFFF);
			return QString("R%1, %2%3(R%4)")
				.arg(ra)
				.arg(disp < 0 ? "-" : "")
				.arg(static_cast<quint16>(disp < 0 ? -disp : disp), 0, 16)
				.arg(rb);
		}

		// Operate format
		if (flags & GF_OperateFormat)
		{
			const bool literalForm = (raw >> 12) & 1;
			if (literalForm)
			{
				const quint8 lit = static_cast<quint8>((raw >> 13) & 0xFF);
				return QString("R%1, #0x%2, R%3")
					.arg(ra)
					.arg(lit, 2, 16, QChar('0'))
					.arg(rc);
			}
			return QString("R%1, R%2, R%3").arg(ra).arg(rb).arg(rc);
		}

		// Fallback -- raw hex
		return QString("0x%1").arg(raw, 8, 16, QChar('0'));
	}

	// ========================================================================
	// getResultString()
	// Returns a DEC ASM style result string showing what changed.
	//
	// Integer writeback:  "R16 = 0x0000000000900018"
	// FP writeback:       "F16 = 0x3FF0000000000000"
	// Branch taken:       "-> 0x0000000000900260"
	// Branch not taken:   ""
	// Store / no result:  ""
	// ========================================================================
	QString getResultString() const noexcept
	{
		// Branch result
		if (branchTaken)
			return QString("-> 0x%1").arg(branchTarget, 16, 16, QChar('0'));

		// No writeback -- store, barrier, or branch not taken
		if (!needsWriteback)
			return QString();

		// FP writeback
		if (writeFa)
		{
			const quint8 dest = di.ra;
			return QString("F%1 = 0x%2")
				.arg(dest)
				.arg(payLoad, 16, 16, QChar('0'));
		}

		// Integer writeback -- destination is rc for operate, ra for load/branch
		const quint8 flags = grain ? grain->flags() : 0;
		quint8 dest = 0;

		if (flags & GF_MemoryFormat)
			dest = di.ra;       // load destination
		else if (flags & GF_BranchFormat)
			dest = di.ra;       // link register
		else
			dest = di.rc;       // operate destination

		return QString("R%1 = 0x%2")
			.arg(dest)
			.arg(payLoad, 16, 16, QChar('0'));
	}

#else // AXP_EXEC_TRACE

	// Release build stubs -- return empty strings, optimised away
	AXP_HOT AXP_ALWAYS_INLINE
		QString getOperandsString() const noexcept { return {}; }

	AXP_HOT AXP_ALWAYS_INLINE
		QString getResultString() const noexcept { return {}; }

#endif // AXP_EXEC_TRACE





	// ============================================================================
	// Register Global Accessors
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE void writeIntReg(quint8 index, quint64 argValue) const noexcept
	{
#ifdef AXP_DEBUG
		const quint64 oldValue = m_iprGlobalMaster->i->read(index);
		qDebug().noquote()
			<< QString("[REG::WRITE::INT] CPU=%1 PC=0x%2 R%3: 0x%4 -> 0x%5")
			.arg(cpuId)
			.arg(di.pc, 16, 16, QChar('0'))
			.arg(index)
			.arg(oldValue, 16, 16, QChar('0'))
			.arg(argValue, 16, 16, QChar('0'));
#endif
		return m_iprGlobalMaster->i->write(index, argValue);
	}

	AXP_HOT AXP_ALWAYS_INLINE void writeFpReg(quint8 index, quint64 argValue) const noexcept
	{
#ifndef AXP_INSTRUMENTATION_TRACE
		const quint64 oldValue = m_iprGlobalMaster->f->read(index);
		qDebug().noquote()
			<< QString("[REG::WRITE::FP ] CPU=%1 PC=0x%2 F%3: 0x%4 -> 0x%5")
			.arg(cpuId)
			.arg(di.pc, 16, 16, QChar('0'))
			.arg(index)
			.arg(oldValue, 16, 16, QChar('0'))
			.arg(argValue, 16, 16, QChar('0'));
#endif
		return m_iprGlobalMaster->f->write(index, argValue);
	}
	AXP_HOT AXP_ALWAYS_INLINE  quint64 readFpReg( quint8 index) const noexcept
	{
		return m_iprGlobalMaster->f->read(index);
	}
	AXP_HOT AXP_ALWAYS_INLINE  quint64 readIntReg(quint8 index) const noexcept
	{
		return m_iprGlobalMaster->i->read(index);
	}

	// PalBox 

	MemoryBarrierKind barrierKind{ MemoryBarrierKind::FETCH }; // least restrictive


	// ====================================================================
	// Methods
	AXP_HOT AXP_ALWAYS_INLINE void clear()
	{
		di = DecodedInstruction{};
		grain = nullptr;
		instructionWord = static_cast<quint32>(-1);
		palDecoded = decoded_Pal{};
		execUnit = ExecUnit::None;
		// cpuId -- intentionally NOT cleared (per-CPU constant)
		faultEvent = PendingEvent{};
		stage = PipelineStage::Empty;
		valid = false;
		stalled = false;
		enterPalMode = false;
		needsWriteback = false;
		dualIssued = false;
		currentStage = 0;
		faultPending = false;
		trapCode = TrapCode_Class::ILLEGAL_INSTRUCTION;
		faultVA = 0;
		targetPALVector = 0;
		reiTarget = 0;
		serialized = false;
		mustComplete = false;
		va = 0;
		pa = 0;
		//ra = 0;
		outPAData = 0;
		pcModified = false;
		//writeRa = false;
		writeFa = false;
		branchTaken = false;
		predictionTaken = false;
		predictionValid = false;
		predictionTarget = 0;
		branchTarget = 0;
		physicalAddr = 0;
		palTransferPending = false;
		palResult = PalResult{};
		halted = false;
		memoryBarrierCompleted = false;
		writeBufferDrained = false;
		needsMemoryBarrier = false;
		needsWriteBufferDrain = false;
		serializeType = SerializationType::Barrier_MB;
		payLoad = 0;
		ra_value = 0;
		// slotSequence -- intentionally NOT cleared (monotonic)
		registerIndex = RegisterBankInteger::NONE;
		memResultValid = false;
		flushPipeline = false;
		m_pending = PendingCommit{};
		barrierKind = MemoryBarrierKind::FETCH;
		nextPC = 0;
		predictedPC = 0;
		linkValue = 0;
		jumpTarget = 0;
		branchTestValue = 0;
		mispredict = false;
		pcReason = static_cast<PCReason>(0xFF);
		// Box pointers (m_eBox, m_fBox, etc.) -- intentionally NOT cleared
		// m_faultDispatcher -- intentionally NOT cleared (per-CPU constant)
		// m_iprGlobalMaster -- intentionally NOT cleared (per-CPU constant)
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
     uint64_t cycle;             // cycle number when instruction was fetched
     uint64_t retireCycle;       // cycle number when instruction retired (WB)

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
