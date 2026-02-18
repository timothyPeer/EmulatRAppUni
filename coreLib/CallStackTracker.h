#ifndef CALLSTACKTRACKER_H
#define CALLSTACKTRACKER_H
#include <QtGlobal>
#include <QString>
#include "LoggingMacros.h"

class CallStackTracker {
private:
    thread_local static int s_callDepth;

public:
    CallStackTracker(const QString& function) : m_function(function) {
        s_callDepth++;
        DEBUG_LOG(QString("-> %1").arg(function), s_callDepth);
    }

    ~CallStackTracker() {
        DEBUG_LOG(QString("<- %1").arg(m_function), s_callDepth);
        s_callDepth--;
    }
    static int currentDepth() noexcept { return s_callDepth;  }
private:
    QString m_function;
};

#define TRACE_FUNCTION() CallStackTracker _trace(__FUNCTION__)


// ============================================================================
// Enhanced Logging Macros with Call Stack
// ============================================================================

#define TRACE_FUNCTION() CallStackTracker _trace(__FUNCTION__)
#define TRACE_FUNCTION_COMPONENT(comp) CallStackTracker _trace(__FUNCTION__, comp)

#define TRACE_LOG_CONTEXT(msg, component) \
global_LoggingSystem().logWithContext(LogLevel::Trace, component, msg, \
                                      CallStackTracker::currentDepth(), \
                                      __FUNCTION__, __FILE__, __LINE__)

#define DEBUG_LOG_CONTEXT(msg, component) \
    global_LoggingSystem().logWithContext(LogLevel::Debug, component, msg, \
                        CallStackTracker::currentDepth(), \
                        __FUNCTION__, __FILE__, __LINE__)

#define INFO_LOG_CONTEXT(msg, component) \
    global_LoggingSystem().logWithContext(LogLevel::Info, component, msg, \
                        CallStackTracker::currentDepth(), \
                        __FUNCTION__, __FILE__, __LINE__)

#define ERROR_LOG_CONTEXT(msg, component) \
    global_LoggingSystem().logWithContext(LogLevel::Error, component, msg, \
                        CallStackTracker::currentDepth(), \
                        __FUNCTION__, __FILE__, __LINE__)
#endif // CALLSTACKTRACKER_H
