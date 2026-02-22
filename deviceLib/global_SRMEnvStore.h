// ============================================================================
// global_SRMEnvStore.h - Get global SRMEnvStore instance
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
