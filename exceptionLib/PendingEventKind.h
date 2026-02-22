// ============================================================================
// PendingEventKind.h - Pending Event Kind
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

#ifndef PENDINGEVENTKIND_H
#define PENDINGEVENTKIND_H
#include <QtGlobal>

enum class PendingEventKind : quint8
{
    None = 0,
    Exception,      // Synchronous fault/trap
    Interrupt,      // Asynchronous interrupt
    Ast,            // Asynchronous System Trap
    MachineCheck,   // Machine check
    Reset,           // Reset/wakeup
    PalCall			// CALL_PAL Eventkind
};
#endif // PENDINGEVENTKIND_H
