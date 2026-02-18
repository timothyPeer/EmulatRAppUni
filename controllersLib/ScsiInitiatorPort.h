// ============================================================================
// ScsiInitiatorPort.H  -  SCSI Initiator Port (SAM-2 Model)
// ============================================================================
// Purpose:
//   Defines a simple SCSI Initiator Port abstraction, representing the
//   host-side identity on a SCSI bus.
//
//   This class stores:
//     • Initiator port name (e.g. "scsi0")
//     • A 64-bit initiator WWN (or synthetic ID)
//     • Basic statistics counters (thread-safe optional)
//
//   It is used by:
//     • ScsiHostAdapter
//     • ScsiController
//     • GenericScsiHostAdapter
//
//   It does NOT depend on:
//     - SafeMemory
//     - AlphaCPU
//     - MMIOManager
//     - coreLib
//
// Design Constraints:
//   - Header-only
//   - Pure ASCII, UTF-8, no BOM
//   - QtCore-only (QtGlobal, QString, QMutex)
// ============================================================================

#ifndef SCSI_INITIATOR_PORT_H
#define SCSI_INITIATOR_PORT_H

#include <QtGlobal>
#include <QString>
#include <QMutex>

// ============================================================================
// ScsiInitiatorPort
// ============================================================================
class ScsiInitiatorPort
{
public:
	// Constructor
	ScsiInitiatorPort(const QString& name,
		quint64        wwn,
		bool           threadSafe = false) noexcept
		: m_name(name)
		, m_wwn(wwn)
		, m_threadSafe(threadSafe)
		, m_cmdCount(0)
		, m_dataBytes(0)
		, m_errorCount(0)
	{
	}

	~ScsiInitiatorPort() noexcept = default;

	// ------------------------------------------------------------------------
	// Identity
	// ------------------------------------------------------------------------

	inline QString name() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_name;
	}

	inline void setName(const QString& n) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_name = n;
	}

	inline quint64 wwn() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_wwn;
	}

	inline void setWwn(quint64 w) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_wwn = w;
	}

	// ------------------------------------------------------------------------
	// Statistics (simple, optional thread safety)
	// ------------------------------------------------------------------------

	inline void incrementCommandCount() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		++m_cmdCount;
	}

	inline quint64 commandCount() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_cmdCount;
	}

	inline void addDataBytes(quint64 bytes) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_dataBytes += bytes;
	}

	inline quint64 dataBytes() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_dataBytes;
	}

	inline void incrementErrorCount() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		++m_errorCount;
	}

	inline quint64 errorCount() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_errorCount;
	}

private:
	QString m_name;         // Initiator port logical name
	quint64 m_wwn;          // World-Wide Name (or synthetic ID)

	bool    m_threadSafe;
	mutable QMutex m_mutex;

	quint64 m_cmdCount;     // Total SCSI commands originated by this initiator
	quint64 m_dataBytes;    // Total data moved (read/write)
	quint64 m_errorCount;   // Total command errors observed
};

#endif // SCSI_INITIATOR_PORT_H

