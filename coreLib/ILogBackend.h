#ifndef ILOGBACKEND_H
#define ILOGBACKEND_H
// ============================================================================
//  ASA Emulator (c) 2025 Timothy Peer / eNVy Systems, Inc.
//  Non-Commercial Use Only.
// ============================================================================
//
//  ILogBackend.h
//
//  Purpose: Abstract interface for log backends
//
// ============================================================================



#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QThread>
#include <QtGlobal>
#include <cstring>



// Forward declarations
// struct LogEntry; // Now defined below

enum class LogLevel : quint8 {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Critical = 5
};

// ============================================================================
// LogEntry - Complete log entry structure  
// ============================================================================
struct LogEntry {
    LogLevel level{LogLevel::Info};
    QString message;
    QDateTime timestamp{QDateTime::currentDateTime()};
    quint32 threadId{0};
    quint32 cpuId{0};
    int callDepth{0};
    QString component;
    QString logFunction;
    QString file;
    int line{0};
    quint64 pc{0};
    quint64 instructionCount{0};
    QString palMode;
    QString category;
    QString subcategory;

    LogEntry() = default;
    
    LogEntry(LogLevel lvl, const QString& msg)
        : level(lvl), message(msg), timestamp(QDateTime::currentDateTime())
    {
        threadId = static_cast<quint32>(reinterpret_cast<quint64>(QThread::currentThreadId()));
    }

    LogEntry(LogLevel lvl, const QString& msg, const QString& comp, int depth = 0)
        : level(lvl), message(msg), component(comp), callDepth(depth)
        , timestamp(QDateTime::currentDateTime())
    {
        threadId = static_cast<quint32>(reinterpret_cast<quint64>(QThread::currentThreadId()));
    }
};

struct alignas(8) LogMessage {
    LogLevel level;
    quint64 timestamp;
    quint32 threadId;
    char text[512];

    LogMessage() noexcept
        : level(LogLevel::Info), timestamp(0), threadId(0), text{ 0 }
    {
    }
};

// ============================================================================
// LOG BACKEND INTERFACE
// ============================================================================

class ILogBackend {
public:
    virtual ~ILogBackend() = default;
    virtual void write(const LogMessage& msg) = 0;
    virtual void flush() = 0;
    virtual bool accepts(LogLevel level) const = 0;

    // Enhanced interface for LogEntry support
    virtual void writeLog(const LogEntry& entry) = 0;
};

#endif // ILOGBACKEND_H
