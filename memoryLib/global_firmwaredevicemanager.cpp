// ============================================================================
// global_FirmwareDeviceManager.cpp
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global FirmwareDeviceManager singleton implementation
// ============================================================================

#include "global_FirmwareDeviceManager.h"
#include "FirmwareDeviceManager.h"
#include "../configLib/global_EmulatorSettings.h"
#include "../coreLib/LoggingMacros.h"
#include <atomic>

namespace {
    // Thread-safe initialization flag
    std::atomic<bool> g_initialized{false};
}

// ============================================================================
// Meyer's Singleton Accessor
// ============================================================================

FirmwareDeviceManager& global_FirmwareDeviceManager() noexcept
{
    static FirmwareDeviceManager instance;
    return instance;
}

// ============================================================================
// Initialization Status
// ============================================================================

bool global_FirmwareDeviceManager_IsInitialized() noexcept
{
    return g_initialized.load(std::memory_order_acquire);
}

// ============================================================================
// Device Tree Initialization (5 Phases)
// ============================================================================

bool initializeGlobalFirmwareDeviceManager() noexcept
{
    INFO_LOG("Initializing Global FirmwareDeviceManager...");

    // Get singleton instance
    auto& fdm = global_FirmwareDeviceManager();

    // Get configuration
    auto& config = global_EmulatorSettings();

    // ------------------------------------------------------------------------
    // Phase 0: Firmware Context Initialization
    // ------------------------------------------------------------------------
    
    if (!fdm.initializePhase0_FirmwareContext(config.podData)) {
        ERROR_LOG("Device Tree Phase 0 failed: Firmware Context");
        return false;
    }
    INFO_LOG("Device Tree Phase 0: Firmware Context - OK");

    // ------------------------------------------------------------------------
    // Phase 1: Platform Root Creation
    // ------------------------------------------------------------------------
    
    if (!fdm.initializePhase1_PlatformRoot()) {
        ERROR_LOG("Device Tree Phase 1 failed: Platform Root");
        return false;
    }
    INFO_LOG("Device Tree Phase 1: Platform Root - OK");

    // ------------------------------------------------------------------------
    // Phase 2: Bus Discovery and Attachment
    // ------------------------------------------------------------------------
    
    if (!fdm.initializePhase2_BusDiscovery()) {
        ERROR_LOG("Device Tree Phase 2 failed: Bus Discovery");
        return false;
    }
    INFO_LOG("Device Tree Phase 2: Bus Discovery - OK");

    // ------------------------------------------------------------------------
    // Phase 3: Device Enumeration and Registration
    // ------------------------------------------------------------------------
    
    if (!fdm.initializePhase3_DeviceEnumeration()) {
        ERROR_LOG("Device Tree Phase 3 failed: Device Enumeration");
        return false;
    }
    INFO_LOG("Device Tree Phase 3: Device Enumeration - OK");

    // ------------------------------------------------------------------------
    // Phase 4: Device Finalization and Console Exposure
    // ------------------------------------------------------------------------
    
    if (!fdm.initializePhase4_Finalization()) {
        ERROR_LOG("Device Tree Phase 4 failed: Finalization");
        return false;
    }
    INFO_LOG("Device Tree Phase 4: Finalization - OK");

    // ------------------------------------------------------------------------
    // Mark as initialized
    // ------------------------------------------------------------------------
    
    g_initialized.store(true, std::memory_order_release);

    INFO_LOG("Global FirmwareDeviceManager initialized successfully");
    INFO_LOG(QString("Device Tree contains %1 devices")
        .arg(fdm.getAllDeviceNames().size()));

    return true;
}

// ============================================================================
// Shutdown
// ============================================================================

void shutdownGlobalFirmwareDeviceManager() noexcept
{
    INFO_LOG("Shutting down Global FirmwareDeviceManager...");
    
    g_initialized.store(false, std::memory_order_release);
    
    // Note: The FirmwareDeviceManager instance persists as a Meyer's singleton
    // but is marked as uninitialized. Call initializeGlobalFirmwareDeviceManager()
    // again to re-initialize if needed.
    
    INFO_LOG("Global FirmwareDeviceManager shutdown complete");
}
