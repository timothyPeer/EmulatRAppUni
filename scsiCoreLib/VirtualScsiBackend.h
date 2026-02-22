// ============================================================================
// VirtualScsiBackend.h - ============================================================================
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

#ifndef VIRTUAL_SCSI_BACKEND_H
#define VIRTUAL_SCSI_BACKEND_H
// ============================================================================
// VirtualScsiBackend.H  -  Abstract Storage Backend for scsiCoreLib
// ============================================================================
// This header defines a small, flexible backend interface that allows
// VirtualScsiDevice subclasses (disk, tape, ISO, etc.) to read/write data
// without tying them directly to QIODevice.
//
// Benefits:
//   - Multiple storage types (QFile, memory buffer, custom device).
//   - Future support for persistent disk caches, async reads, host I/O, etc.
//   - Keeps VirtualScsiDisk / VirtualTape / VirtualIso simpler.
//
// Design constraints:
//   - Header-only, no .CPP.
//   - Depends only on QtCore and scsiCoreLib headers.
//   - Absolutely NO dependency on coreLib, controllerLib, AlphaCPU,
//     register banks, PALcode, MMIO, PTE, or emulator state.
//   - Pure ASCII; UTF-8 (no BOM).
//
// Recommended implementation classes (later):
//   - FileBackend (QFile-based)
//   - MemoryBackend (RAM buffer)
//   - CachedBackend (QCache/QPersistentCache layer)
//   - PhysicalDiskBackend (OS-specific raw disk)
// ============================================================================



#include <QtGlobal>
#include <QByteArray>
#include <QIODevice>

// ============================================================================
// VirtualScsiBackend - abstract I/O provider
// ============================================================================
//
// This interface provides a basic set of operations for a storage backend.
// The methods mirror QIODevice's read/write API but are abstracted to allow
// additional layers (caching, journaling, RAID mapping, etc.).
//
// Clients:
//   - VirtualScsiDisk
//   - VirtualTapeDevice
//   - VirtualIsoDevice
//
// ============================================================================

class VirtualScsiBackend
{
public:
	VirtualScsiBackend() noexcept = default;
	virtual ~VirtualScsiBackend() noexcept = default;

	// ------------------------------------------------------------------------
	// Status: is backend open and usable?
	// ------------------------------------------------------------------------
	virtual bool isOpen() const noexcept = 0;

	// ------------------------------------------------------------------------
	// Size in bytes of the backing storage.
	//   For EOF/end-of-media: return the file/volume size.
	// ------------------------------------------------------------------------
	virtual qint64 size() const noexcept = 0;

	// ------------------------------------------------------------------------
	// Seek to absolute byte offset.
	// Returns:
	//   true  -> ok
	//   false -> failure (out-of-range or backend error)
	// ------------------------------------------------------------------------
	virtual bool seek(qint64 offset) noexcept = 0;

	// ------------------------------------------------------------------------
	// Read into buffer.
	// Returns number of bytes read, or:
	//   -1 on error
	//    0 on EOF
	// ------------------------------------------------------------------------
	virtual qint64 read(char* dest, qint64 maxBytes) noexcept = 0;

	// ------------------------------------------------------------------------
	// Write from buffer.
	// Returns:
	//   number of bytes written
	//   -1 on error
	//
	// Read-only devices should override to return -1.
	// ------------------------------------------------------------------------
	virtual qint64 write(const char* src, qint64 maxBytes) noexcept = 0;
};

// ============================================================================
// Default QIODevice-based backend
// ============================================================================
//
// This is a simple adapter that wraps a QIODevice to satisfy the backend
// interface. It is what VirtualScsiDisk / VirtualTape / VirtualIso typically
// use by default.
// ============================================================================

class QIODeviceBackend final : public VirtualScsiBackend
{
public:
	explicit QIODeviceBackend(QIODevice* dev, bool own = false) noexcept
		: m_dev(dev)
		, m_own(own)
	{
	}

	virtual ~QIODeviceBackend() noexcept override
	{
		if (m_own && m_dev)
		{
			m_dev->close();
			delete m_dev;
			m_dev = nullptr;
		}
	}

	virtual bool isOpen() const noexcept override
	{
		return m_dev && m_dev->isOpen();
	}

	virtual qint64 size() const noexcept override
	{
		return (m_dev ? m_dev->size() : -1);
	}

	virtual bool seek(qint64 offset) noexcept override
	{
		return (m_dev ? m_dev->seek(offset) : false);
	}

	virtual qint64 read(char* dest, qint64 maxBytes) noexcept override
	{
		return (m_dev ? m_dev->read(dest, maxBytes) : -1);
	}

	virtual qint64 write(const char* src, qint64 maxBytes) noexcept override
	{
		return (m_dev ? m_dev->write(src, maxBytes) : -1);
	}

private:
	QIODevice* m_dev;
	bool       m_own;
};

#endif // VIRTUAL_SCSI_BACKEND_H
