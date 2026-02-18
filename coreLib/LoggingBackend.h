#ifndef LOGGINGBACKEND_H
#define LOGGINGBACKEND_H
// ============================================================================
// LoggingBackend.h - Fixed File Initialization
// ============================================================================



#include <QDebug>
#include <QString>
#include <QMutex>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <chrono>
#include "Axp_Attributes_core.h"
#include "../configLib/global_EmulatorSettings.h"



// Configuration flags (set once at startup)
struct LogConfig {
	static inline bool enableFileOutput = true;
	static inline bool enableConsole = true;
	static inline LogSeverity minLevel = LOG_TRACE;
	static inline QString logFileName = "emulatr.log";
	static inline QString networkApiUrl;
	static inline quint32 maxLogFileSizeBytes;
	static inline quint8 maxLogFileCount;
	static inline bool enableTimestamps = true;
	static inline bool useHighPerfTimestamps = true;
	static inline bool appendToExisting = true;  // true=append, false=rename old

	static AXP_FLATTEN void initializeFromSettings() noexcept {

		auto& emu = global_EmulatorSettings();
		enableFileOutput = emu.podData.logging.enableDiskLogging;
		logFileName = emu.podData.logging.logFileName;
		minLevel = static_cast<LogSeverity>(emu.podData.logging.logLevel);
		enableFileOutput = emu.podData.logging.enableDiskLogging;
		enableConsole = emu.podData.logging.enableConsole;
		networkApiUrl = emu.podData.logging.networkApiUrl;
		maxLogFileSizeBytes = emu.podData.logging.maxLogFileSizeBytes;
		maxLogFileCount = emu.podData.logging.maxLogFileCount;
		appendToExisting = emu.podData.logging.appendToExisting;
		enableTimestamps = emu.podData.logging.enableTimestamps;
		useHighPerfTimestamps = emu.podData.logging.useHighPerfTimestamps;
	}
};

// ============================================================================
// Timestamp Utilities
// ============================================================================

namespace LoggingInternal {

	inline QString getHighPerfTimestamp() noexcept
	{
		using namespace std::chrono;

		auto now = system_clock::now();
		auto nowTime_t = system_clock::to_time_t(now);
		auto nowMs = duration_cast<microseconds>(now.time_since_epoch()) % 1000000;

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
			static_cast<long long>(nowMs.count()));

		return QString::fromLatin1(buffer);
	}

	inline QString getQtTimestamp() noexcept
	{
		return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
	}

	inline QString getTimestamp() noexcept
	{
		if (!LogConfig::enableTimestamps) {
			return QString();
		}

		return LogConfig::useHighPerfTimestamps
			? getHighPerfTimestamp()
			: getQtTimestamp();
	}

	inline const char* severityToString(LogSeverity severity) noexcept
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

	static QMutex g_initMutex;
	static bool g_initialized = false;

} // namespace LoggingInternal

// ============================================================================
// Public Initialization (Call this BEFORE any logging!)
// ============================================================================

inline void InitializeFileLogging() noexcept
{
	QMutexLocker lock(&LoggingInternal::g_initMutex);

	if (LoggingInternal::g_initialized) {
		qDebug() << "Logging: Already initialized, skipping";
		return;
	}

	LogConfig::initializeFromSettings();
	LoggingInternal::g_initialized = true;

	if (!LogConfig::enableFileOutput) {
		qDebug() << "Logging: File output disabled";
		return;
	}

	// Ensure log directory exists
	QFileInfo logInfo(LogConfig::logFileName);
	QDir logDir = logInfo.absoluteDir();
	if (!logDir.exists()) {
		if (logDir.mkpath(".")) {
			qDebug() << "Logging: Created log directory" << logDir.absolutePath();
		}
		else {
			qWarning() << "Logging: Failed to create log directory" << logDir.absolutePath();
			return;
		}
	}

	// If appending to existing, nothing more to do
	if (LogConfig::appendToExisting) {
		qDebug() << "Logging: Appending to" << LogConfig::logFileName;
		return;
	}

	// Rename old log file if it exists
	if (logInfo.exists()) {

		// Generate timestamp for old log
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

		// Try to rename
		QFile oldLog(LogConfig::logFileName);

		// Ensure file is closed by scoping
		{
			// Force any handles closed (shouldn't be any, but just in case)
			if (oldLog.exists()) {
				QFile temp(LogConfig::logFileName);
				temp.close();
			}
		}

		if (oldLog.rename(backupName)) {
			qDebug() << "Logging: Renamed old log:" << LogConfig::logFileName
				<< "->" << backupName;
		}
		else {
			qWarning() << "Logging: Failed to rename old log (error:"
				<< oldLog.errorString() << "), will append instead";
			// Don't return - still create the header
		}
	}

	// Create fresh log file with header
	QFile newLog(LogConfig::logFileName);
	if (newLog.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		QTextStream out(&newLog);
		out << "============================================================\n";
		out << "ASA EmulatR Log File\n";
		out << "Started: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
		out << "============================================================\n\n";
		out.flush();
		newLog.close();
		qDebug() << "Logging: Created fresh log file" << LogConfig::logFileName;
	}
	else {
		qWarning() << "Logging: Failed to create log file" << LogConfig::logFileName
			<< "error:" << newLog.errorString();
	}
}

inline void ShutdownFileLogging() noexcept
{
	QMutexLocker lock(&LoggingInternal::g_initMutex);

	if (!LoggingInternal::g_initialized) return;

	// Write shutdown marker
	if (LogConfig::enableFileOutput) {
		QFile file(LogConfig::logFileName);
		if (file.open(QIODevice::Append | QIODevice::Text)) {
			QTextStream out(&file);
			out << "\n============================================================\n";
			out << "Shutdown: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
			out << "============================================================\n";
			out.flush();
			file.close();
		}
	}

	LoggingInternal::g_initialized = false;
	qDebug() << "Logging: Shutdown complete";
}

// ============================================================================
// Thread-Safe Logger
// ============================================================================

inline void LogMessage(LogSeverity severity, const QString& msg) noexcept
{
	// Early exit for filtered messages
	if (severity < LogConfig::minLevel) return;

	// Format log line with timestamp and severity
	QString logLine;
	if (LogConfig::enableTimestamps) {
		logLine = QString("[%1] [%2] %3")
			.arg(LoggingInternal::getTimestamp())
			.arg(LoggingInternal::severityToString(severity))
			.arg(msg);
	}
	else {
		logLine = QString("[%1] %2")
			.arg(LoggingInternal::severityToString(severity))
			.arg(msg);
	}

#if !defined(NDEBUG)
	// Debug builds: console output
	if (LogConfig::enableConsole) {
		qDebug().noquote() << logLine;
	}
#endif

	// File output (both debug and release)
	if (LogConfig::enableFileOutput) {
		static QMutex logMutex;
		QMutexLocker lock(&logMutex);

		QFile file(LogConfig::logFileName);
		if (file.open(QIODevice::Append | QIODevice::Text)) {
			QTextStream out(&file);
			out << logLine << "\n";
			out.flush();  // Ensure written immediately
			file.close(); // Close immediately
		}
	}
}

#endif // LOGGINGBACKEND_H