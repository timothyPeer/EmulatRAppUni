#ifndef INITPHASELOGGING_H
#define INITPHASELOGGING_H
// ============================================================================
// InitPhaseLogger.h - Initialization Phase Tracking
// ============================================================================
#include "LoggingMacros.h"
#include <QString>
#include <QElapsedTimer>

class InitPhaseLogger {
public:
    InitPhaseLogger(const QString& phaseName)
        : m_phaseName(phaseName)
        , m_indent(s_depth * 2)  // 2 spaces per level
    {
        s_depth++;
        m_timer.start();

        INFO_LOG(QString("%1[Phase %2/%3] %4 - Starting...")
                     .arg(QString(m_indent, ' '))
                     .arg(s_phaseNumber)
                     .arg(s_totalPhases)
                     .arg(m_phaseName));
    }

    ~InitPhaseLogger() {
        qint64 elapsed = m_timer.elapsed();
        s_depth--;

        INFO_LOG(QString("%1[Phase %2/%3] %4 - Complete (%5 ms)")
                     .arg(QString(m_indent, ' '))
                     .arg(s_phaseNumber)
                     .arg(s_totalPhases)
                     .arg(m_phaseName)
                     .arg(elapsed));

        s_phaseNumber++;
    }

    void logDetail(const QString& detail) const
    {
        INFO_LOG(QString("%1  → %2")
                     .arg(QString(m_indent, ' '))
                     .arg(detail));
    }

    void logConfig(const QString& key, const QVariant& value) const
    {
        INFO_LOG(QString("%1  • %2: %3")
                     .arg(QString(m_indent, ' '))
                     .arg(key)
                     .arg(value.toString()));
    }

    static void setTotalPhases(int total) { s_totalPhases = total; }
    static void reset() { s_phaseNumber = 1; s_depth = 0; }

private:
    QString m_phaseName;
    int m_indent;
    QElapsedTimer m_timer;

    static inline int s_phaseNumber = 1;
    static inline int s_totalPhases = 0;
    static inline int s_depth = 0;
};
#endif // INITPHASELOGGING_H
