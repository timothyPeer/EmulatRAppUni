// ============================================================================
// global_executioncoordinator.cpp - UPDATED
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

#include "global_ExecutionCoordinator.h"
#include "ExecutionCoordinator.h"
#include "../configLib/global_EmulatorSettings.h"
#include "../coreLib/LoggingMacros.h"


// ============================================================================
// Global Accessor - Meyers Singleton with EmulatorSettings Integration
// ============================================================================

ExecutionCoordinator& global_ExecutionCoordinator() noexcept
{
    // Read CPU count from EmulatorSettings (SSOT)
    auto& settings = global_EmulatorSettings();
    int cpuCount = settings.podData.system.processorCount;

    // Validate CPU count
    if (cpuCount < 1 || cpuCount > MAX_CPUS) {
        WARN_LOG(QString("Invalid CPU count %1 from settings, using default 4")
            .arg(cpuCount));
        cpuCount = 4;
    }

    // Create singleton with settings-derived CPU count
    static ExecutionCoordinator instance(cpuCount);

    return instance;
}






