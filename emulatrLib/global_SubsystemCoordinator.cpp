// ============================================================================
// global_SubsystemCoordinator.cpp - global  Subsystem Coordinator
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

#include "global_SubSystemCoordinator.h"
#include "SubsystemCoordinator.h"

AXP_HOT AXP_FLATTEN SubsystemCoordinator& global_SubsystemCoordinator() noexcept {
    static SubsystemCoordinator instance;
    return instance;
}


