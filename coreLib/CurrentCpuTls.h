#ifndef CURRENTCPUTLS_H
#define CURRENTCPUTLS_H

// ============================================================================
// CurrentCpuTls.h - Thread-Local Storage for Current CPU ID
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Purpose:
//   Provides thread-local storage for tracking current CPU ID
//   Used by TLS proxy functions for compatibility with embedded code
//
// Usage:
//   CurrentCpuTLS::set(cpuId);  // Set current CPU for this thread
//   auto id = CurrentCpuTLS::get();  // Get current CPU ID
//   bool valid = CurrentCpuTLS::isSet();  // Check if set
// ============================================================================

#include <QtGlobal>
#include <limits>

namespace CurrentCpuTLS {
    // Thread-local storage for CPU ID
    inline thread_local quint16 g_id = std::numeric_limits<quint16>::max();

    /**
     * @brief Set current CPU ID for this thread
     * @param id CPU identifier to set
     */
    inline void set(quint16 id) noexcept {
        g_id = id;
    }

    /**
     * @brief Get current CPU ID for this thread
     * @return CPU identifier, or max value if not set
     */
    inline quint16 get() noexcept {
        return g_id;
    }

    /**
     * @brief Check if CPU ID has been set for this thread
     * @return true if CPU ID is valid
     */
    inline bool isSet() noexcept {
        return g_id != std::numeric_limits<quint16>::max();
    }

    /**
     * @brief Clear CPU ID for this thread
     */
    inline void clear() noexcept {
        g_id = std::numeric_limits<quint16>::max();
    }
}

#endif // CURRENTCPUTLS_H
