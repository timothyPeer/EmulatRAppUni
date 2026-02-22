// ============================================================================
// global_faultDispatcher.h - Get current CPU's fault dispatcher (TLS proxy)
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

#ifndef GLOBAL_FAULTDISPATCHER_H
#define GLOBAL_FAULTDISPATCHER_H

// Forward declaration
class FaultDispatcher;

/**
 * @brief Get current CPU's fault dispatcher (TLS proxy)
 *
 * Uses CurrentCpuTLS::get() to determine CPU automatically.
 * Perfect for embedded code that can't easily pass CPU ID.
 *
 * @return FaultDispatcher for current CPU
 */
FaultDispatcher& global_faultDispatcher() noexcept;

#endif // GLOBAL_FAULTDISPATCHER_H
