// ============================================================================
// ScsiInitatorPort.h - ============================================================================
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
// ScsiInitiatorPort.H  -  SCSI Initiator Port Abstraction (controllerLib)
// ============================================================================
// This header defines a lightweight model of a SCSI Initiator Port, which
// represents the "host side" endpoint that issues SCSI commands to targets
// on a ScsiBus via a ScsiController.
//
// It provides:
//   - Identity (port name, world-wide name style identifier).
//   - Basic statistics (commands issued, bytes read/written).
//   - A convenience execute() wrapper around ScsiController::execute() that
//     updates statistics and timestamps.
//
// Design constraints:
//   - Header-only, no .CPP file.
//   - Depends only on QtCore + scsiCoreLib + ScsiController.H.
//   - No dependency on coreLib, AlphaCPU, PAL, MMIO, PTE, SafeMemory, etc.
//   - Pure ASCII; UTF-8 (no BOM).
//
// Conceptual references:
//   - SAM-2: SCSI Architecture Model (initiator port, target port).
//   - SPC-3: Command and task model as seen from the initiator side.
// ============================================================================

#ifndef SCSI_INITIATOR_PORT_H
#define SCSI_INITIATOR_PORT_H

#include <QtGlobal>
#include <QString>
#include <QDateTime>
#include <QMutex>

#include "ScsiCoreLib.H"
#include "ScsiController.H"

// ============================================================================
// ScsiInitiatorPort
// ============================================================================
//
// In the SAM-2 model, an Initiator Port issues SCSI tasks to Target Ports.
// Here, ScsiInitiatorPort is a simple logical representation that:
//
//   - Has a human-readable name.
//   - Has a 64-bit "world-wide" style identifier.
//   - Tracks how many commands and bytes have been sent/received.
//   - Provides a convenience execute() wrapper that uses a ScsiController.
//
// This class does not manage any transport-level details (no PCI, no SRP,
// no FC/iSCSI); it is strictly an in-memory logical object.
// ============================================================================

class ScsiInitiatorPort
{
public:
	// ------------------------------------------------------------------------
	// Constructors
	// ------------------------------------------------------------------------

	// Default constructor: anonymous initiator, ID = 0.
	explicit ScsiInitiatorPort(bool threadSafe = false) noexcept
		: m_name("INIT-PORT")
		, m_wwn(0)
		, m_threadSafe(threadSafe)
		, m_commandsIssued(0)
		, m_bytesRead(0)
		, m_bytesWritten(0)
	{
	}

	// Constructor with name and 64-bit identifier.
	//
	// The wwn value is conceptually similar to a World Wide Name or Port Name
	// in Fibre Channel or iSCSI, but we keep it generic: the emulator can map
	// this to any external identity it wishes (e.g., configuration JSON).
	//
	ScsiInitiatorPort(const QString& name,
		quint64        wwn,
		bool           threadSafe = false) noexcept
		: m_name(name)
		, m_wwn(wwn)
		, m_threadSafe(threadSafe)
		, m_commandsIssued(0)
		, m_bytesRead(0)
		, m_bytesWritten(0)
	{
	}

	~ScsiInitiatorPort() noexcept = default;

	// ------------------------------------------------------------------------
	// Identity accessors
	// ------------------------------------------------------------------------

	inline QString name() const noexcept
	{
		return m_name;
	}

	inline void setName(const QString& name) noexcept
	{
		m_name = name;
	}

	inline quint64 worldWideName() const noexcept
	{
		return m_wwn;
	}

	inline void setWorldWideName(quint64 wwn) noexcept
	{
		m_wwn = wwn;
	}

	// ------------------------------------------------------------------------
	// Statistics
	// ------------------------------------------------------------------------

	inline quint64 commandsIssued() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_commandsIssued;
	}

	inline quint64 bytesRead() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_bytesRead;
	}

	inline quint64 bytesWritten() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_bytesWritten;
	}

	inline QDateTime lastCommandTime() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_lastCommandTime;
	}

	// ------------------------------------------------------------------------
	// Command execution wrapper
	// ------------------------------------------------------------------------
	//
	// This convenience method wraps ScsiController::execute() and records:
	//   - commandsIssued
	//   - bytesRead / bytesWritten (based on cmd.dataDirection)
	//   - lastCommandTime
	//
	// The controller and underlying VirtualScsiDevice still perform all
	// SCSI semantics; ScsiInitiatorPort only adds accounting and an identity
	// context that higher layers (logging, tracing) can use.
	//
	inline bool execute(ScsiController& controller,
		quint8          targetId,
		ScsiLun         lun,
		ScsiCommand& cmd) noexcept
	{
		const bool ok = controller.execute(targetId, lun, cmd);

		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		++m_commandsIssued;
		m_lastCommandTime = QDateTime::currentDateTimeUtc();

		// Update byte counters based on command direction and transfer count.
		//
		// Note: cmd.dataTransferred is in bytes per our scsiCoreLib design.
		//
		if (cmd.hasDataPhase())
		{
			const quint64 bytes = static_cast<quint64>(cmd.dataTransferred);
			switch (cmd.dataDirection)
			{
			case ScsiDataDirection::FromDevice:
				m_bytesRead += bytes;
				break;
			case ScsiDataDirection::ToDevice:
				m_bytesWritten += bytes;
				break;
			case ScsiDataDirection::Bidirectional:
				// For simplicity, treat bidirectional as both.
				m_bytesRead += bytes;
				m_bytesWritten += bytes;
				break;
			case ScsiDataDirection::None:
			default:
				// No data phase; nothing to count.
				break;
			}
		}

		return ok;
	}

private:
	QString   m_name;
	quint64   m_wwn;

	bool      m_threadSafe;
	mutable QMutex m_mutex;

	quint64   m_commandsIssued;
	quint64   m_bytesRead;
	quint64   m_bytesWritten;
	QDateTime m_lastCommandTime;
};

#endif // SCSI_INITIATOR_PORT_H
