// ============================================================================
// cpuCore_core.h - enum class Redirect_Reason : int {
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

#ifndef CPUCORE_CORE_H2
#define CPUCORE_CORE_H2

#include <QtGlobal>

#include "AlphaPipeline.h"
#include "RedirectReason.h"
// enum class Redirect_Reason : int {
//     None,
//     PALEntry,           // CALL_PAL instruction
//     Trap,               // Synchronous exception
//     Interrupt,          // Asynchronous interrupt
//     MachineCheck,       // Machine check
//     Reset               // CPU reset
// };
enum class StepResultType : quint16
{
    Committed,      // Normal instruction completion
    Redirect,       // Control transfer (PAL/trap/interrupt)
    Halted,         // HALT instruction
    Parked,         // CPU parked (WFI-style wait)
    Fault           // Fault occurred
};
struct StepResult {
    StepResultType stepType;
    quint64 nextPC;

    // For Redirect
    RedirectReason redirectReason;
    quint64 metadata1;  // PAL func / trap class / interrupt IPL
    quint64 metadata2;  // call PC / fault PC / interrupt vector
    quint64 metadata3;  // fault VA / R16 / etc.

    // For Halted
    quint32 haltCode;
};
// ============================================================================
// PAL Entry Info - Returned by PalBox
// ============================================================================
struct PALEntryInfo {
    quint64 vectorPC{ 0 };
    bool shadowRegsActive{ false };
    quint64 exc_addr_value{ 0 };
    // Add other side effects as needed
};



#endif // CPUCORE_CORE_H
