// ============================================================================
// ScsiTransaction.h - ============================================================================
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
// ScsiTransaction.H  -  SCSI I/O Transaction Descriptor (controllerLib)
// ============================================================================
// This header defines a neutral, controllerLib-side representation of a
// SCSI I/O transaction. It wraps a ScsiCommand together with metadata:
//
//   - Initiator port reference
//   - Target port reference
//   - Target ID / LUN
//   - Timestamps (queued, started, completed)
//   - Controller-visible transaction identifier
//   - Result flags (completed, success)
//
// Design constraints:
//   - Header-only, no .CPP file.
//   - Depends only on QtCore and scsiCoreLib + controllerLib headers.
//   - No dependency on coreLib, AlphaCPU, PAL, MMIO, PTE, SafeMemory, etc.
//   - Pure ASCII; UTF-8 (no BOM).
//
// References (conceptual):
//   - SAM-2 task / command set model.
//   - SPC-3 status and sense reporting via ScsiCommand.
// ============================================================================

#ifndef SCSI_TRANSACTION_H
#define SCSI_TRANSACTION_H

#include <QtGlobal>
#include <QDateTime>
#include <QString>

#include "ScsiCoreLib.H"
#include "ScsiInitiatorPort.H"
#include "ScsiTargetPort.H"

// ============================================================================
// ScsiTransaction
// ============================================================================
//
// This struct is designed to be created by a host adapter or scheduling layer
// when it receives a request from the guest. It can then be enqueued,
// scheduled, executed, and inspected after completion.
//
// Command semantics (CDB, data, status, sense) are all contained in the
// embedded ScsiCommand member.
// ============================================================================

struct ScsiTransaction
{
	// Simple controller-visible transaction identifier.
	// This can be assigned monotonically by the host adapter.
	quint64 transactionId;

	// Optional human-readable description or debug label.
	QString description;

	// References to the logical endpoints of this transaction.
	//
	// These are non-owning pointers; ScsiInitiatorPort and ScsiTargetPort
	// must outlive any transactions that reference them.
	//
	ScsiInitiatorPort* initiator;
	ScsiTargetPort* target;

	// Target addressing on the logical bus.
	quint8  targetId;
	ScsiLun lun;

	// The actual SCSI command and its parameters.
	//
	// The adapter fills in:
	//   - cmd.cdb
	//   - cmd.cdbLength
	//   - cmd.dataBuffer
	//   - cmd.dataTransferLength
	//   - cmd.dataDirection
	//
	// After execution, the target device updates:
	//   - cmd.status
	//   - cmd.serviceResult
	//   - cmd.dataTransferred
	//   - cmd.senseData (if CheckCondition)
	//
	ScsiCommand cmd;

	// Timestamps for observability.
	QDateTime queuedTime;
	QDateTime startTime;
	QDateTime completionTime;

	// Transaction state flags.
	bool      completed;
	bool      success;

	// ------------------------------------------------------------------------
	// Constructors
	// ------------------------------------------------------------------------

	ScsiTransaction() noexcept
		: transactionId(0)
		, description()
		, initiator(nullptr)
		, target(nullptr)
		, targetId(0)
		, lun(0)
		, cmd()
		, queuedTime()
		, startTime()
		, completionTime()
		, completed(false)
		, success(false)
	{
	}

	// Convenience constructor for common initialization.
	ScsiTransaction(quint64           id,
		ScsiInitiatorPort* initPort,
		ScsiTargetPort* tgtPort,
		quint8            tgtId,
		ScsiLun           lu) noexcept
		: transactionId(id)
		, description()
		, initiator(initPort)
		, target(tgtPort)
		, targetId(tgtId)
		, lun(lu)
		, cmd()
		, queuedTime(QDateTime::currentDateTimeUtc())
		, startTime()
		, completionTime()
		, completed(false)
		, success(false)
	{
		cmd.lun = lu;
	}

	// ------------------------------------------------------------------------
	// Lifecycle helpers
	// ------------------------------------------------------------------------

	// Mark that the transaction has been queued at the scheduler.
	inline void markQueued() noexcept
	{
		queuedTime = QDateTime::currentDateTimeUtc();
	}

	// Mark that execution has started.
	inline void markStarted() noexcept
	{
		startTime = QDateTime::currentDateTimeUtc();
	}

	// Mark that execution has completed. The caller should set 'success'
	// based on command status (for example, GOOD vs CHECK CONDITION).
	inline void markCompleted(bool ok) noexcept
	{
		completionTime = QDateTime::currentDateTimeUtc();
		completed = true;
		success = ok;
	}

	// Convenience helper: return true if execution has been completed.
	inline bool isCompleted() const noexcept
	{
		return completed;
	}

	// Convenience helper: return true if completed with GOOD status.
	inline bool isSuccessful() const noexcept
	{
		return completed && success &&
			cmd.status == ScsiStatus::Good;
	}

	// ------------------------------------------------------------------------
	// Duration helpers
	// ------------------------------------------------------------------------

	// Total time from queued to completed (in milliseconds).
	// Returns -1 if timestamps are insufficient.
	inline qint64 totalDurationMs() const noexcept
	{
		if (!queuedTime.isValid() || !completionTime.isValid())
			return -1;
		return queuedTime.msecsTo(completionTime);
	}

	// Service time (start to completion) in milliseconds.
	// Returns -1 if timestamps are insufficient.
	inline qint64 serviceDurationMs() const noexcept
	{
		if (!startTime.isValid() || !completionTime.isValid())
			return -1;
		return startTime.msecsTo(completionTime);
	}

	// Queue wait time (queued to start) in milliseconds.
	// Returns -1 if timestamps are insufficient.
	inline qint64 queueWaitMs() const noexcept
	{
		if (!queuedTime.isValid() || !startTime.isValid())
			return -1;
		return queuedTime.msecsTo(startTime);
	}
};

#endif // SCSI_TRANSACTION_H

