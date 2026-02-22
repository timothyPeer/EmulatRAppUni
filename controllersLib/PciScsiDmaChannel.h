// ============================================================================
// PciScsiDmaChannel.h - ============================================================================
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
// PciScsiDmaChannel.H  -  DMA Channel Wrapper for PCI SCSI Devices
// ============================================================================
// Purpose:
//   This header defines a generic DMA "channel" abstraction that wraps a
//   PciScsiDmaEngine instance and adds:
//
//     � Channel enable/disable control
//     � Channel state (Idle / Running / Paused / Stopped / Error)
//     � Optional channel identifier and name
//     � Simple helpers to start, pause, resume, stop, and reset a channel
//
//   Concrete PCI SCSI controllers (derived from PciScsiDevice) can embed one
//   or more PciScsiDmaChannel instances and tie them to device-specific
//   registers.
//
// Design constraints:
//   - Header-only, no .CPP file.
//   - Depends only on QtCore (QtGlobal, QString, QMutex) and
//     PciScsiDmaEngine.H.
//   - Pure ASCII, UTF-8 (no BOM).
//   - NO dependency on AlphaCPU, SafeMemory, MMIOManager, PAL, or PTE.
//
// Typical usage in a PCI SCSI controller:
//
//   class MyPciScsiDevice : public PciScsiDevice {
//       class MyDmaEngine : public PciScsiDmaEngine { ... };
//       MyDmaEngine         m_dmaEngine;
//       PciScsiDmaChannel   m_dmaChannel;
//
//       MyPciScsiDevice(...)
//         : PciScsiDevice(bus, "PCI-SCSI0", 0x...)
//         , m_dmaEngine()
//         , m_dmaChannel(0, "CHAN0", &m_dmaEngine, true)
//       {}
//
//       void guestWritesDmaControl(quint32 val) {
//           if (val & START_BIT) m_dmaChannel.start();
//           if (val & STOP_BIT)  m_dmaChannel.stop();
//       }
//   };
//
// ============================================================================

#ifndef PCI_SCSI_DMA_CHANNEL_H
#define PCI_SCSI_DMA_CHANNEL_H

#include <QtGlobal>
#include <QString>
#include <QMutex>

#include "PciScsiDmaEngine.H"

// ============================================================================
// PciScsiDmaChannelState
// ============================================================================
enum class PciScsiDmaChannelState : quint8
{
	Idle = 0,
	Running = 1,
	Paused = 2,
	Stopped = 3,
	Error = 4
};

// ============================================================================
// PciScsiDmaChannel
// ============================================================================
//
// Lightweight channel wrapper around a PciScsiDmaEngine. The channel does not
// own the engine; it simply coordinates state and provides control methods.
//
// Thread-safety:
//   - Optional QMutex guarding state when constructed with threadSafe=true.
//   - Actual memory I/O and DMA semantics remain in the PciScsiDmaEngine
//     implementation and the emulator's memory subsystem.
//
// ============================================================================
class PciScsiDmaChannel
{
public:
	// ------------------------------------------------------------------------
	// Constructor / Destructor
	// ------------------------------------------------------------------------
	//
	// Parameters:
	//   channelId  - numeric identifier (e.g., 0..N-1).
	//   name       - human-readable channel name (for debug/logging).
	//   engine     - non-owning pointer to a PciScsiDmaEngine instance.
	//   threadSafe - when true, operations on this channel are mutex-protected.
	//
	PciScsiDmaChannel(quint8             channelId,
		const QString& name,
		PciScsiDmaEngine* engine,
		bool               threadSafe = false) noexcept
		: m_channelId(channelId)
		, m_name(name)
		, m_engine(engine)
		, m_state(PciScsiDmaChannelState::Idle)
		, m_enabled(false)
		, m_threadSafe(threadSafe)
	{
	}

	~PciScsiDmaChannel() noexcept = default;

	// ------------------------------------------------------------------------
	// Identity accessors
	// ------------------------------------------------------------------------

	inline quint8 channelId() const noexcept
	{
		return m_channelId;
	}

	inline QString name() const noexcept
	{
		return m_name;
	}

	inline void setName(const QString& name) noexcept
	{
		m_name = name;
	}

	// ------------------------------------------------------------------------
	// Engine binding
	// ------------------------------------------------------------------------

	inline void setEngine(PciScsiDmaEngine* engine) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_engine = engine;
	}

	inline PciScsiDmaEngine* engine() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_engine;
	}

	// ------------------------------------------------------------------------
	// Channel state / enable
	// ------------------------------------------------------------------------

	inline void setEnabled(bool enable) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_enabled = enable;
	}

	inline bool isEnabled() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_enabled;
	}

	inline PciScsiDmaChannelState state() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_state;
	}

	// ------------------------------------------------------------------------
	// Control operations
	// ------------------------------------------------------------------------
	//
	// These helpers change the channel state and coordinate with the engine.
	// The actual DMA transfer work is performed inside the engine�s
	// performDmaTransfer() implementation, not here.
	//
	// ------------------------------------------------------------------------

	// Start DMA on this channel if enabled and engine is present.
	inline void start() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		if (!m_enabled || !m_engine)
		{
			m_state = PciScsiDmaChannelState::Error;
			return;
		}

		m_engine->start();
		m_state = PciScsiDmaChannelState::Running;
	}

	// Stop DMA and mark channel as Stopped. Does not clear descriptors.
	inline void stop() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		if (!m_engine)
		{
			m_state = PciScsiDmaChannelState::Error;
			return;
		}

		// The engine is responsible for responding to stop semantics as needed.
		// Here we just mark the channel state.
		m_state = PciScsiDmaChannelState::Stopped;
	}

	// Pause DMA (logical only; engine implementation may interpret this).
	inline void pause() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		if (!m_engine || m_state != PciScsiDmaChannelState::Running)
		{
			return;
		}

		m_state = PciScsiDmaChannelState::Paused;
	}

	// Resume DMA if previously paused.
	inline void resume() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		if (!m_engine || m_state != PciScsiDmaChannelState::Paused)
		{
			return;
		}

		m_state = PciScsiDmaChannelState::Running;
	}

	// Reset DMA channel: clear engine descriptors and return to Idle state.
	inline void reset() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		if (m_engine)
		{
			m_engine->clearDescriptors();
		}
		m_state = PciScsiDmaChannelState::Idle;
	}

	// Mark channel as having encountered an error.
	inline void markError() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_state = PciScsiDmaChannelState::Error;
	}

private:
	quint8             m_channelId;
	QString            m_name;
	PciScsiDmaEngine* m_engine;

	PciScsiDmaChannelState m_state;
	bool                   m_enabled;

	bool                   m_threadSafe;
	mutable QMutex         m_mutex;
};

#endif // PCI_SCSI_DMA_CHANNEL_H
