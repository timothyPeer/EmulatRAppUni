// ============================================================================
// AMASK_Constants_inl.h - ============================================================================
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

#ifndef AMASK_CONSTANTS_INL_H
#define AMASK_CONSTANTS_INL_H
#include <QtGlobal>
// ============================================================================
// AMASK feature bits (Architectural)
// ============================================================================
static constexpr quint64 AMASK_BWX = (1ULL << 0);  // Byte/Word extensions
static constexpr quint64 AMASK_FIX = (1ULL << 1);  // Integer extensions
static constexpr quint64 AMASK_CIX = (1ULL << 2);  // Count extensions
static constexpr quint64 AMASK_MVI = (1ULL << 3);  // Multimedia extensions
static constexpr quint64 AMASK_PAT = (1ULL << 4);  // Prefetch assist
static constexpr quint64 AMASK_PM  = (1ULL << 5);  // Performance monitoring


static constexpr quint64 AMASK_EMULATOR_SUPPORTED =
    AMASK_BWX |
    AMASK_FIX |
    AMASK_CIX |
    AMASK_PAT;   // add/remove deliberately

#endif // AMASK_CONSTANTS_INL_H
