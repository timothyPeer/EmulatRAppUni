// ============================================================================
// EventLog.h - Static Global Event Logging System
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

#ifndef EVENTLOG_H
#define EVENTLOG_H
// ============================================================================
// EventLog.h - Static Global Event Logging System
// ============================================================================
#include <QtGlobal>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QTimer>

enum LogSeverity : int {
    LOG_TRACE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_CRITICAL
};

// ============================================================================
// EventLog - Static Global Event Logging (matches ExecTrace pattern)
// ============================================================================

class EventLog
{
public:
    // ========================================================================
    // Initialization (called once at startup from EmulatR_init)
    // ========================================================================

    /**
     * @brief Initialize event logging system
     * Reads configuration from global_EmulatorSettings()
     * Creates log file, sets up rotation if enabled
     * @return true on success, false on failure
     */
    static bool initialize() noexcept;

    /**
     * @brief Shutdown event logging system
     * Flushes pending logs, writes shutdown marker, closes files
     */
    static void shutdown() noexcept;


    // ========================================================================
    // Logging Interface (called by macros)
    // ========================================================================

    /**
     * @brief Write log message (primary interface)
     * @param severity Log severity level
     * @param msg Message string (QString for compatibility)
     */
    static void write(LogSeverity severity, const QString& msg) noexcept;

    /**
     * @brief Write log message with explicit CPU context
     * @param cpuId CPU that generated this log
     * @param severity Log severity level
     * @param msg Message string
     */
    static void writeCpu(uint16_t cpuId, LogSeverity severity, const QString& msg) noexcept;

    // ========================================================================
    // Configuration Query
    // ========================================================================

    /**
     * @brief Check if logging is enabled
     * @return true if any output backend is enabled
     */
    static bool isEnabled() noexcept;

    /**
     * @brief Check if specific severity level is enabled
     * @param severity Severity level to check
     * @return true if severity >= configured minimum level
     */
    static bool isEnabledForSeverity(LogSeverity severity) noexcept;

    // ========================================================================
    // Manual Flush Control
    // ========================================================================

    /**
     * @brief Force flush of pending log data to disk
     * Normally logs flush automatically; use for critical operations
     */
    static void flush() noexcept;

	// Configurable flush behavior
    static inline int writeCount{ 0 };


private:
    // Internal implementation (hidden from users)
    class Impl;



};
#endif // EVENTLOG_H
