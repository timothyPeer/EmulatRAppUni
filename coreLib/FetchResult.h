// ============================================================================
// FetchResult.h - ============================================================================
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

#ifndef FETCHRESULT_H
#define FETCHRESULT_H
#include <QtGlobal>
#include "../grainFactoryLib/DecodedInstruction.h"
#include "../grainFactoryLib/iGrain-KeyIdenties.h"
#include "../coreLib/Axp_Attributes_core.h"

#include "faultLib/fault_core.h"
#include "types_core.h"

enum class MEM_STATUS;



// ============================================================================
// Pipeline Step Result
// ============================================================================

enum class PipelineAction : quint8 {
	ADVANCED = 0,       // Ring advanced normally
	STALLED,            // Ring frozen (box stalled)
	FAULT,              // Fault detected - enter PAL
	MISPREDICTION,      // Branch misprediction - flush
	PAL_CALL,           // CALL_PAL - enter PAL mode
	HALT                // HW_HALT - stop CPU
};

// ============================================================================
// Pipeline Step Result
// ============================================================================
struct PipelineStepResult {
	PipelineAction action = PipelineAction::ADVANCED;

	// For FAULT:
	TrapCode_Class trapCode;
	quint64 faultVA;
	quint64 faultPC;

	// For PAL_CALL:
	quint32 palFunction;     // PAL function code (0-255)
	quint64 callPC;         // PC of CALL_PAL instruction
	quint64 palVector;      // Computed PAL entry vector

	// For MISPREDICTION:
	quint64 redirectPC;     // Correct target address

	// Helper constructors
	static PipelineStepResult Advanced() {
		return PipelineStepResult{ .action = PipelineAction::ADVANCED, .trapCode = TrapCode_Class::NONE, .faultVA = 0,
			.faultPC = 0, .palFunction = 0, .callPC = 0, .palVector = 0, .redirectPC = 0 };
	}

	static PipelineStepResult Stalled() {
		return PipelineStepResult{ .action = PipelineAction::STALLED };
	}
	AXP_HOT AXP_ALWAYS_INLINE bool isCallPalReturn() const noexcept
	{
		return callPC > 0;
	}

	static PipelineStepResult Fault(TrapCode_Class tc, quint64 va, quint64 pc) {
		PipelineStepResult r;
		r.action = PipelineAction::FAULT;
		r.trapCode = tc;
		r.faultVA = va;
		r.faultPC = pc;
		return r;
	}

	static PipelineStepResult PalCall(quint8 func, quint64 pc) {
		PipelineStepResult r;
		r.action = PipelineAction::PAL_CALL;
		r.palFunction = func;
		r.callPC = pc;
		return r;
	}

	static PipelineStepResult Mispredict(quint64 target) {
		PipelineStepResult r;
		r.action = PipelineAction::MISPREDICTION;
		r.redirectPC = target;
		return r;
	}

};



struct FetchResult {
    DecodedInstruction di{};
    bool               predictedTaken{ false };
    quint64            predictedTarget{ 0 };
    quint64            virtualAddress{ 0 };
	quint64				physicalAddress{0};  //  Clear: Physical address (after translation)
	bool			   predictedValid{ false };
    bool               valid{ false };    // we will test this as successful completion of the fetch sequence
    bool               isCallPal{ false };         // We set this in the iBox->decode() function.
    quint16            palFunction{}; // Index extracted from the instruction &0x7F this will be used later to calculate the PALVectorID for this PAL Instruction.
    CPUIdType          m_cpuId{ 0 }; // we will pass the CPU in to ensure cpu context
    PcKey              pcKey{};
    PaKey              paKey{};
    PipelineStepResult pipelineStepResult{};

	
    MEM_STATUS                fetchStatus;
};



#endif // FETCHRESULT_H
