// ============================================================================
// ScsiScheduler.h - ============================================================================
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

// ============================================================================
// ScsiScheduler.H  -  Lightweight SCSI Transaction Scheduler (controllerLib)
// ============================================================================
// This header defines a simple, synchronous, single-queue SCSI scheduler.
// It manages ScsiTransaction objects, dispatches them to ScsiTargetPort,
// tracks state, timestamps, and ensures ordered completion.
//
// Features:
//   - FIFO queue of ScsiTransaction*
//   - enqueue(), dequeue(), runNext(), runAll()
//   - No threading inside scheduler (caller may wrap externally)
//   - Observability via transaction timestamps
//
// Design constraints:
//   - Header-only, no .CPP file.
//   - Depends only on QtCore and controllerLib/scsiCoreLib headers.
//   - ABSOLUTELY no dependency on coreLib, MMIO, PAL, PTE, CPU state.
//   - Pure ASCII; UTF-8 (no BOM).
//
// References:
//   - SAM-2: Queueing and task dispatch model (informal alignment)
// ============================================================================

#ifndef SCSI_SCHEDULER_H
#define SCSI_SCHEDULER_H

#include <QtGlobal>
#include <QVector>
#include <QMutex>

#include "ScsiTransaction.H"
#include "ScsiTargetPort.H"

// ============================================================================
// ScsiScheduler
// ============================================================================
//
// This is intentionally simple - it does not model iSCSI/FC multi-queue,
// tagged ordered/deferred queuing, head-of-queue tasks, nor starvation
// prevention. It is meant to serve as a correct, minimal building block
// for emulator-level SCSI controller implementations.
//
// Typical usage:
//     ScsiScheduler scheduler(true);
//     scheduler.enqueue(txn);
//     scheduler.runAll();    // or runNext()
//
// ============================================================================

class ScsiScheduler
{
public:
	explicit ScsiScheduler(bool threadSafe = false) noexcept
		: m_threadSafe(threadSafe)
		, m_nextTransactionId(1)
	{
	}

	~ScsiScheduler() noexcept = default;

	// ------------------------------------------------------------------------
	// Transaction creation helper
	// ------------------------------------------------------------------------
	//
	// Automatically assigns a transaction ID and queues the transaction.
	// Caller must fill in txn.cmd fields before calling enqueue().
	//
	inline quint64 nextTransactionId() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		return m_nextTransactionId++;
	}

	// ------------------------------------------------------------------------
	// Queue operations
	// ------------------------------------------------------------------------

	inline void enqueue(ScsiTransaction* txn) noexcept
	{
		if (!txn)
			return;

		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		txn->markQueued();
		m_queue.append(txn);
	}

	inline ScsiTransaction* dequeue() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		if (m_queue.isEmpty())
			return nullptr;

		return m_queue.takeFirst();
	}

	inline bool isEmpty() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_queue.isEmpty();
	}

	inline int count() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_queue.size();
	}

	// ------------------------------------------------------------------------
	// Dispatch operations
	// ------------------------------------------------------------------------
	//
	// runNext():
	//   - Dequeues the next transaction
	//   - Sends it through its ScsiTargetPort
	//   - Updates timestamps & success flags
	//
	// runAll():
	//   - Continuously runs until queue exhausted
	//
	// Caller must ensure targetPort pointer inside transaction is non-null.
	//
	// ------------------------------------------------------------------------

	inline bool runNext() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		if (m_queue.isEmpty())
			return false;

		ScsiTransaction* txn = m_queue.takeFirst();
		if (!txn || !txn->target)
		{
			if (txn)
			{
				txn->cmd.setCheckCondition(scsiSense_InternalHardwareError());
				txn->markCompleted(false);
			}
			return false;
		}

		// Dispatch outside the lock to avoid deadlocks if target uses locks.
		ScsiTargetPort* tgt = txn->target;
		quint8          tid = txn->targetId;
		ScsiLun         lun = txn->lun;

		locker.unlock();   // release internal scheduler lock
		txn->markStarted();

		bool ok = tgt->dispatch(lun, txn->cmd);

		txn->markCompleted(ok);

		return ok;
	}

	inline void runAll() noexcept
	{
		while (runNext()) { /* keep going */ }
	}

	// ------------------------------------------------------------------------
	// Clear queue (does NOT delete transactions)
// ------------------------------------------------------------------------
	inline void clear() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_queue.clear();
	}

private:
	bool              m_threadSafe;
	mutable QMutex    m_mutex;
	QVector<ScsiTransaction*> m_queue;

	quint64           m_nextTransactionId;
};

#endif // SCSI_SCHEDULER_H
