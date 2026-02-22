// ============================================================================
// globalCPUCount.h - ===============================================================
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

#ifndef GLOBALCPUCOUNT_H
#define GLOBALCPUCOUNT_H


// ===============================================================
// Global CPU Count Provider (Meyers Singleton)
// ===============================================================
#include "types_core.h"
class GlobalCPUCount final
{
public:
    static inline void initialize(int count = MAX_CPUS) noexcept {
        cpuCount_ = count;
    }

    static inline int get() noexcept {
        return cpuCount_;
    }

private:
    inline static int cpuCount_ = 1; // safe default, updated at startup
};

#endif // GLOBALCPUCOUNT_H
