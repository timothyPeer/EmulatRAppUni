// ============================================================================
// EventLog.cpp - Event Logging Implementation
// ============================================================================

#include "EventLog.h"
#include "../configLib/global_EmulatorSettings.h"
#include <QMutex>
#include <QMutexLocker>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <chrono>
#include <QDebug>

#include "emulatrLib/EmulatorPaths.h"

// ============================================================================
// Internal Implementation Class
// ============================================================================

class EventLog::Impl
{
public:
    // Configuration (loaded from EmulatorSettings)
    static inline bool s_initialized{false};
    static inline bool s_enableFileOutput{true};
    static inline bool s_enableConsole{true};
    static inline LogSeverity s_minLevel{LOG_TRACE};
    static inline QString s_logFileName;
    static inline quint64 s_maxLogFileSizeBytes{104857600};  // 100 MB
    static inline quint32 s_maxLogFileCount{10};
    static inline bool s_appendToExisting{true};
    static inline bool s_enableTimestamps{true};
    static inline bool s_useHighPerfTimestamps{true};
    static inline quint32 s_flushInterval{ 10 };  // 
    // Thread safety
    static inline QMutex s_initMutex;
    static inline QMutex s_writeMutex;
	// log to buffered IO
	static inline QFile s_logFile;
	static inline QTextStream s_logStream;
    static inline QTimer* s_flushTimer{ nullptr };

    // ========================================================================
    // Timestamp Generation
    // ========================================================================

    static QString getTimestamp() noexcept
    {
        if (!s_enableTimestamps) {
            return QString();
        }

        if (s_useHighPerfTimestamps) {
            return getHighPerfTimestamp();
        } else {
            return getQtTimestamp();
        }
    }

    static QString getHighPerfTimestamp() noexcept
    {
        using namespace std::chrono;

        auto now = system_clock::now();
        auto nowTime_t = system_clock::to_time_t(now);
        auto nowUs = duration_cast<microseconds>(now.time_since_epoch()) % 1000000;

        std::tm localTime;
#ifdef _WIN32
        localtime_s(&localTime, &nowTime_t);
#else
        localtime_r(&nowTime_t, &localTime);
#endif

        char buffer[32];
        std::snprintf(buffer, sizeof(buffer),
                      "%04d-%02d-%02d %02d:%02d:%02d.%06lld",
                      localTime.tm_year + 1900,
                      localTime.tm_mon + 1,
                      localTime.tm_mday,
                      localTime.tm_hour,
                      localTime.tm_min,
                      localTime.tm_sec,
                      static_cast<long long>(nowUs.count()));

        return QString::fromLatin1(buffer);
    }

    static QString getQtTimestamp() noexcept
    {
        return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    }

    // ========================================================================
    // Severity to String
    // ========================================================================

    static const char* severityToString(LogSeverity severity) noexcept
    {
        switch (severity) {
        case LOG_TRACE:    return "TRACE";
        case LOG_DEBUG:    return "DEBUG";
        case LOG_INFO:     return "INFO ";
        case LOG_WARN:     return "WARN ";
        case LOG_ERROR:    return "ERROR";
        case LOG_CRITICAL: return "CRIT ";
        default:           return "?????";
        }
    }

    // ========================================================================
    // File Management
    // ========================================================================

    static bool createLogDirectory() noexcept
    {
        QFileInfo logInfo(s_logFileName);
        QDir logDir = logInfo.absoluteDir();

        if (!logDir.exists()) {
            if (!logDir.mkpath(".")) {
                qWarning() << "EventLog: Failed to create log directory" << logDir.absolutePath();
                return false;
            }
            qDebug() << "EventLog: Created log directory" << logDir.absolutePath();
        }

        return true;
    }

    static bool renameOldLog() noexcept
    {
        QFileInfo logInfo(s_logFileName);

        if (!logInfo.exists()) {
            return true;  // No old log to rename
        }

        // Generate timestamp for backup
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

        // Create backup filename: emulatr.log -> emulatr_20260114_064523.log.old
        QString baseName = logInfo.completeBaseName();
        QString extension = logInfo.suffix();
        QString dir = logInfo.absolutePath();

        QString backupName = QString("%1/%2_%3.%4.old")
                                 .arg(dir)
                                 .arg(baseName)
                                 .arg(timestamp)
                                 .arg(extension);

        QFile oldLog(s_logFileName);

        if (oldLog.rename(backupName)) {
            qDebug() << "EventLog: Renamed old log:" << s_logFileName
                     << "->" << backupName;
            return true;
        } else {
            qWarning() << "EventLog: Failed to rename old log (error:"
                       << oldLog.errorString() << "), will append instead";
            return false;
        }
    }

	static bool createLogFileHeader() noexcept
	{
		//  Open file once and keep it open
		s_logFile.setFileName(s_logFileName);

		if (!s_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
			qWarning() << "EventLog: Failed to create log file" << s_logFileName
				<< "error:" << s_logFile.errorString();
			return false;
		}

		// Attach stream to file (keep attached)
		s_logStream.setDevice(&s_logFile);

		// Write header
		s_logStream << "============================================================\n";
		s_logStream << "ASA EmulatR Event Log\n";
		s_logStream << "Started: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
		s_logStream << "============================================================\n\n";
		s_logStream.flush();  // Flush header immediately

		qDebug() << "EventLog: Created fresh log file" << s_logFileName;
		return true;
	}

	static void writeShutdownMarker() noexcept
	{
		if (!s_enableFileOutput) return;
		if (!s_logFile.isOpen()) return;

		s_logStream << "\n============================================================\n";
		s_logStream << "Shutdown: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
		s_logStream << "============================================================\n";
		s_logStream.flush();

		// Close file on shutdown
		s_logFile.close();
	}

    // ========================================================================
    // Format Log Line
    // ========================================================================

    static QString formatLogLine(LogSeverity severity, const QString& msg,
                                 int cpuId = -1) noexcept
    {
        QString logLine;

        if (s_enableTimestamps) {
            if (cpuId >= 0) {
                // With CPU context
                logLine = QString("[%1] [CPU%2] [%3] %4")
                              .arg(getTimestamp())
                              .arg(cpuId)
                              .arg(severityToString(severity))
                              .arg(msg);
            } else {
                // Without CPU context
                logLine = QString("[%1] [%2] %3")
                              .arg(getTimestamp())
                              .arg(severityToString(severity))
                              .arg(msg);
            }
        } else {
            if (cpuId >= 0) {
                logLine = QString("[CPU%1] [%2] %3")
                .arg(cpuId)
                    .arg(severityToString(severity))
                    .arg(msg);
            } else {
                logLine = QString("[%1] %2")
                .arg(severityToString(severity))
                    .arg(msg);
            }
        }

        return logLine;
    }

    // ========================================================================
    // Write to Console
    // ========================================================================

    static void writeToConsole(const QString& logLine) noexcept
    {
#if !defined(NDEBUG)
        if (s_enableConsole) {
            qDebug().noquote() << logLine;
        }
#else
        Q_UNUSED(logLine);
#endif
    }

    // ========================================================================
    // Write to File
    // ========================================================================

	static void writeToFile(const QString& logLine, LogSeverity severity) noexcept
	{
		if (!s_enableFileOutput) return;
		if (!s_logFile.isOpen()) return;

		// Write to stream
		s_logStream << logLine << "\n";

#if !defined(NDEBUG)
		// ? DEBUG: Always flush immediately for debugging
		s_logStream.flush();
#else
		// RELEASE: Flush on ERROR/CRITICAL or periodically
		static int writeCount = 0;

		if (severity >= LOG_ERROR) {
			s_logStream.flush();
		}
		else if (++writeCount >= 10) {
			s_logStream.flush();
			writeCount = 0;
		}
#endif
	}
};

// ============================================================================
// EventLog Public Interface Implementation
// ============================================================================

bool EventLog::initialize() noexcept
{
    QMutexLocker lock(&Impl::s_initMutex);

    if (Impl::s_initialized) {
        qDebug() << "EventLog: Already initialized, skipping";
        return true;
    }

    // ========================================================================
    // Load configuration from EmulatorSettings (if available)
    // ========================================================================

    // Handle case where settings might not be loaded yet
    const bool settingsAvailable = global_EmulatorSettings().bAlreadyInitialized;  // Add this check function

    if (settingsAvailable) {
        auto& settings = global_EmulatorSettings();

        Impl::s_enableFileOutput = settings.podData.logging.enableDiskLogging;
        Impl::s_enableConsole = settings.podData.logging.enableConsole;
        Impl::s_minLevel = static_cast<LogSeverity>(settings.podData.logging.logLevel);
        Impl::s_maxLogFileSizeBytes = settings.podData.logging.maxLogFileSizeBytes;
        Impl::s_maxLogFileCount = settings.podData.logging.maxLogFileCount;
        Impl::s_appendToExisting = settings.podData.logging.appendToExisting;
        Impl::s_enableTimestamps = settings.podData.logging.enableTimestamps;
        Impl::s_useHighPerfTimestamps = settings.podData.logging.useHighPerfTimestamps;

        // Get log filename from settings
        QString logFileName = settings.podData.logging.logFileName;

        // Resolve full path using EmulatorPaths
        EmulatorPaths paths;
        Impl::s_logFileName = paths.getLogPath(logFileName);
    }
    else {
        // Use safe defaults if settings not loaded yet
        qDebug() << "EventLog: Settings not available, using defaults";

        Impl::s_enableFileOutput = true;
        Impl::s_enableConsole = true;
        Impl::s_minLevel = LOG_INFO;
        Impl::s_enableTimestamps = true;
        Impl::s_useHighPerfTimestamps = true;
        Impl::s_appendToExisting = false;

        // Default log location
        EmulatorPaths paths;
        Impl::s_logFileName = paths.getLogPath("es40_instance.log");
    }

    Impl::s_initialized = true;

    if (!Impl::s_enableFileOutput) {
        qDebug() << "EventLog: File output disabled";
        return true;
    }

    // ========================================================================
    // Verify log directory exists (EmulatorPaths should have created it)
    // ========================================================================

    QFileInfo logInfo(Impl::s_logFileName);
    if (!logInfo.absoluteDir().exists()) {
        qWarning() << "EventLog: Log directory doesn't exist, creating:"
            << logInfo.absolutePath();
        if (!QDir().mkpath(logInfo.absolutePath())) {
            qCritical() << "EventLog: Failed to create log directory";
            return false;
        }
    }

    // ========================================================================
    // Open log file
    // ========================================================================

    Impl::s_logFile.setFileName(Impl::s_logFileName);

    QIODevice::OpenMode mode = QIODevice::WriteOnly | QIODevice::Text;

    if (Impl::s_appendToExisting && Impl::s_logFile.exists()) {
        mode |= QIODevice::Append;
        qDebug() << "EventLog: Appending to" << Impl::s_logFileName;
    }
    else {
        // Rename old log if exists
        Impl::renameOldLog();
        mode |= QIODevice::Truncate;
    }

    if (!Impl::s_logFile.open(mode)) {
        qWarning() << "EventLog: Failed to open log file:" << Impl::s_logFileName
            << "error:" << Impl::s_logFile.errorString();
        return false;
    }

    // Attach stream (keep attached for entire session)
    Impl::s_logStream.setDevice(&Impl::s_logFile);

    // Write header if new file
    if (!(mode & QIODevice::Append)) {
        Impl::s_logStream << "============================================================\n";
        Impl::s_logStream << "ASA EmulatR Event Log\n";
        Impl::s_logStream << "Started: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
        Impl::s_logStream << "============================================================\n\n";
        Impl::s_logStream.flush();
    }

    // Setup periodic flush timer
    Impl::s_flushTimer = new QTimer();
    QObject::connect(Impl::s_flushTimer, &QTimer::timeout, []() {
        QMutexLocker lock(&Impl::s_writeMutex);
        if (Impl::s_logStream.device()) {
            Impl::s_logStream.flush();
        }
        });
    Impl::s_flushTimer->start(1000);  // Flush every 1 second

    qDebug() << "EventLog: Initialized successfully";
    qDebug() << "  -> Log file:" << Impl::s_logFileName;
    qDebug() << "  -> Disk logging:" << (Impl::s_enableFileOutput ? "enabled" : "disabled");
    qDebug() << "  -> Log level:" << Impl::s_minLevel;

    return true;
}

void EventLog::shutdown() noexcept
{
    QMutexLocker lock(&Impl::s_initMutex);

    if (!Impl::s_initialized) return;

    // Write shutdown marker
    Impl::writeShutdownMarker();


	if (Impl::s_flushTimer) {
		Impl::s_flushTimer->stop();
		delete Impl::s_flushTimer;
		Impl::s_flushTimer = nullptr;
	}

    Impl::s_initialized = false;
    qDebug() << "EventLog: Shutdown complete";
}



void EventLog::write(LogSeverity severity, const QString& msg) noexcept
{
    // Early exit for filtered messages
    if (!Impl::s_initialized) return;
    if (severity < Impl::s_minLevel) return;

    // Format log line
    QString logLine = Impl::formatLogLine(severity, msg);

    // Thread-safe write
    QMutexLocker lock(&Impl::s_writeMutex);

    // Write to console (debug builds only)
    Impl::writeToConsole(logLine);

    // Write to file
    Impl::writeToFile(logLine, severity);
}

void EventLog::writeCpu(uint16_t cpuId, LogSeverity severity, const QString& msg) noexcept
{
    // Early exit for filtered messages
    if (!Impl::s_initialized) return;
    if (severity < Impl::s_minLevel) return;

    // Format log line with CPU context
    QString logLine = Impl::formatLogLine(severity, msg, cpuId);

    // Thread-safe write
    QMutexLocker lock(&Impl::s_writeMutex);

    // Write to console (debug builds only)
    Impl::writeToConsole(logLine);

    // Write to file
    Impl::writeToFile(logLine, severity);
}

bool EventLog::isEnabled() noexcept
{
    return Impl::s_initialized &&
           (Impl::s_enableFileOutput || Impl::s_enableConsole);
}

bool EventLog::isEnabledForSeverity(LogSeverity severity) noexcept
{
    return Impl::s_initialized && (severity >= Impl::s_minLevel);
}

void EventLog::flush() noexcept
{
	QMutexLocker lock(&Impl::s_writeMutex);

	if (Impl::s_logFile.isOpen()) {
		Impl::s_logStream.flush();
	}
}
