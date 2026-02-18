// ============================================================================
// global_ConsoleManager.h
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global singleton accessor for ConsoleManager
// Provides system-wide console device registry
// ============================================================================

#ifndef GLOBAL_CONSOLEMANAGER_H
#define GLOBAL_CONSOLEMANAGER_H

// Forward declaration
class ConsoleManager;

/**
 * @brief Get global ConsoleManager singleton instance.
 * 
 * Provides access to the system-wide console device registry.
 * Thread-safe Meyer's singleton pattern.
 * 
 * @return Reference to global ConsoleManager instance
 */
ConsoleManager& global_ConsoleManager() noexcept;

#endif // GLOBAL_CONSOLEMANAGER_H
