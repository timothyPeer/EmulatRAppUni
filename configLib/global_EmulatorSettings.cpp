// ============================================================================
// global_EmulatorSettings.cpp - Global Singleton Implementation
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

// ============================================================================
//  global_EmulatorSettings.cpp - Global Singleton Implementation
// ============================================================================

#include "global_EmulatorSettings.h"
#include <QMutex>
#include <QMutexLocker>
#include <QFileInfo>

// ============================================================================
// SINGLETON STATE (INTERNAL)
// ============================================================================

namespace {
    // Thread-safe singleton instance
    QMutex g_settingsMutex;
    EmulatorSettingsInline* g_settingsInstance = nullptr;
    QString g_loadedIniPath;
    bool g_isInitialized = false;
}

// ============================================================================
// GLOBAL SINGLETON ACCESSOR
// ============================================================================

EmulatorSettingsInline& global_EmulatorSettings()
{
    QMutexLocker locker(&g_settingsMutex);
    
    // Lazy initialization with defaults
    if (!g_settingsInstance) {
        g_settingsInstance = new EmulatorSettingsInline();
        
        // If no explicit initialization has occurred, log a warning
        if (!g_isInitialized) {
            WARN_LOG_Fallback("EmulatorSettings accessed before initialization - using defaults");
        }
    }
    
    return *g_settingsInstance;
}

// ============================================================================
// INITIALIZATION FUNCTION
// ============================================================================

bool initializeGlobalEmulatorSettings(const QString& iniFilePath)
{
    QMutexLocker locker(&g_settingsMutex);
    
    // Check if already initialized
    if (g_isInitialized) {
        WARN_LOG_Fallback(QString(
            "Global EmulatorSettings already initialized from: %1\n"
            "Ignoring attempt to re-initialize from: %2\n"
            "Restart application to change configuration files."
        ).arg(g_loadedIniPath).arg(iniFilePath));
        return false;
    }
    
    // Create instance if it doesn't exist yet
    if (!g_settingsInstance) {
        g_settingsInstance = new EmulatorSettingsInline();
    }
    
    // Validate file exists
    if (!QFileInfo::exists(iniFilePath)) {
        CRITICAL_LOG_Fallback(QString(
            "Configuration file not found: %1\n"
            "Using default configuration values."
        ).arg(iniFilePath));
        
        // Mark as initialized even though we're using defaults
        // This prevents repeated initialization attempts
        g_isInitialized = true;
        g_loadedIniPath = "(defaults - file not found)";
        return false;
    }
    
    // Load the configuration
    INFO_LOG_Fallback(QString("Initializing global EmulatorSettings from: %1").arg(iniFilePath));
    
    bool success = g_settingsInstance->loadFromIni(iniFilePath);
    
    if (success) {
        g_isInitialized = true;
        g_loadedIniPath = iniFilePath;
        
        INFO_LOG_Fallback(QString(
            "Global EmulatorSettings initialized successfully\n"
            "  Controllers: %1\n"
            "  Devices: %2\n"
            "  Consoles: %3\n"
            "  Caches: %4"
        )
        .arg(g_settingsInstance->podData.controllers.size())
        .arg(g_settingsInstance->podData.devices.size())
        .arg(g_settingsInstance->podData.opaConsoles.size())
        .arg(g_settingsInstance->podData.caches.size()));
    }
    else {
        CRITICAL_LOG_Fallback(QString("Failed to load configuration from: %1").arg(iniFilePath));
    }
    
    return success;
}

// ============================================================================
// QUERY FUNCTIONS
// ============================================================================

bool isGlobalEmulatorSettingsInitialized()
{
    QMutexLocker locker(&g_settingsMutex);
    return g_isInitialized;
}

QString getGlobalEmulatorSettingsPath()
{
    QMutexLocker locker(&g_settingsMutex);
    return g_loadedIniPath;
}

// ============================================================================
// SHUTDOWN FUNCTION
// ============================================================================

void shutdownGlobalEmulatorSettings()
{
    QMutexLocker locker(&g_settingsMutex);
    
    if (g_settingsInstance) {
        INFO_LOG_Fallback("Shutting down global EmulatorSettings");
        delete g_settingsInstance;
        g_settingsInstance = nullptr;
    }
    
    g_isInitialized = false;
    g_loadedIniPath.clear();
}

// bool initializeGlobalAlphaMemorySystem()
// {
// 	static QMutex g_memorySystemMutex;
// 	static AlphaMemorySystem* g_memorySystemInstance = nullptr;
// 	static bool g_isInitialized = false;
// 
// 	QMutexLocker locker(&g_memorySystemMutex);
// 
// 	if (g_isInitialized) {
// 		WARN_LOG("Global AlphaMemorySystem already initialized");
// 		return false;
// 	}
// 
// 	INFO_LOG("Initializing global AlphaMemorySystem...");
// 
// 	g_memorySystemInstance = new AlphaMemorySystem();
// 	g_isInitialized = true;
// 
// 	INFO_LOG("Global AlphaMemorySystem initialized successfully");
// 
// 	return true;
// }