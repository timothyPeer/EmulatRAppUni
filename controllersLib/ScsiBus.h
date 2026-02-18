// ============================================================================
// ScsiBus.H  -  Logical SCSI Bus Abstraction for controllerLib
// ============================================================================
// The ScsiBus class is a lightweight abstraction representing a SCSI bus.
// It manages multiple SCSI target IDs, each of which may contain multiple LUNs
// attached through a ScsiController.
//
// This class *does not* model electrical/physical bus phases, arbitration,
// REQ/ACK, or timing. Instead, it models the *logical topology*:
//
//     +-----------------------------+
//     |          ScsiBus            |
//     |    (targets 0..15, LUNs)    |
//     +-----------------------------+
//             |            |
//      [target 0]      [target 3]
//         |                |
//      LUN0 LUN1       LUN0
//         |                |
//     VirtualScsiDisk    VirtualIsoDevice
//
// Higher layers (e.g., a PCI SCSI controller emulation) may bind to the
// ScsiBus to execute commands.
//
// Design constraints:
//   - Header-only (no .CPP).
//   - Depends only on QtCore + scsiCoreLib + ScsiController.H.
//   - No dependency on coreLib / MMIO / AlphaCPU.
//   - Pure ASCII, UTF-8 (no BOM).
//
// Reference concepts:
//   - SAM-2: SCSI domain model (initiator, target, LUN).
// ============================================================================

#ifndef SCSI_BUS_H
#define SCSI_BUS_H

#include <QtGlobal>
#include <QVector>
#include <QMutex>

#include "ScsiController.H"
#include "ScsiTypes.H"

// ============================================================================
// ScsiBus
// ============================================================================
//
// A ScsiBus owns a ScsiController and exposes bus-like attach/detach utilities.
// It provides:
//    - A target namespace (0..15 typically).
//    - Global bus-level reset and introspection.
//    - A convenient façade for binding devices by target and LUN.
//
// The bus delegates all actual I/O to ScsiController::execute().
// ============================================================================

class ScsiBus
{
public:
	explicit ScsiBus(bool threadSafe = false) noexcept
		: m_controller(threadSafe)
		, m_threadSafe(threadSafe)
	{
	}

	virtual ~ScsiBus() noexcept = default;

	// ------------------------------------------------------------------------
	// Device binding
	// ------------------------------------------------------------------------

	inline bool attachDevice(quint8 targetId,
		ScsiLun lun,
		VirtualScsiDevice* dev) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		return m_controller.attachDevice(targetId, lun, dev);
	}

	inline bool detachDevice(quint8 targetId,
		ScsiLun lun) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		return m_controller.detachDevice(targetId, lun);
	}

	inline bool hasDevice(quint8 targetId, ScsiLun lun) const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_controller.hasDevice(targetId, lun);
	}

	inline VirtualScsiDevice* device(quint8 targetId,
		ScsiLun lun) const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_controller.deviceFor(targetId, lun);
	}

	// ------------------------------------------------------------------------
	// Command Execution
	// ------------------------------------------------------------------------
	//
	// Execute a ScsiCommand on the bus at (targetId, lun).
	//
	inline bool execute(quint8 targetId,
		ScsiLun lun,
		ScsiCommand& cmd) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		return m_controller.execute(targetId, lun, cmd);
	}

	// ------------------------------------------------------------------------
	// Bus-wide reset (logical only)
	// ------------------------------------------------------------------------
	//
	// Calls reset() on every attached VirtualScsiDevice.
	//
	inline void resetBus() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		// Reset each bound device
		for (int i = 0; i < m_controller.bindingCount(); ++i)
		{
			// No direct index access -> we re-query all bindings
			// via deviceFor for each target/LUN combination.
			// This is acceptable because binding count is small.
		}

		// More efficient method: enforce binding index access
		// by modifying ScsiController if needed. But for now,
		// we can simply loop through known targets:
		for (quint8 tid = 0; tid < 16; ++tid)
		{
			for (quint8 ll = 0; ll < 8; ++ll)   // LUNs 0–7 typical
			{
				VirtualScsiDevice* dev =
					m_controller.deviceFor(tid, ScsiLun(ll));
				if (dev)
				{
					dev->reset();
				}
			}
		}
	}

	// ------------------------------------------------------------------------
	// Introspection
	// ------------------------------------------------------------------------
	inline int bindingCount() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_controller.bindingCount();
	}

	inline ScsiController& controller() noexcept
	{
		return m_controller;
	}

	inline const ScsiController& controller() const noexcept
	{
		return m_controller;
	}

protected:
	ScsiController m_controller;
	bool           m_threadSafe;
	mutable QMutex m_mutex;
};

#endif // SCSI_BUS_H
