// ============================================================================
// global_EmulatR_init.cpp - global  Emulat R init
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

#include "global_EmulatR_init.h"
#include "EmulatR_init.h"
#include "global_SubsystemCoordinator.h"

EmulatR_init& global_EmulatR_init() noexcept
{
    static EmulatR_init instance(&global_SubsystemCoordinator());
    return instance;
}

