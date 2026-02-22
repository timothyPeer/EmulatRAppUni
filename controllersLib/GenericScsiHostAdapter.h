// ============================================================================
// GenericScsiHostAdapter.h - ============================================================================
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef GENERIC_SCSI_HOST_ADAPTER_H
#define GENERIC_SCSI_HOST_ADAPTER_H
// ============================================================================
// GenericScsiHostAdapter.H  -  QThread-based SCSI Host Adapter (controllerLib)
// ============================================================================
// This header defines a concrete, QThread-enabled SCSI host adapter that
// builds on top of:
//
//   - ScsiHostAdapter          (logical host adapter base)
//   - ScsiBus / ScsiController (target + LUN topology)
//   - ScsiInitiatorPort        (host-side identity)
//   - ScsiTargetPort           (target-side identity)
//   - ScsiTransaction / ScsiScheduler
//   - ScsiHostAdapterBackend   (platform glue / completion callback)
//
// Key features:
//   - Synchronous path via executeCommand() inherited from ScsiHostAdapter.
//   - Asynchronous path using a dedicated QThread that:
//       * Waits for pending ScsiTransaction objects,
//       * Processes each using ScsiHostAdapter::submitAndRunOnce(),
//       * Propagates completion to ScsiHostAdapterBackend.
//
// Design constraints:
//   - Header-only (no .CPP file).
//   - Depends only on QtCore + scsiCoreLib + controllerLib headers.
//   - NO dependency on coreLib, AlphaCPU, MMIO, PAL, PTE, SafeMemory, etc.
//   - Pure ASCII, UTF-8 (no BOM).
//
// QThread pipeline (high-level):
//   1. External code (e.g., PCI/MMIO layer) creates a ScsiTransaction.
//   2. External code calls GenericScsiHostAdapter::submitAsync(txn).
//   3. Worker thread wakes up, pulls txn from the pending queue.
//   4. Worker calls prepareTransaction() + submitAndRunOnce(txn).
//   5. submitAndRunOnce() uses ScsiScheduler and then calls
//      onTransactionCompleted(), which GenericScsiHostAdapter overrides.
//   6. onTransactionCompleted() calls ScsiHostAdapterBackend::onTransactionComplete().
//
// ============================================================================



#include <QtGlobal>
#include <QThread>
#include <QWaitCondition>
#include <QAtomicInt>
#include "ScsiBus.H"
#include "ScsiHostAdapter.H"
#include "ScsiHostAdapterBackend.H"
#include "ScsiTransaction.H"

// ============================================================================
// GenericScsiHostAdapter
// ============================================================================
//
// This class provides a ready-to-use host adapter that supports both
// synchronous and asynchronous (QThread-based) I/O pipelines.
//
// Usage pattern (async):
//   GenericScsiHostAdapter adapter(&bus, "INIT0", 0x1122334455667788ULL, true);
//   adapter.setBackend(&backend);
//   adapter.startIoThread();
//
//   ScsiTransaction* txn = new ScsiTransaction();
//   // Fill txn->target, txn->targetId, txn->lun, txn->cmd.cdb, etc.
//   adapter.submitAsync(txn);
//   // Later, backend->onTransactionComplete(txn) will be invoked.
//
// ============================================================================

class GenericScsiHostAdapter : public ScsiHostAdapter
{
public:
	// ------------------------------------------------------------------------
	// Worker thread class
	// ------------------------------------------------------------------------
	//
	// Internal QThread that waits for pending transactions and processes them
	// in the background using submitAndRunOnce().
	//
	class IoWorkerThread : public QThread
	{
	public:
		explicit IoWorkerThread(GenericScsiHostAdapter* adapter) noexcept
			: QThread()
			, m_adapter(adapter)
			, m_stopFlag(0)
		{
		}

		// Request the thread to stop as soon as practical.
		inline void requestStop() noexcept
		{
			m_stopFlag.storeRelease(1);
		}

	protected:
		void run() override
		{
			if (!m_adapter)
			{
				return;
			}

			for (;;)
			{
				if (m_stopFlag.loadAcquire() != 0)
				{
					break;
				}

				ScsiTransaction* txn = nullptr;

				{
					// Lock the pending queue and wait for work.
					QMutexLocker locker(&m_adapter->m_queueMutex);

					if (m_adapter->m_pendingTransactions.isEmpty())
					{
						// Wait for up to 50 ms for a new transaction or stop.
						m_adapter->m_queueCond.wait(&m_adapter->m_queueMutex, 50);
					}

					if (!m_adapter->m_pendingTransactions.isEmpty())
					{
						txn = m_adapter->m_pendingTransactions.takeFirst();
					}
				}

				if (!txn)
				{
					// No work this iteration; re-check stop flag and loop.
					continue;
				}

				// Process this transaction in the worker thread context.
				m_adapter->processTransactionInWorker(txn);
			}
		}

	private:
		GenericScsiHostAdapter* m_adapter;
		QAtomicInt              m_stopFlag;
	};

	// ------------------------------------------------------------------------
	// Constructor / Destructor
	// ------------------------------------------------------------------------

	GenericScsiHostAdapter(ScsiBus* bus,
		const QString& initiatorName,
		quint64        initiatorWwn,
		bool           threadSafe = false) noexcept
		: ScsiHostAdapter(bus, initiatorName, initiatorWwn, threadSafe)
		, m_backend(nullptr)
		, m_ioThread(this)
	{
	}

	virtual ~GenericScsiHostAdapter() noexcept override
	{
		stopIoThread();
	}

	// ------------------------------------------------------------------------
	// Backend binding
	// ------------------------------------------------------------------------

	inline void setBackend(ScsiHostAdapterBackend* backend) noexcept
	{
		QMutexLocker locker(&m_adapterMutex);
		m_backend = backend;
	}

	inline ScsiHostAdapterBackend* backend() const noexcept
	{
		QMutexLocker locker(&m_adapterMutex);
		return m_backend;
	}

	// ------------------------------------------------------------------------
	// Asynchronous I/O control (QThread-based)
	// ------------------------------------------------------------------------

	// Start the I/O worker thread if not already running.
	inline void startIoThread() noexcept
	{
		QMutexLocker locker(&m_adapterMutex);
		if (!m_ioThread.isRunning())
		{
			// Ensure pending queue is in a consistent state.
			m_ioThread.start();
		}
	}

	// Request the I/O worker thread to stop and wait for it to finish.
	inline void stopIoThread() noexcept
	{
		QMutexLocker locker(&m_adapterMutex);
		if (m_ioThread.isRunning())
		{
			m_ioThread.requestStop();
			// Wake the worker in case it's waiting on the condition variable.
			m_queueCond.wakeAll();
			locker.unlock();  // avoid deadlock while waiting
			m_ioThread.wait();
		}
	}

	inline bool isIoThreadRunning() const noexcept
	{
		return m_ioThread.isRunning();
	}

	// ------------------------------------------------------------------------
	// Asynchronous submission
	// ------------------------------------------------------------------------
	//
	// Public API for queueing a transaction into the worker thread.
	//
	// Ownership:
	//   - Caller allocates ScsiTransaction (commonly on heap).
	//   - GenericScsiHostAdapter does NOT delete the transaction.
	//   - The backend (or higher-level code) is expected to delete it
	//     after completion has been observed.
	//
	inline void submitAsync(ScsiTransaction* txn) noexcept
	{
		if (!txn)
		{
			return;
		}

		{
			QMutexLocker locker(&m_queueMutex);
			m_pendingTransactions.append(txn);
		}

		// Wake the worker thread so it can process this transaction.
		m_queueCond.wakeOne();
	}

	// ------------------------------------------------------------------------
	// Synchronous helper (inherits executeCommand from ScsiHostAdapter)
	// ------------------------------------------------------------------------
	//
	// For blocking, synchronous I/O use executeCommand():
	//
	//   ScsiCommand cmd;
	//   // fill in CDB, buffers, etc.
	//   adapter.executeCommand(targetId, lun, cmd);
	//
	// That call bypasses the worker thread and uses the initiatorPort directly.
	//
	// ------------------------------------------------------------------------

	// ------------------------------------------------------------------------
	// Hook overrides from ScsiHostAdapter
	// ------------------------------------------------------------------------

	// We currently do not need to override onTransactionQueued or
	// onTransactionStarted; they can be extended later for logging/tracing.
	virtual void onTransactionQueued(ScsiTransaction& txn) noexcept override
	{
		Q_UNUSED(txn);
	}

	virtual void onTransactionStarted(ScsiTransaction& txn) noexcept override
	{
		Q_UNUSED(txn);
	}

	// When a transaction completes (whether synchronous or via worker),
	// forward completion to the backend if one is bound.
	virtual void onTransactionCompleted(ScsiTransaction& txn) noexcept override
	{
		ScsiHostAdapterBackend* b = nullptr;
		{
			QMutexLocker locker(&m_adapterMutex);
			b = m_backend;
		}
		if (b)
		{
			b->onTransactionComplete(txn);
		}
	}

protected:
	// ------------------------------------------------------------------------
	// Worker-side transaction processing
	// ------------------------------------------------------------------------
	//
	// Called by IoWorkerThread::run() for each pending transaction.
	// This method:
	//   - Prepares the transaction (assigns transactionId, timestamps).
	//   - Uses submitAndRunOnce() to:
	//        * enqueue into ScsiScheduler
	//        * dispatch to the correct ScsiTargetPort
	//        * call onTransactionCompleted() when done
	//
	inline void processTransactionInWorker(ScsiTransaction* txn) noexcept
	{
		if (!txn)
		{
			return;
		}

		// Ensure txn->target and addressing are set by caller.
		if (!txn->target)
		{
			txn->cmd.setCheckCondition(scsiSense_InternalHardwareError());
			txn->markCompleted(false);
			onTransactionCompleted(*txn);
			return;
		}

		// Prepare the transaction with a new ID and link to initiator.
		prepareTransaction(*txn, txn->target, txn->targetId, txn->lun);

		// submitAndRunOnce():
		//   - Enqueues txn into ScsiScheduler
		//   - Runs exactly one transaction from the scheduler
		//   - Calls onTransactionCompleted(*txn) afterward
		submitAndRunOnce(txn);
	}

private:
	// Backend pointer and adapter-level mutex (distinct from queue mutex).
	ScsiHostAdapterBackend* m_backend;
	mutable QMutex          m_adapterMutex;

	// Asynchronous worker thread and its queue.
	IoWorkerThread          m_ioThread;
	mutable QMutex          m_queueMutex;
	QWaitCondition          m_queueCond;
	QVector<ScsiTransaction*> m_pendingTransactions;
};

#endif // GENERIC_SCSI_HOST_ADAPTER_H

