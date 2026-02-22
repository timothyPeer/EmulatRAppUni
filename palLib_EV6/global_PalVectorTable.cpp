// ============================================================================
// global_PalVectorTable.cpp - ============================================================================
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

#include "global_PalVectorTable.h"
#include "PalVectorTable_final.h"

// ============================================================================
// Global Accessor Implementation
// ============================================================================

PalVectorTable& global_PalVectorTable() noexcept
{
    // Delegate to PalVectorTable's singleton instance() method
    return PalVectorTable::instance();
}
