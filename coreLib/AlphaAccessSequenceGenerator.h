//
// ============================================================================
//  ASA Emulator (c) 2025 Timothy Peer / eNVy Systems, Inc.
//  Non-Commercial Use Only.  Commercial use, resale, or sublicensing strictly
//  prohibited without prior written agreement.
//
//  License details: https://envysys.com/ASA/License.html
// ============================================================================
//

// GlobalAccessSequencer.h
// Provides globally incrementing sequence ID and timestamp for cache/memory access tracking
#pragma once

#include <QAtomicInteger>
#include <QtGlobal>
#include <QElapsedTimer>
#include <QPair>

class GlobalAccessSequencer
{
public:
	struct SequenceInfo {
		quint64 sequenceId;
		quint64 timestampNs;
	};

    static SequenceInfo next()
    {
        static QAtomicInteger<quint64> counter{1};

        // Start the timer exactly once using thread-safe local static init
        struct TimerHolder {
            QElapsedTimer timer;
            TimerHolder() { timer.start(); }
        };
        static TimerHolder th; // guaranteed thread-safe init in C++11+

        const quint64 seqId = counter.fetchAndAddRelaxed(1);               // returns old value
        const quint64 ts    = static_cast<quint64>(th.timer.nsecsElapsed()); // qint64 -> quint64

        return SequenceInfo{ seqId, ts };
    }

};

// Usage:
// GlobalAccessSequencer::SequenceInfo access = GlobalAccessSequencer::next();
// access.sequenceId -> unique id, access.timestampNs -> timestamp in ns
