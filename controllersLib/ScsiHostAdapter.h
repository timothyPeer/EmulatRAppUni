// ============================================================================
// ScsiHostAdapter.H  -  Abstract SCSI Host Adapter (controllerLib)
// ============================================================================
// This header defines a thin, abstract SCSI Host Adapter layer that sits on
// top of:
//
//   - ScsiBus           (logical SCSI bus / target + LUN space)
//   - ScsiController    (performs execute() against VirtualScsiDevice)
//   - ScsiInitiatorPort (identifies the host-side initiator endpoint)
//   - ScsiTargetPort    (identifies the target-side port)
//   - ScsiScheduler     (queues and dispatches ScsiTransaction objects)
//
// The ScsiHostAdapter class is intended as the base class for concrete
// emulated adapters such as:
//
//   - "GenericScsiHostAdapter"        (software-only)
//   - "NCR53C8xxHostAdapter"          (if you later emulate NCR chips)
//   - "QLogicHostAdapter"             (KZPBA style, etc.)
//
// This layer is *transport-neutral* and strictly controllerLib-only:
//
//   - No dependency on coreLib, AlphaCPU, PAL, MMIO, PTE, SafeMemory, etc.
//   - No reference to PCI configuration space or BARs.
//   - No direct guest memory mapping.
//
// Instead, derived classes should:
//   - Translate guest bus requests (from MMIO/PCI code elsewhere) into
//     ScsiTransaction instances.
//   - Use the provided synchronous and asynchronous helpers to dispatch
//     commands and observe the results.
//
// Design constraints:
//   - Header-only (no .CPP file).
//   - Depends only on QtCore + scsiCoreLib + controllerLib headers.
//   - Pure ASCII, UTF-8 (no BOM).
//
// SCSI reference model:
//   - SAM-2: SCSI Architecture Model
//     * Initiator Port <-> Target Port
//     * Logical Units (LUNs)
//     * Task and command model.
//
// ============================================================================

#ifndef SCSI_HOST_ADAPTER_H
#define SCSI_HOST_ADAPTER_H

#include <QtGlobal>
#include <QString>
#include <QVector>
#include <QMutex>

#include "ScsiCoreLib.H"
#include "ScsiBus.H"
#include "ScsiInitiatorPort.H"
#include "ScsiTargetPort.H"
#include "ScsiTransaction.H"
#include "ScsiScheduler.H"

// ============================================================================
// ScsiHostAdapter
// ============================================================================
//
// This abstract base class represents a generic SCSI host adapter within
// controllerLib. It aggregates:
//
//   - A ScsiBus    (device topology)
//   - A ScsiScheduler (transaction queue)
//   - A ScsiInitiatorPort (identity of the host side)
//
// It provides:
//   - A synchronous executeCommand() helper using ScsiController directly.
//   - An asynchronous submitTransaction() using ScsiScheduler.
//   - Hooks (virtual methods) for derived adapters to observe lifecycle
//     events (onQueued, onStarted, onCompleted).
//
// ============================================================================

class ScsiHostAdapter
{
public:
	// ------------------------------------------------------------------------
	// Constructor / Destructor
	// ------------------------------------------------------------------------

	ScsiHostAdapter(ScsiBus* bus,
		const QString& initName,
		quint64            initWwn,
		bool               threadSafe = false) noexcept
		: m_bus(bus)
		, m_initiator(initName, initWwn, threadSafe)
		, m_scheduler(threadSafe)
		, m_threadSafe(threadSafe)
	{
	}

	virtual ~ScsiHostAdapter() noexcept = default;

	// ------------------------------------------------------------------------
	// Accessors
	// ------------------------------------------------------------------------

	inline ScsiBus* bus() noexcept
	{
		return m_bus;
	}

	inline const ScsiBus* bus() const noexcept
	{
		return m_bus;
	}

	inline ScsiInitiatorPort& initiatorPort() noexcept
	{
		return m_initiator;
	}

	inline const ScsiInitiatorPort& initiatorPort() const noexcept
	{
		return m_initiator;
	}

	inline ScsiScheduler& scheduler() noexcept
	{
		return m_scheduler;
	}

	inline const ScsiScheduler& scheduler() const noexcept
	{
		return m_scheduler;
	}

	// ------------------------------------------------------------------------
	// Synchronous command execution
	// ------------------------------------------------------------------------
	//
	// This helper method allows derived host adapters to perform a single,
	// synchronous SCSI command and immediately receive the result. The
	// caller is responsible for:
	//
	//   - Initializing 'cmd' with:
	//       * cdb / cdbLength
	//       * dataBuffer / dataTransferLength
	//       * dataDirection
	//   - Providing a targetId and LUN on the bus.
	//
	// After return, the caller inspects:
	//   - cmd.status
	//   - cmd.serviceResult
	//   - cmd.dataTransferred
	//   - cmd.senseData (if status == CheckCondition)
	//
	inline bool executeCommand(quint8       targetId,
		ScsiLun      lun,
		ScsiCommand& cmd) noexcept
	{
		if (!m_bus)
		{
			cmd.serviceResult = ScsiServiceResult::HostAdapterError;
			cmd.setCheckCondition(scsiSense_InternalHardwareError());
			return false;
		}

		// Use the initiatorPort to execute and update statistics.
		return m_initiator.execute(m_bus->controller(), targetId, lun, cmd);
	}

	// ------------------------------------------------------------------------
	// Asynchronous-style transaction submission
	// ------------------------------------------------------------------------
	//
	// These helpers create and enqueue ScsiTransaction objects for later
	// scheduling via ScsiScheduler::runNext() or runAll().
	//
	// Ownership of ScsiTransaction:
	//   - Caller allocates the ScsiTransaction (e.g., on heap).
	//   - Scheduler stores only raw pointers.
	//   - Caller is responsible for deleting transactions after completion.
	//
	// ------------------------------------------------------------------------

	// Prepare a new transaction with a fresh transactionId.
	inline void prepareTransaction(ScsiTransaction& txn,
		ScsiTargetPort* target,
		quint8          targetId,
		ScsiLun         lun) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		txn.transactionId = m_scheduler.nextTransactionId();
		txn.initiator = &m_initiator;
		txn.target = target;
		txn.targetId = targetId;
		txn.lun = lun;
		txn.cmd.lun = lun;
		txn.completed = false;
		txn.success = false;
		txn.queuedTime = QDateTime();
		txn.startTime = QDateTime();
		txn.completionTime = QDateTime();
	}

	// Submit a pre-initialized transaction into the scheduler queue.
	inline void submitTransaction(ScsiTransaction* txn) noexcept
	{
		if (!txn)
		{
			return;
		}

		onTransactionQueued(*txn);
		m_scheduler.enqueue(txn);
	}

	// Optionally, host adapter can provide convenience wrappers for
	// creating, submitting, and running synchronous transactions from a CDB.
	//
	// Note: This still uses the scheduler, but the adapter waits for the
	// transaction to complete in the same thread by calling runNext().
	//
	inline bool submitAndRunOnce(ScsiTransaction* txn) noexcept
	{
		if (!txn)
		{
			return false;
		}

		submitTransaction(txn);
		const bool ok = m_scheduler.runNext();
		onTransactionCompleted(*txn);
		return ok;
	}

	// Higher layers can also just call scheduler().runAll() as needed for
	// batched submission. This class only provides a single-transaction
	// helper.

	// ------------------------------------------------------------------------
	// Abstract / virtual hooks for derived adapters
	// ------------------------------------------------------------------------
	//
	// Derived host adapters may override these hooks to perform logging,
	// tracing, or controller-specific operations at various points in the
	// transaction lifecycle.
	//
	// NOTE:
	//   These functions are intentionally lightweight; they must not
	//   reference any CPU-specific structures or MMIO state.
	// ------------------------------------------------------------------------

	// Called just before a transaction is enqueued into the scheduler.
	virtual void onTransactionQueued(ScsiTransaction& txn) noexcept
	{
		Q_UNUSED(txn);
	}

	// Called when a transaction has been started. Currently the scheduler
	// handles markStarted(), but this hook is for future expansion if you
	// choose to tie into more detailed statistics or tracing.
	virtual void onTransactionStarted(ScsiTransaction& txn) noexcept
	{
		Q_UNUSED(txn);
	}

	// Called after a transaction has completed (once). Host adapter derived
	// code may use this to update per-device or per-queue statistics or
	// raise completion events to higher layers.
	virtual void onTransactionCompleted(ScsiTransaction& txn) noexcept
	{
		Q_UNUSED(txn);
	}

protected:
	// Protected setter for the bus, in case a derived class needs to
	// swap out or reinitialize the bus topology at runtime.
	inline void setBus(ScsiBus* bus) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_bus = bus;
	}

private:
	ScsiBus* m_bus;
	ScsiInitiatorPort m_initiator;
	ScsiScheduler     m_scheduler;

	bool              m_threadSafe;
	mutable QMutex    m_mutex;
};

#endif // SCSI_HOST_ADAPTER_H
