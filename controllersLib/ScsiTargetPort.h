
// ============================================================================
// ScsiTargetPort.H  -  SCSI Target Port Abstraction (controllerLib)
// ============================================================================
// This header defines a lightweight SCSI Target Port abstraction that models
// the "target-side" identity in the SAM-2 architecture. Whereas a VirtualScsiDevice
// represents a specific logical unit (LU), a Target Port represents the port
// that contains one or more LUNs exposed to initiators.
//
// ScsiTargetPort provides:
//   - Identity (port name, 64-bit WWN-style identifier)
//   - Association with a ScsiBus (logical domain)
//   - Attach/detach LUNs to this target port
//   - Statistics (commands received, bytes in/out)
//   - A dispatch() convenience wrapper for routing commands to the bus
//
// Design Constraints:
//   - Header-only, no .CPP file.
//   - Depends only on QtCore, scsiCoreLib, ScsiBus, ScsiController, ScsiTypes.
//   - Absolutely no dependency on coreLib (AlphaCPU, MMIO, IPR, PAL, PTE).
//   - Pure ASCII, UTF-8 (no BOM).
//
// Reference Concepts:
//   - SAM-2 "Target Port" entity
//   - SPC-3 device command model
// ============================================================================

#ifndef SCSI_TARGET_PORT_H
#define SCSI_TARGET_PORT_H

#include <QtGlobal>
#include <QString>
#include <QDateTime>
#include <QMutex>

#include "ScsiBus.H"
#include "ScsiCoreLib.H"

// ============================================================================
// ScsiTargetPort
// ============================================================================
//
// This class represents the *port* through which one or more LUNs
// (VirtualScsiDevice instances) are exposed to initiators.
//
// Example (ControllerLib topology):
//    ┌------------------+
//    |  ScsiBus         |
//    └---┬--------┬-----┘
//        |        |
//   [TargetPort0][TargetPort1]
//        |
//   (LUN0, LUN1, ...)
//        |
//   VirtualScsiDevice
//
// The ScsiTargetPort is NOT the device itself — it is the port through which
// initiators access the device(s).
// ============================================================================

class ScsiTargetPort
{
public:
	// ------------------------------------------------------------------------
	// Constructors
	// ------------------------------------------------------------------------

	explicit ScsiTargetPort(ScsiBus* bus,
		const QString& name = "TARGET-PORT",
		quint64 wwn = 0,
		bool threadSafe = false) noexcept
		: m_bus(bus)
		, m_name(name)
		, m_wwn(wwn)
		, m_threadSafe(threadSafe)
		, m_commandsReceived(0)
		, m_bytesIn(0)
		, m_bytesOut(0)
	{
	}

	~ScsiTargetPort() noexcept = default;

	// ------------------------------------------------------------------------
	// Identity
	// ------------------------------------------------------------------------

	inline QString name() const noexcept { return m_name; }
	inline void setName(const QString& name) noexcept { m_name = name; }

	inline quint64 worldWideName() const noexcept { return m_wwn; }
	inline void setWorldWideName(quint64 wwn) noexcept { m_wwn = wwn; }

	// ------------------------------------------------------------------------
	// Bus association
	// ------------------------------------------------------------------------

	inline ScsiBus* bus() noexcept { return m_bus; }
	inline const ScsiBus* bus() const noexcept { return m_bus; }
	inline void setBus(ScsiBus* b) noexcept { m_bus = b; }

	// ------------------------------------------------------------------------
	// LUN registration
	// ------------------------------------------------------------------------
	//
	// These methods simply delegate to ScsiBus + ScsiController.
	//
	// Note: Target Port typically maps to SCSI targetId, while device-level
	//       mapping is (targetId, lun). Here we enforce a single targetId
	//       per ScsiTargetPort.
	//
	// You must call setTargetId() before using attachLun().
	// ------------------------------------------------------------------------

	inline void setTargetId(quint8 tid) noexcept { m_targetId = tid; }
	inline quint8 targetId() const noexcept { return m_targetId; }

	inline bool attachLun(ScsiLun lun, VirtualScsiDevice* dev) noexcept
	{
		if (!m_bus)
		{
			return false;
		}
		return m_bus->attachDevice(m_targetId, lun, dev);
	}

	inline bool detachLun(ScsiLun lun) noexcept
	{
		if (!m_bus)
		{
			return false;
		}
		return m_bus->detachDevice(m_targetId, lun);
	}

	inline bool hasLun(ScsiLun lun) const noexcept
	{
		return m_bus && m_bus->hasDevice(m_targetId, lun);
	}

	inline VirtualScsiDevice* lunDevice(ScsiLun lun) const noexcept
	{
		return (m_bus ? m_bus->device(m_targetId, lun) : nullptr);
	}

	// ------------------------------------------------------------------------
	// Command dispatch
	// ------------------------------------------------------------------------
	//
	// This wraps ScsiBus::execute() and applies target-port statistics.
	// ------------------------------------------------------------------------

	inline bool dispatch(ScsiLun      lun,
		ScsiCommand& cmd) noexcept
	{
		if (!m_bus)
		{
			cmd.setCheckCondition(scsiSense_InternalHardwareError());
			cmd.serviceResult = ScsiServiceResult::HostAdapterError;
			return false;
		}

		const bool ok = m_bus->execute(m_targetId, lun, cmd);

		// Stats update
		{
			QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

			++m_commandsReceived;
			const quint64 bytes = static_cast<quint64>(cmd.dataTransferred);

			switch (cmd.dataDirection)
			{
			case ScsiDataDirection::FromDevice:
				m_bytesOut += bytes;
				break;
			case ScsiDataDirection::ToDevice:
				m_bytesIn += bytes;
				break;
			case ScsiDataDirection::Bidirectional:
				m_bytesIn += bytes;
				m_bytesOut += bytes;
				break;
			case ScsiDataDirection::None:
			default:
				break;
			}
			m_lastCommandTime = QDateTime::currentDateTimeUtc();
		}

		return ok;
	}

	// ------------------------------------------------------------------------
	// Statistics access
	// ------------------------------------------------------------------------

	inline quint64 commandsReceived() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_commandsReceived;
	}

	inline quint64 bytesIn() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_bytesIn;
	}

	inline quint64 bytesOut() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_bytesOut;
	}

	inline QDateTime lastCommandTime() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_lastCommandTime;
	}

private:
	ScsiBus* m_bus;
	QString      m_name;
	quint64      m_wwn;
	quint8       m_targetId{ 0 };

	bool         m_threadSafe;
	mutable QMutex m_mutex;

	quint64      m_commandsReceived;
	quint64      m_bytesIn;
	quint64      m_bytesOut;
	QDateTime    m_lastCommandTime;
};

#endif // SCSI_TARGET_PORT_H
