// ============================================================================
// global_faultDispatcher.cpp - ============================================================================
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

#include "global_faultDispatcher.h"
#include "GlobalFaultDispatcherBank.h"
#include "../coreLib/CurrentCpuTls.h"
#include "../coreLib/LoggingMacros.h"
#include "FaultDispatcher.h"

FaultDispatcher& global_faultDispatcher() noexcept
{
    CPUIdType currentCpu = CurrentCpuTLS::get();

    if (Q_UNLIKELY(!CurrentCpuTLS::isSet())) {
        ERROR_LOG("TLS not set!");
        currentCpu = 0;  // Safe fallback
    }

    return GlobalFaultDispatcherBank::getDispatcher(currentCpu);
}
