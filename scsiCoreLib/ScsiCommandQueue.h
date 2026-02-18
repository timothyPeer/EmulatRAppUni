// ============================================================================
// ScsiCommandQueue.H  -  Lightweight SCSI Command Queue for scsiCoreLib
// ============================================================================
// This header provides a simple FIFO queue abstraction for ScsiCommand
// objects. It is intentionally minimal and suitable for:
//
//   - VirtualScsiDevice implementations that want per-LUN queues.
//   - SCSI controllers (in controllerLib) that need a queue but must not
//     introduce dependencies into scsiCoreLib.
//
// Features:
//   - FIFO insertion and retrieval
//   - Optional tagged-queue support (Simple / Ordered / Head-of-Queue)
//   - Thread-safe via optional QMutex (enabled by constructor flag)
//
// Design constraints:
//   - Header-only (no .CPP file).
//   - ASCII only, UTF-8, no BOM.
//   - Depends only on QtCore + SCSI core headers.
//   - No dependencies on coreLib, controllerLib, AlphaCPU, MMIO, PAL, PTE.
//
// References:
//   - SAM-2 Task Attributes (simple, ordered, head-of-queue).
//
// ============================================================================

#ifndef SCSI_COMMAND_QUEUE_H
#define SCSI_COMMAND_QUEUE_H

#include <QtGlobal>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>

#include "ScsiCommand.H"
#include "ScsiTypes.H"

// ============================================================================
// ScsiCommandQueue
// ============================================================================
//
// A simple Qt-based queue for ScsiCommand pointers. The queue does not take
// ownership of the commands; the caller must manage object lifetime.
//
// Thread safety:
//   - Optional. Enabled by passing 'true' to the constructor.
//   - When enabled, all operations lock a QMutex.
// ============================================================================

class ScsiCommandQueue
{
public:
	explicit ScsiCommandQueue(bool threadSafe = false) noexcept
		: m_threadSafe(threadSafe)
	{
	}

	~ScsiCommandQueue() noexcept = default;

	// ------------------------------------------------------------------------
	// Enqueue Command
	// ------------------------------------------------------------------------
	//
	// For SIMPLE tasks -> append to queue
	// For HEAD-OF-QUEUE tasks -> insert at front
	// For ORDERED tasks -> append, but semantics must be enforced by caller
	//
	inline void enqueue(ScsiCommand* cmd) noexcept
	{
		if (!cmd)
			return;

		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		switch (cmd->taskAttribute)
		{
		case ScsiTaskAttribute::HeadOfQueue:
			m_queue.prepend(cmd);
			break;

		case ScsiTaskAttribute::Ordered:
			// For ORDERED, we append but callers must enforce ordering rules.
			m_queue.append(cmd);
			break;

		case ScsiTaskAttribute::Simple:
		default:
			m_queue.append(cmd);
			break;
		}
	}

	// ------------------------------------------------------------------------
	// Dequeue Command (FIFO default)
	// ------------------------------------------------------------------------
	inline ScsiCommand* dequeue() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		if (m_queue.isEmpty())
			return nullptr;

		return m_queue.takeFirst();
	}

	// ------------------------------------------------------------------------
	// Peek the first command without removal
	// ------------------------------------------------------------------------
	inline ScsiCommand* peek() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);

		return m_queue.isEmpty() ? nullptr : m_queue.first();
	}

	// ------------------------------------------------------------------------
	// Clear all commands (does not delete them!)
	// ------------------------------------------------------------------------
	inline void clear() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_queue.clear();
	}

	// ------------------------------------------------------------------------
	// Count
	// ------------------------------------------------------------------------
	inline int count() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_queue.size();
	}

	// ------------------------------------------------------------------------
	// IsEmpty
	// ------------------------------------------------------------------------
	inline bool isEmpty() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_queue.isEmpty();
	}

private:
	QVector<ScsiCommand*> m_queue;
	bool                  m_threadSafe;
	mutable QMutex        m_mutex;
};

#endif // SCSI_COMMAND_QUEUE_H
