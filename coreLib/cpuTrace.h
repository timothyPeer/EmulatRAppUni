#ifndef CPUTRACE_H
#define CPUTRACE_H
// ============================================================================
// cpuTrace.h -- Alpha AXP CPU Instruction Trace
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025, 2026 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// ============================================================================

#include <QFile>
#include <QTextStream>
#include <QString>
#include <QMutex>
#include <QElapsedTimer>
#include <cstdint>

class CpuTrace
{
public:
    // -----------------------------------------------------------------------
    // Trace levels (bitmask)
    // -----------------------------------------------------------------------
    enum TraceMask : uint32_t {
        TRACE_NONE      = 0,
        TRACE_INSTR     = 1 << 0,
        TRACE_INTEGER   = 1 << 1,
        TRACE_FP        = 1 << 2,
        TRACE_MEMORY    = 1 << 3,
        TRACE_PAL       = 1 << 4,
        TRACE_TLB       = 1 << 5,
        TRACE_PIPELINE  = 1 << 6,
        TRACE_EVENT     = 1 << 7,
        TRACE_ALL       = 0xFFFFFFFF
    };

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    static void initialize(const QString& decLogFile,
                           const QString& machineLogFile);

    static void setTraceMask(uint32_t mask);
    static uint32_t getTraceMask();

    // -----------------------------------------------------------------------
    // Instruction trace
    // -----------------------------------------------------------------------

    static void instruction(uint64_t        cycle,
                            uint64_t        pc,
                            uint32_t        rawInstr,
                            const char*     mnemonic,
                            const QString&  operands,
                            const QString&  result);

    // -----------------------------------------------------------------------
    // Register delta logging
    // -----------------------------------------------------------------------

    static void logRegisterDeltas(uint64_t        cycle,
                                  const uint64_t  intRegs[32],
                                  const uint64_t  fpRegs[32]);

    // -----------------------------------------------------------------------
    // Events and pipeline
    // -----------------------------------------------------------------------

    static void event(uint64_t     cycle,
                      const char*  eventType,
                      uint64_t     pc = 0);

    static void pipeline(uint64_t       cycle,
                         const char*    stage,
                         const QString& info);

    // -----------------------------------------------------------------------
    // Elapsed time -- hh:mm:ss.mmm
    // -----------------------------------------------------------------------

    static void startElapsedTime() noexcept
    {
        m_timer.start();
    }

    /// Returns elapsed time formatted as hh:mm:ss.mmm
    static QString elapsedTime() noexcept
    {
        const qint64 totalMs = m_timer.elapsed();
        const qint64 hours   =  totalMs / 3600000LL;
        const qint64 mins    = (totalMs %  3600000LL) / 60000LL;
        const qint64 secs    = (totalMs %    60000LL) /  1000LL;
        const qint64 ms      =  totalMs %     1000LL;

        return QString("%1:%2:%3.%4")
            .arg(hours,  2, 10, QChar('0'))
            .arg(mins,   2, 10, QChar('0'))
            .arg(secs,   2, 10, QChar('0'))
            .arg(ms,     3, 10, QChar('0'));
    }

    /// Returns elapsed milliseconds as raw integer (for calculations)
    static qint64 elapsedMs() noexcept
    {
        return m_timer.elapsed();
    }

    /// Writes "Elapsed hh:mm:ss.mmm" banner line to the machine log
    static void writeElapsedToMachineLine() noexcept
    {
        writeMachineLine(QString("# Elapsed %1").arg(elapsedTime()));
        flush();
    }

    // -----------------------------------------------------------------------
    // Buffer control
    // -----------------------------------------------------------------------

    static void flush();

private:
    static QFile        decFile;
    static QTextStream  decOut;

    static QFile        machineFile;
    static QTextStream  machineOut;

    static uint32_t     traceMask;

    static uint64_t     shadowIntRegs[32];
    static uint64_t     shadowFpRegs[32];

    static QMutex       mutex;

    inline static QElapsedTimer m_timer;   // C++17 inline static -- no .cpp definition needed

    static void writeDecLine    (const QString& line);
    static void writeMachineLine(const QString& line);
};

#endif // CPUTRACE_H
