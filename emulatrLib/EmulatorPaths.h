// ============================================================================
// EmulatorPaths.h - COMPLETE - Consistent Path Management
// ============================================================================

#ifndef EMULATORPATHS_H
#define EMULATORPATHS_H

#include <QString>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>

class EmulatorPaths {
public:
    EmulatorPaths() {
        // Get exe location (e.g., Z:/EmulatRAppUni/out/build/x64-debug/EmulatRAppUni)
        QString exePath = QCoreApplication::applicationDirPath();

        // Set bin directory (where exe lives)
        m_binDir = exePath;

        // Calculate root (parent of bin directory)
        // For: Z:/EmulatRAppUni/out/build/x64-debug/EmulatRAppUni
        // Root: Z:/EmulatRAppUni/out/build/x64-debug
        QDir binQDir(exePath);
        if (binQDir.cdUp()) {
            m_rootDir = binQDir.absolutePath();
        }
        else {
            m_rootDir = exePath;  // Fallback if can't go up
        }

        // Calculate standard directories relative to exe
        m_configDir = QDir(exePath).filePath("config");
        m_logsDir = QDir(exePath).filePath("logs");
        m_firmwareDir = QDir(exePath).filePath("firmware");

        // Create directories if they don't exist
        QDir().mkpath(m_configDir);
        QDir().mkpath(m_logsDir);
        QDir().mkpath(m_firmwareDir);
    }

    // ========================================================================
    // Directory Getters
    // ========================================================================

    QString getBinDir() const { return m_binDir; }
    QString getConfigDir() const { return m_configDir; }
    QString getLogsDir() const { return m_logsDir; }
    QString getFirmwareDir() const { return m_firmwareDir; }
    QString getRootDir() const { return m_rootDir; }

    // ========================================================================
    // Legacy/Alias Methods (for backwards compatibility)
    // ========================================================================

    QString getBinPath() const { return m_binDir; }         // Alias for getBinDir()
    QString getRootPath() const { return m_rootDir; }       // Alias for getRootDir()
    QString getConfigPath() const { return m_configDir; }   // No-arg version
    QString getLogPath() const { return m_logsDir; }        // No-arg version

    // ========================================================================
    // File Path Builders (with filename parameter)
    // ========================================================================

    QString getConfigPath(const QString& filename) const {
        return QDir(m_configDir).filePath(filename);
    }

    QString getLogPath(const QString& filename) const {
        return QDir(m_logsDir).filePath(filename);
    }

    QString getFirmwarePath(const QString& filename) const {
        return QDir(m_firmwareDir).filePath(filename);
    }

    // ========================================================================
    // Utility Methods
    // ========================================================================

    void initialize() {
        // Already initialized in constructor, but provide no-op for compatibility
    }

    bool createDirectories() {
        bool configOk = QDir().mkpath(m_configDir);
        bool logsOk = QDir().mkpath(m_logsDir);
        bool firmwareOk = QDir().mkpath(m_firmwareDir);

        // All three must succeed
        return configOk && logsOk && firmwareOk;
    }

    bool verifyDirectories() const {
        return QFileInfo::exists(m_configDir) &&
            QFileInfo::exists(m_logsDir) &&
            QFileInfo::exists(m_firmwareDir);
    }

private:
    QString m_rootDir;      // Parent of bin (e.g., Z:/EmulatRAppUni/out/build/x64-debug)
    QString m_binDir;       // Exe directory (e.g., .../x64-debug/EmulatRAppUni)
    QString m_configDir;    // Config directory (bin/config)
    QString m_logsDir;      // Logs directory (bin/logs)
    QString m_firmwareDir;  // Firmware directory (bin/firmware)
};

#endif // EMULATORPATHS_H