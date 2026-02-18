// ============================================================================
// ScsiController.H  -  Generic SCSI Initiator/Controller Interface (controllerLib)
// ============================================================================
// This header defines a lightweight, generic SCSI controller / initiator
// abstraction that sits above scsiCoreLib.
//
// Responsibilities:
//   - Maintain a mapping from (target ID, LUN) to VirtualScsiDevice*.
//   - Provide a synchronous "execute" path that:
//        * locates the correct target device
//        * passes a ScsiCommand to VirtualScsiDevice::handleCommand()
//        * returns success/failure at the controller level
//
// Design constraints:
//   - Header-only (no .CPP file).
//   - Depends only on QtCore and scsiCoreLib headers.
//   - Does NOT depend on coreLib (AlphaCPU, SafeMemory, MMIO, PAL, PTE, etc.).
//   - Pure ASCII; UTF-8 (no BOM).
//
// Higher layers (e.g., PCI SCSI host adapters, DMA engines, MMIO devices)
// are free to derive from ScsiController and:
//   - Translate guest bus transactions into ScsiCommand instances.
//   - Invoke execute() with appropriate targetId and LUN.
//   - Map completion back into guest-visible status.
//
// References (conceptual):
//   - SAM-2: SCSI Architecture Model (initiator / target / LUN).
//   - SPC-3: SCSI Primary Commands model.
//   - SBC-3, SSC-3, MMC-5: device-type-specific command behavior.
// ============================================================================

#ifndef SCSI_CONTROLLER_H
#define SCSI_CONTROLLER_H

#include <QtGlobal>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>

#include "../scsiCoreLib/SCSI_helpers_inl.h"   // ScsiCommand, ScsiLun, VirtualScsiDevice, etc.
#include "../scsiCoreLib/ScsiTypes.h"
#include "../scsiCoreLib/ScsiCommand.h"
#include "../scsiCoreLib/VirtualScsiDevice.h"

// ============================================================================
// ScsiController
// ============================================================================
//
// This class represents a logical SCSI controller / initiator that can talk
// to multiple targets (SCSI IDs) and LUNs. It is intentionally generic and
// does not embed any PCI, MMIO, or CPU-specific logic.
//
// Typical usage:
//   ScsiController ctrl(true);  // thread-safe
//   ctrl.attachDevice(0, ScsiLun(0), disk0);
//   ctrl.attachDevice(1, ScsiLun(0), cdrom0);
//
//   ScsiCommand cmd;
//   cmd.cdb      = cdbBytes;
//   cmd.cdbLength= 10;
//   cmd.lun      = ScsiLun(0);
//   // set dataBuffer, dataTransferLength, etc.
//
//   bool ok = ctrl.execute(0, ScsiLun(0), cmd);
//   // Afterwards, cmd.status / cmd.senseData carry guest-visible result.
//
// ============================================================================

class ScsiController
{
public:
	// Simple binding between a SCSI target ID, LUN, and a virtual device.
	struct TargetBinding
	{
		quint8             targetId;
		ScsiLun            lun;
		VirtualScsiDevice* device;

		TargetBinding() noexcept
			: targetId(0)
			, lun(0)
			, device(nullptr)
		{
		}

		TargetBinding(quint8 tid, ScsiLun l, VirtualScsiDevice* dev) noexcept
			: targetId(tid)
			, lun(l)
			, device(dev)
		{
		}
	};

	// Constructor
	//
	// Parameters:
	//   threadSafe - when true, a QMutex guards all attach/detach/execute
	//                operations. When false, caller must provide external
	//                synchronization if used from multiple threads.
	//
	explicit ScsiController(bool threadSafe = false) noexcept
		: m_threadSafe(threadSafe)
	{
	}

	virtual ~ScsiController() noexcept = default;

	// ------------------------------------------------------------------------
	// Device attachment
	// ------------------------------------------------------------------------
	//
	// Attach a VirtualScsiDevice instance to a (targetId, LUN) pair.
	//
	// Returns:
	//   true  - attached successfully
	//   false - a device is already attached at that address
	//
	inline bool attachDevice(quint8 targetId,
		ScsiLun lun,
		VirtualScsiDevice* device) noexcept
	{
		if (!device)
		{
			return false;
		}

		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		TargetBinding* existing = findBindingInternal(targetId, lun);
		if (existing && existing->device)
		{
			// Already bound.
			return false;
		}

		if (existing)
		{
			existing->device = device;
		}
		else
		{
			m_bindings.append(TargetBinding(targetId, lun, device));
		}

		return true;
	}

	// Detach any device bound to (targetId, LUN). Does not delete the device.
	//
	// Returns:
	//   true  - a device was detached
	//   false - no device was bound at that address
	//
	inline bool detachDevice(quint8 targetId, ScsiLun lun) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		for (int i = 0; i < m_bindings.size(); ++i)
		{
			if (m_bindings[i].targetId == targetId &&
				m_bindings[i].lun == lun)
			{
				m_bindings.removeAt(i);
				return true;
			}
		}
		return false;
	}

	// Look up a device bound at (targetId, LUN).
	//
	// Returns:
	//   VirtualScsiDevice* - if found
	//   nullptr            - if none bound
	//
	inline VirtualScsiDevice* deviceFor(quint8 targetId,
		ScsiLun lun) const noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		for (int i = 0; i < m_bindings.size(); ++i)
		{
			if (m_bindings[i].targetId == targetId &&
				m_bindings[i].lun == lun)
			{
				return m_bindings[i].device;
			}
		}
		return nullptr;
	}

	// ------------------------------------------------------------------------
	// Command execution
	// ------------------------------------------------------------------------
	//
	// Execute a command synchronously on the device bound at (targetId, LUN).
	//
	// Parameters:
	//   targetId - SCSI target ID (0..7 or 0..15, depending on bus).
	//   lun      - logical unit number (ScsiLun).
	//   cmd      - command object (CDB, buffer, and lengths must be set).
	//
	// Behavior:
	//   - Sets cmd.lun to 'lun' for the target device's benefit.
	//   - If no device is bound, sets:
	//       cmd.status        = CheckCondition
	//       cmd.serviceResult = TargetError
	//       cmd.senseData     = IllegalRequest / InvalidFieldInCdb
	//   - Calls beforeDispatch() hook.
	//   - Invokes device->handleCommand(cmd).
	//   - Calls afterDispatch() hook.
	//
	// Return:
	//   true  - dispatch path succeeded (deviceHandle called).
	//   false - no device bound, or device->handleCommand returned false.
	//
	inline bool execute(quint8 targetId,
		ScsiLun lun,
		ScsiCommand& cmd) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		VirtualScsiDevice* dev = deviceForUnlocked(targetId, lun);
		if (!dev)
		{
			// No such target/LUN; report as ILLEGAL REQUEST at the SCSI level.
			cmd.serviceResult = ScsiServiceResult::TargetError;
			cmd.setCheckCondition(scsiSense_InvalidFieldInCdb());
			return false;
		}

		// Fill in the LUN field in the command so the device can inspect it.
		cmd.lun = lun;

		beforeDispatch(targetId, lun, cmd);

		const bool ok = dev->handleCommand(cmd);

		afterDispatch(targetId, lun, cmd);

		return ok;
	}

	// ------------------------------------------------------------------------
	// Introspection helpers
	// ------------------------------------------------------------------------

	// Return true if a device is bound at the given (targetId, LUN).
	inline bool hasDevice(quint8 targetId, ScsiLun lun) const noexcept
	{
		return deviceFor(targetId, lun) != nullptr;
	}

	// Return the number of currently bound (targetId, LUN, device) tuples.
	inline int bindingCount() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		return m_bindings.size();
	}

protected:
	// Hooks for derived controllers.
	//
	// These are called with the controller lock already held (if threadSafe),
	// so they must not perform long-running or blocking operations if you
	// expect high concurrency.
	//
	// Derived classes can override these to implement logging, timing,
	// tracing, or controller-specific behavior (for example, DMA setup).
	//
	virtual void beforeDispatch(quint8 targetId,
		ScsiLun lun,
		ScsiCommand& cmd) noexcept
	{
		Q_UNUSED(targetId);
		Q_UNUSED(lun);
		Q_UNUSED(cmd);
	}

	virtual void afterDispatch(quint8 targetId,
		ScsiLun lun,
		ScsiCommand& cmd) noexcept
	{
		Q_UNUSED(targetId);
		Q_UNUSED(lun);
		Q_UNUSED(cmd);
	}

private:
	// Internal helper: find binding (mutable).
	inline TargetBinding* findBindingInternal(quint8 targetId,
		ScsiLun lun) noexcept
	{
		for (int i = 0; i < m_bindings.size(); ++i)
		{
			if (m_bindings[i].targetId == targetId &&
				m_bindings[i].lun == lun)
			{
				return &m_bindings[i];
			}
		}
		return nullptr;
	}

	// Internal helper: find device without taking a lock
	// (caller must already hold lock when threadSafe == true).
	inline VirtualScsiDevice* deviceForUnlocked(quint8 targetId,
		ScsiLun lun) const noexcept
	{
		for (int i = 0; i < m_bindings.size(); ++i)
		{
			if (m_bindings[i].targetId == targetId &&
				m_bindings[i].lun == lun)
			{
				return m_bindings[i].device;
			}
		}
		return nullptr;
	}

private:
	QVector<TargetBinding> m_bindings;
	bool                   m_threadSafe;
	mutable QMutex         m_mutex;
};

#endif // SCSI_CONTROLLER_H
