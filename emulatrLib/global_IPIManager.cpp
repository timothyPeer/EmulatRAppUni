// ============================================================================
// global_IPIManager.cpp - ============================================================================
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

// ============================================================================
// global_IPIManager.cpp
// ============================================================================

#include "global_IPIManager.h"

#include "SubsystemCoordinator.h"
#include "global_SubsystemCoordinator.h"
#include "../emulatrLib/IPIManager.h"

IPIManager& global_IPIManager() noexcept
{
    return *global_SubsystemCoordinator().ipiManager();
}