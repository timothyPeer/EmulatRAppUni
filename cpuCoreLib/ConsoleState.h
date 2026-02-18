#ifndef ConsoleState_h__
#define ConsoleState_h__

// ============================================================================
// ConsoleState
// ----------------------------------------------------------------------------
// Firmware-visible console state (SRM / ARC style).
// Tracks HALT, RESET, and other console-visible CPU events.
// ============================================================================
#include <QtGlobal>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>
#include "ConsoleHaltInfo.h"


class ConsoleState final
{
public:
    // ---------------------------------------------------------------------
    // Public API
    // ---------------------------------------------------------------------
    void notifyCpuHalted(const ConsoleHaltInfo& info) noexcept
    {
        QMutexLocker lock(&m_mutex);

        if (info.cpuId >= static_cast<quint32>(m_perCpuHalt.size())) {
            m_perCpuHalt.resize(info.cpuId + 1);
        }

        m_perCpuHalt[info.cpuId] = info;
        m_lastHalt = info;
    }

    bool isCpuHalted(quint32 cpuId) const noexcept
    {
        QMutexLocker lock(&m_mutex);

        if (cpuId >= static_cast<quint32>(m_perCpuHalt.size())) {
            return false;
        }
        return m_perCpuHalt[cpuId].haltCode != 0 || cpuId == m_lastHalt.cpuId;
    }

    ConsoleHaltInfo lastHalt() const noexcept
    {
        QMutexLocker lock(&m_mutex);
        return m_lastHalt;
    }

	// NEW: CSERVE hook
	void notifyConsoleService(quint32 cpuId) noexcept
	{
		Q_UNUSED(cpuId);
		// TODO: SRM/ARC console service dispatch
	}
	// Allow singleton accessor to construct us
	friend ConsoleState& globalConsoleState() noexcept;
private:
    // Only globalConsoleState() may construct this
    ConsoleState() = default;

    // Non-copyable, non-movable
    ConsoleState(const ConsoleState&) = delete;
    ConsoleState& operator=(const ConsoleState&) = delete;

    mutable QMutex m_mutex;
    QVector<ConsoleHaltInfo> m_perCpuHalt;
    ConsoleHaltInfo m_lastHalt{};
};

// ============================================================================
// globalConsoleState
// ----------------------------------------------------------------------------
// Meyers' singleton accessor for ConsoleState.
// Thread-safe since C++11.
// ============================================================================
inline ConsoleState& globalConsoleState() noexcept
{
    static ConsoleState instance;
    return instance;
}


#endif // ConsoleState_h__
