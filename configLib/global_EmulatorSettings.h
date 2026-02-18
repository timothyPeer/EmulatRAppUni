#ifndef global_EmulatorSettings_h__
#define global_EmulatorSettings_h__

// ============================================================================
//  global_EmulatorSettings.h - Global Singleton for Emulator Configuration
//
//  Purpose: Provides thread-safe, globally accessible emulator settings
//
//  Usage:
//    auto& config = global_EmulatorSettings();
//    int cpuCount = config.podData.system.processorCount;
//    QString logFile = config.podData.logging.logFileName;
//
//  The singleton is initialized lazily on first access or explicitly
//  via initializeGlobalEmulatorSettings(iniFilePath)
// ============================================================================

#include "EmulatorSettingsInline.h"
#include <QString>

// ============================================================================
// GLOBAL SINGLETON ACCESSOR
// ============================================================================

/**
 * @brief Get the global EmulatorSettings singleton
 * @return Reference to the singleton instance
 * 
 * Thread-safe. First access initializes the singleton with defaults.
 * Subsequent calls return the same instance.
 */
EmulatorSettingsInline& global_EmulatorSettings();

/**
 * @brief Initialize the global EmulatorSettings from an INI file
 * @param iniFilePath Path to ASAEmulatR.ini configuration file
 * @return true if successfully loaded, false otherwise
 * 
 * This function should be called early in application initialization.
 * If not called, the singleton will use default values.
 * Can only be called once - subsequent calls will be ignored with a warning.
 */
bool initializeGlobalEmulatorSettings(const QString& iniFilePath);

/**
 * @brief Check if the global settings have been initialized from a file
 * @return true if initialized from file, false if using defaults
 */
bool isGlobalEmulatorSettingsInitialized();

/*bool initializeGlobalAlphaMemorySystem();*/

/**
 * @brief Get the path of the INI file that was loaded (if any)
 * @return Path to loaded INI file, or empty string if not yet initialized
 */
QString getGlobalEmulatorSettingsPath();

/**
 * @brief Shutdown/cleanup the global settings singleton
 * 
 * This is primarily for testing or clean shutdown scenarios.
 * Not typically needed in production code.
 */
void shutdownGlobalEmulatorSettings();

#endif // global_EmulatorSettings_h__
