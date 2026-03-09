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

#include "coreLib/Axp_Attributes_core.h"
#include "grainFactoryLib/iGrain-KeyIdenties.h"
#include "faultLib/fault_core.h"
#include "types_core.h"
#include "machineLib/PipelineSlot.h"
enum class MEM_STATUS;





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
	// IBox Adjacent path support only
	PipelineSlot	   slot;				// carry fault state on fetch failure ( we need this for IBox - Adjacent execution path cannot return to exception dispatch in run loop.
	
    MEM_STATUS                fetchStatus;
};



#endif // FETCHRESULT_H
