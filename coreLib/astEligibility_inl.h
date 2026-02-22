// ============================================================================
// astEligibility_inl.h - AstEligibility.H
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

#ifndef ASTELIGIBILITY_INL_H
#define ASTELIGIBILITY_INL_H
// AstEligibility.H
// ASCII, header-only.
//
// updateAstEligibility() cold-path gating for Alpha-style AST delivery.
//
// Contract (as you stated):
// An AST is deliverable only when ALL are true simultaneously:
//  1) ASTSR has a pending bit set for some mode (K/E/S/U)
//  2) ASTEN has the enable bit set for that same mode
//  3) Current mode (CM) is equal to OR less privileged than the target mode
//     (with numeric ordering K=0, E=1, S=2, U=3 -> "less privileged" == larger number)
//     => condition: (cm >= targetMode)
//  4) IPL <= 2
//
// Notes:
//  - This function does not deliver the AST; it only updates "AST pending"
//    state (e.g., IRQPendingState) based on current HWPCB state.
//  - If multiple modes are eligible simultaneously, we choose the MOST
//    privileged eligible target (K then E then S then U) for determinism.
//
// Source reference (comment-level):
//  - Alpha AXP System Architecture (ASA), AST mechanism and enable/summary
//    gating, and the "interrupt level / IPL gating" behavior described in
//    the processor control / interrupt/exception sections.
//    (Use your platform SRM/PAL docs for exact per-IPL behavior and
//     per-mode selection rules if you want silicon-faithful ordering.)

#pragma once

#include <QtGlobal>

#ifndef AXP_HOT
#define AXP_HOT
#endif

#ifndef AXP_ALWAYS_INLINE
#define AXP_ALWAYS_INLINE inline
#endif

// Mode encoding used by your HWPCB::cm and common Alpha conventions:
//   0 = Kernel, 1 = Executive, 2 = Supervisor, 3 = User
namespace asa_ast
{
enum : quint8
{
    CM_K = 0,
    CM_E = 1,
    CM_S = 2,
    CM_U = 3
};

// AST bit positions in HWPCB::aster / HWPCB::astsr:
// bits [3:0] correspond to K/E/S/U, respectively.
enum : quint8
{
    ASTBIT_K = 0,
    ASTBIT_E = 1,
    ASTBIT_S = 2,
    ASTBIT_U = 3
};

struct AstEligibilityResult
{
    bool  anyEligible{false};
    quint8 targetMode{CM_U}; // valid only if anyEligible==true
};

AXP_HOT AXP_ALWAYS_INLINE static bool testBit4(quint8 v, quint8 bit) noexcept
{
    return ((v >> (bit & 0x3)) & 0x1u) != 0u;
}

// Compute eligibility without mutating external state.
// This can be unit-tested easily.
AXP_HOT AXP_ALWAYS_INLINE static AstEligibilityResult computeAstEligibility(
    quint8 asten,   // HWPCB::aster (enable mask, 4-bit)
    quint8 astsr,   // HWPCB::astsr (pending summary, 4-bit)
    quint8 cm,      // HWPCB::cm (0..3)
    quint8 ipl      // HWPCB::ipl (0..31)
    ) noexcept
{
    AstEligibilityResult r;

    // Condition (4): IPL gating
    if (ipl > 2) {
        return r; // not eligible at any mode
    }

    // Build "candidate" bits: pending AND enabled
    const quint8 pendingEnabled = quint8((astsr & 0x0Fu) & (asten & 0x0Fu));
    if ((pendingEnabled & 0x0Fu) == 0u) {
        return r; // nothing pending+enabled
    }

    // Condition (3): CM must be equal or less privileged than targetMode.
    // Using numeric ordering K=0..U=3, "less privileged" means larger number.
    // Thus eligible if: cm >= targetMode.
    //
    // Select the MOST privileged eligible target for determinism:
    // K first, then E, then S, then U.
    //
    // K target eligible if cm>=0 always (true) but requires pendingEnabled[K].
    if (testBit4(pendingEnabled, ASTBIT_K) && (cm >= CM_K)) {
        r.anyEligible = true;
        r.targetMode = CM_K;
        return r;
    }
    if (testBit4(pendingEnabled, ASTBIT_E) && (cm >= CM_E)) {
        r.anyEligible = true;
        r.targetMode = CM_E;
        return r;
    }
    if (testBit4(pendingEnabled, ASTBIT_S) && (cm >= CM_S)) {
        r.anyEligible = true;
        r.targetMode = CM_S;
        return r;
    }
    if (testBit4(pendingEnabled, ASTBIT_U) && (cm >= CM_U)) {
        r.anyEligible = true;
        r.targetMode = CM_U;
        return r;
    }

    // Pending+enabled exists, but current CM is too privileged for the target(s).
    // Example: cm=Kernel(0) and only User AST is pending+enabled -> not eligible yet.
    return r;
}
} // namespace asa_ast

#endif // ASTELIGIBILITY_INL_H
