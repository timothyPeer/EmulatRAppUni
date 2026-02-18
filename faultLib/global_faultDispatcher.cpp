// ============================================================================
// global_faultDispatcher.cpp
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// TLS-based fault dispatcher accessor implementation
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
