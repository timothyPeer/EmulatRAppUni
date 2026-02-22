// ============================================================================
// ExceptionClass_EV6.h - ----------------------------------------------------------------------------
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

#ifndef EXCEPTIONCLASS_EV6_H
#define EXCEPTIONCLASS_EV6_H
#include "../coreLib/Axp_Attributes_core.h"
#include <QtGlobal>

// ----------------------------------------------------------------------------
// ExceptionClass_EV6
//
// High level classification of exceptions that the CPU raises to PALcode.
// This enumeration is intended to help map CPU pipeline events to the
// PAL vector resolver.
//
// Reference:
//   21264 HRM, Section 4.x, Table 4-1 "PALcode Entry Points"
//   (RESET, MCHK, ARITH, INTERRUPT, ITB_MISS, ITB_ACV, OPCDEC, FEN, etc.)
// ----------------------------------------------------------------------------
enum class ExceptionClass_EV6 : quint8
{
    None,
    Reset,        // RESET (anytime)
    MachineCheck, // MCHK (anytime)
    InternalProcessorError,
    BugCheck,     // 
    Arithmetic,   // ARITH (anytime)
    Interrupt,    // INTERRUPT (anytime)
    DStream,      // D-stream errors (pipe_stage 6, see Table 4-2)
    ItbMiss,      // ITB_MISS (pipe_stage 5)
    ItbAcv,       // ITB_ACV (pipe_stage 5)
    OpcDec,       // OPCDEC (pipe_stage 5)
    OpcDecFault, 
    Fen,          // FEN (pipe_stage 5)
    Unalign,
    Dfault,
    DtbAcv,       // DTB Access Violation
    Dtb_miss_double_4,
    Dtb_miss_single,
    Dtb_miss_native,
    MT_FPCR,
    IllegalInstruction,
    MemoryFault,
    SoftwareTrap,
    BreakPoint,
    Panic,
    General,
    SubsettedInstruction,
    SystemService,
    PerformanceMonitor,
    PrivilegeViolation,     // PAL Mode violation
    ReservedOperand,
    CallPal       // CALL_PAL (pipe_stage 5)
};

// ExceptionClass to String
//
AXP_HOT AXP_ALWAYS_INLINE const char* exceptionClassName(ExceptionClass_EV6 ec) noexcept
{
    switch (ec)
    {
    case ExceptionClass_EV6::None:              return "NONE";

        // Memory Management
    case ExceptionClass_EV6::ItbMiss:          return "ITB_MISS";
    case ExceptionClass_EV6::ItbAcv:           return "ITB_ACV";
    case ExceptionClass_EV6::Dtb_miss_single:   return "DTB_MISS_SINGLE";
    case ExceptionClass_EV6::Dtb_miss_double_4: return "DTB_MISS_DOUBLE_3";
    case ExceptionClass_EV6::Dtb_miss_native:   return "DTB_MISS_NATIVE";
    case ExceptionClass_EV6::Dfault:            return "DFAULT";

        // Alignment & Opcodes
    case ExceptionClass_EV6::Unalign:           return "UNALIGN";
    case ExceptionClass_EV6::OpcDec:            return "OPCDEC";
    case ExceptionClass_EV6::Fen:               return "FEN";

        // Arithmetic
    case ExceptionClass_EV6::Arithmetic:             return "ARITH";
    case ExceptionClass_EV6::MT_FPCR:           return "MT_FPCR";

        // Interrupts & System
    case ExceptionClass_EV6::Interrupt:         return "INTERRUPT";
    case ExceptionClass_EV6::MachineCheck:              return "MCHK";
    case ExceptionClass_EV6::Reset:             return "RESET";

        // CALL_PAL
    case ExceptionClass_EV6::CallPal:          return "CALL_PAL";

    default:                                return "UNKNOWN";
    }
}
#endif // EXCEPTIONCLASS_EV6_H
