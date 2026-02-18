#ifndef LoggingMacros_h__
#define LoggingMacros_h__

// ============================================================================
// LoggingMacros.h - Routes to EventLog Static Global
// ============================================================================

/*

### ** Unified Architecture:**
```
EventLog (event logging) - static global, settings-driven
ExecTrace (CPU trace)    - static global, settings-driven


// CPU-aware logging 
INFO_LOG_CPU(cpuId, "CPU halted");
ERROR_LOG_CPU(cpuId, "TLB miss");


*/

#include "EventLog.h"

#ifdef EXECTRACE_ENABLED
#define DEBUG_LOG(msg) qDebug() << msg
#define DEBUG_LOGF(fmt, ...) qDebug().noquote() << QString::asprintf(fmt, __VA_ARGS__)
#else
#define DEBUG_LOG(msg) do { } while(0)
#define DEBUG_LOGF(fmt, ...) do { } while(0)
#endif



// ============================================================================
// Standard Event Logging Macros (100% backward compatible)
// ============================================================================

#define INFO_LOG(msg)     EventLog::write(LOG_INFO, (msg))
#define WARN_LOG(msg)     EventLog::write(LOG_WARN, (msg))
#define ERROR_LOG(msg)    EventLog::write(LOG_ERROR, (msg))
#define CRITICAL_LOG(msg) EventLog::write(LOG_CRITICAL, (msg))

// Debug-only (compiled out in release)
#if !defined(NDEBUG)
#define DEBUG_LOG(msg)  EventLog::write(LOG_DEBUG, (msg))
#define TRACE_LOG(msg)  EventLog::write(LOG_TRACE, (msg))
#else
#define DEBUG_LOG(msg)  do {} while (0)
#define TRACE_LOG(msg)  do {} while (0)
#endif

// ============================================================================
// CPU-Aware Event Logging Macros
// ============================================================================

#define INFO_LOG_CPU(cpuId, msg)     EventLog::writeCpu(cpuId, LOG_INFO, (msg))
#define WARN_LOG_CPU(cpuId, msg)     EventLog::writeCpu(cpuId, LOG_WARN, (msg))
#define ERROR_LOG_CPU(cpuId, msg)    EventLog::writeCpu(cpuId, LOG_ERROR, (msg))
#define CRITICAL_LOG_CPU(cpuId, msg) EventLog::writeCpu(cpuId, LOG_CRITICAL, (msg))

#if !defined(NDEBUG)
#define DEBUG_LOG_CPU(cpuId, msg)  EventLog::writeCpu(cpuId, LOG_DEBUG, (msg))
#define TRACE_LOG_CPU(cpuId, msg)  EventLog::writeCpu(cpuId, LOG_TRACE, (msg))
#else
#define DEBUG_LOG_CPU(cpuId, msg)  do {} while (0)
#define TRACE_LOG_CPU(cpuId, msg)  do {} while (0)
#endif

#endif // LoggingMacros_h__
