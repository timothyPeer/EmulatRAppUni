// ============================================================================
// DStreamErrorContext_EV6.h - ----------------------------------------------------------------------------
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

#ifndef DSTREAMERRORCONTEXT_EV6_H
#define DSTREAMERRORCONTEXT_EV6_H
#include <QtGlobal>

// ----------------------------------------------------------------------------
// DStreamErrorContext_EV6
//
// Captures the classification bits used by EV6 for D-stream exceptions.
// Implements the decision table in EV6 HRM Table 4-2.
// ----------------------------------------------------------------------------
//  Fields (conceptual mapping to Table 4-2:
//
//    badVa    : BAD_VA column
//    dtbMiss  : DTB_MISS column
//    unalign  : UNALIGN column
//    inPal    : PAL column (1 when fault while executing PAL code)
//    other    : Other column (used to flag remaining error types)
//
//  Reference:
//    21264 HRM, Section 4.x, Table 4-2 "D-stream Error PALcode Entry Points"
// ----------------------------------------------------------------------------
struct DStreamErrorContext_EV6
{
    bool badVa;    // 1 means virtual address is invalid
    bool dtbMiss;  // 1 means DTB miss
    bool unalign;  // 1 means unaligned access
    bool inPal;    // 1 when exception occurred in PALmode
    bool other;    // remaining error classification (access violation, etc.)

    // Default constructor: initialize to no error.
    DStreamErrorContext_EV6() noexcept
        : badVa(false)
        , dtbMiss(false)
        , unalign(false)
        , inPal(false)
        , other(false)
    {
    }
};


#endif // DSTREAMERRORCONTEXT_EV6_H
