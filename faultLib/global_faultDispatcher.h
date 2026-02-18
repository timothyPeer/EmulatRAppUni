// ============================================================================
// global_faultDispatcher.h
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// TLS-based fault dispatcher accessor
// Uses CurrentCpuTLS to automatically determine which CPU's dispatcher to use
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
