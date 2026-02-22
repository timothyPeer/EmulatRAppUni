// ============================================================================
// global_ExecutionCoordinator.h - Global Accessor Declaration
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

#ifndef GLOBAL_EXECUTIONCOORDINATOR_H
#define GLOBAL_EXECUTIONCOORDINATOR_H

// Forward declaration ONLY - NO INCLUDES!
class ExecutionCoordinator;

/**
 * @brief Get global ExecutionCoordinator instance (singleton)
 * @return Reference to global ExecutionCoordinator
 */
ExecutionCoordinator& global_ExecutionCoordinator() noexcept;

#endif // GLOBAL_EXECUTIONCOORDINATOR_H