// ============================================================================
// global_SRMEnvStore.h
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global singleton access to SRM Environment Store
// Thread-safe singleton pattern with initialization/cleanup
// ============================================================================

#ifndef GLOBAL_SRMENVSTORE_H
#define GLOBAL_SRMENVSTORE_H

#include <QString>

// Forward declarations
class SRMEnvStore;

// ============================================================================
// Global Access Functions
// ============================================================================

/**
 * @brief Get global SRMEnvStore instance
 * @return Reference to singleton SRMEnvStore
 * @note Thread-safe, lazy initialization
 */
SRMEnvStore& global_SRMEnvStore() noexcept;

/**
 * @brief Initialize global SRMEnvStore with config path
 * @param configPath Configuration directory path
 * @note Should be called once during application startup
 */
void initializeGlobalSRMEnvStore(const QString& configPath) noexcept;

/**
 * @brief Shutdown and cleanup global SRMEnvStore
 * @note Should be called during application shutdown
 */
void shutdownGlobalSRMEnvStore() noexcept;

/**
 * @brief Check if global SRMEnvStore is initialized
 * @return true if initialized
 */
bool isGlobalSRMEnvStoreInitialized() noexcept;

// ============================================================================
// Convenience Functions
// ============================================================================

/**
 * @brief Quick access to SRM environment variable
 * @param name Variable name
 * @return Variable value or empty string if not found
 */
QString getSRMEnv(const QString& name) noexcept;

/**
 * @brief Quick set of SRM environment variable
 * @param name Variable name
 * @param value Variable value
 */
void setSRMEnv(const QString& name, const QString& value) noexcept;

/**
 * @brief Check if SRM environment variable exists
 * @param name Variable name
 * @return true if variable exists
 */
bool hasSRMEnv(const QString& name) noexcept;

#endif // GLOBAL_SRMENVSTORE_H
