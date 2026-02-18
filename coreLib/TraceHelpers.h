#ifndef TraceHelpers_h__
#define TraceHelpers_h__
#include <QString>
#include <string>
#include "Axp_Attributes_core.h"
#include "TraceCore.h"


// ================================================================
// LOGGING HELPERS — dual overload design
// ================================================================

// ------------------------------
// QString overload (primary API)
// ------------------------------
AXP_FLATTEN void logTrace(const QString& msg) noexcept
{
#if AXP_TRACE_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Trace, msg);
#else
    Q_UNUSED(msg)
#endif
}

AXP_FLATTEN
void logInfo(const QString& msg) noexcept
{
	TraceCore::emit(TraceCore::LoggingLevel::Info, msg);
}

AXP_FLATTEN
void logInfo(const std::string& msg) noexcept
{
	TraceCore::emit(
		TraceCore::LoggingLevel::Info,
		QString::fromStdString(msg)
	);
}

// ================================================================
// Canonical overload: QString
// ================================================================
AXP_FLATTEN void logWarning(const QString& msg) noexcept
{
#if AXP_WARN_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Warning, msg);
#else
    Q_UNUSED(msg);
#endif
}

// ================================================================
// const char* overload — avoids ambiguity between QString and std::string
// ================================================================
AXP_FLATTEN void logWarning(const char* msg) noexcept
{
#if AXP_WARN_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Warning, QString::fromUtf8(msg));
#else
    Q_UNUSED(msg)
#endif
}

// ================================================================
// std::string overload — only used when actual std::string is passed
// ================================================================
AXP_FLATTEN void logWarning(const std::string& msg) noexcept
{
#if AXP_WARN_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Warning,
                    QString::fromStdString(msg));
#else
    Q_UNUSED(msg)
#endif
}




AXP_FLATTEN void logCritical(const QString& msg) noexcept
{
#if AXP_CRITICAL_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Critical, msg);
#else
    Q_UNUSED(msg)
#endif
}


// ----------------------------------------------------------
// std::string overload -> forwards to QString under the hood
// ----------------------------------------------------------
AXP_FLATTEN void logTrace(const std::string& msg) noexcept
{
#if AXP_TRACE_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Trace, QString::fromStdString(msg));
#else
    Q_UNUSED(msg)
#endif
}

// ================================================================
    // Canonical overload: QString
    // ================================================================
    AXP_FLATTEN void logDebug(const QString& msg) noexcept
{
#if AXP_DEBUG_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Debug, msg);
#else
    Q_UNUSED(msg);
#endif
}

// ================================================================
// const char* overload — avoids ambiguity between QString and std::string
// ================================================================
AXP_FLATTEN void logDebug(const char* msg) noexcept
{
#if AXP_DEBUG_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Debug, QString::fromUtf8(msg));
#else
    Q_UNUSED(msg)
#endif
}

// ================================================================
// std::string overload — only chosen when argument is actually std::string
// ================================================================
AXP_FLATTEN void logDebug(const std::string& msg) noexcept
{
#if AXP_DEBUG_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Debug,
                    QString::fromStdString(msg));
#else
    Q_UNUSED(msg)
#endif
}

// ================================================================
// Canonical overload: QString
// ================================================================
AXP_FLATTEN void logError(const QString& msg) noexcept
{
#if AXP_DEBUG_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Error, msg);
#else
    Q_UNUSED(msg)
#endif
}

// ================================================================
// const char* overload — resolves literal ambiguity
// ================================================================
AXP_FLATTEN void logError(const char* msg) noexcept
{
#if AXP_DEBUG_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Error, QString::fromUtf8(msg));
#else
    Q_UNUSED(msg)
#endif
}

// ================================================================
// std::string overload — only matched when an actual std::string is passed
// ================================================================
AXP_FLATTEN void logError(const std::string& msg) noexcept
{
#if AXP_DEBUG_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Error,
                    QString::fromStdString(msg));
#else
    Q_UNUSED(msg)
#endif
}




AXP_FLATTEN void logCritical(const std::string& msg) noexcept
{
#if AXP_CRITICAL_ENABLED
    TraceCore::emit(TraceCore::LoggingLevel::Critical, QString::fromStdString(msg));
#else
    Q_UNUSED(msg)
#endif
}
#endif // TraceHelpers_h__






/*
 *
 *				DEBUG_LOG(std::format("PREPARE_VA_XLATE VA: 0x{:X}", va));
 HEX:			DEBUG_LOG(std::format("IRQ Fired for CPU {:X}", cpuId));
 PADDING:		DEBUG_LOG(std::format("Registered IRQ vector: 0x{:04X}", vector));
 FIXED Width:	DEBUG_LOG(std::format("CPU {:>2} entering PAL mode", cpuId));
 *
 *
 */
