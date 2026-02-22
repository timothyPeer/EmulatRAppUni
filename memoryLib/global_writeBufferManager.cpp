// ============================================================================
// global_writeBufferManager.cpp - ============================================================================
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

#include "global_writeBufferManager.h"
#include "WriteBufferManager.h"
#include "../coreLib/LoggingMacros.h"
#include <QtCore/QString>

// ============================================================================
// Internal Singleton Instance
// ============================================================================

namespace {
    // Single static instance shared by all functions
    WriteBufferManager* g_instance = nullptr;
}

// ============================================================================
// Global WriteBufferManager Accessor
// ============================================================================

WriteBufferManager& global_WriteBufferManager() noexcept
{
    if (!g_instance) {
        // Should not happen - must be initialized in EmulatRInit
        Q_ASSERT_X(false, "global_WriteBufferManager",
                   "WriteBufferManager not initialized - call initializeGlobalWriteBufferManager() first");
        
        // For release builds, create a default instance as fallback
        CRITICAL_LOG("WriteBufferManager not initialized! Creating emergency fallback instance.");
        g_instance = new WriteBufferManager(1);  // Single CPU fallback
    }

    return *g_instance;
}

// ============================================================================
// Initialization
// ============================================================================

WriteBufferManager* initializeGlobalWriteBufferManager(quint16 cpuCount) noexcept
{
    if (g_instance) {
        WARN_LOG("WriteBufferManager already initialized - ignoring duplicate initialization");
        return g_instance;
    }

    try {
        g_instance = new WriteBufferManager(cpuCount);

        INFO_LOG(QString("Global WriteBufferManager initialized for %1 CPUs")
                     .arg(cpuCount));

        return g_instance;

    } catch (const std::exception& e) {
        ERROR_LOG(QString("Failed to initialize WriteBufferManager: %1").arg(e.what()));
        return nullptr;
    } catch (...) {
        ERROR_LOG("Failed to initialize WriteBufferManager: Unknown exception");
        return nullptr;
    }
}

// ============================================================================
// Shutdown
// ============================================================================

void shutdownGlobalWriteBufferManager() noexcept
{
    if (g_instance) {
        DEBUG_LOG("Shutting down global WriteBufferManager");
        
        delete g_instance;
        g_instance = nullptr;

        INFO_LOG("Global WriteBufferManager shutdown complete");
    }
}

// ============================================================================
// Initialization Check
// ============================================================================

bool global_WriteBufferManager_IsInitialized() noexcept
{
    return g_instance != nullptr;
}
