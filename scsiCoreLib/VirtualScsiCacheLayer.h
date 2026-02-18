#ifndef VIRTUAL_SCSI_CACHE_LAYER_H
#define VIRTUAL_SCSI_CACHE_LAYER_H

// ============================================================================
// VirtualScsiCacheLayer.H  -  Block-Oriented Cache Backend for scsiCoreLib
// ============================================================================
// This header defines a block-based caching backend that wraps another
// VirtualScsiBackend instance and provides a simple in-memory cache
// for fixed-size blocks (for example, 512-byte disk sectors or 2048-byte
// CD-ROM sectors).
//
// The primary design goal is to accelerate frequently accessed logical
// blocks such as:
//   - LBN 0 (home block / disk label)
//   - Boot blocks
//   - Frequently read directory/index regions
//
// Design constraints:
//   - Header-only, no .CPP file required.
//   - Pure ASCII, UTF-8 (no BOM).
//   - Depends only on QtCore and scsiCoreLib headers.
//   - No dependency on coreLib, controllerLib, AlphaCPU, PAL, MMIO, PTE,
//     or any other CPU-related structures.
//
// Behavior notes:
//   - Cache granularity is one fixed-size block, defined by blockSize.
//   - Only *aligned* reads whose size is a multiple of blockSize are cached.
//   - Unaligned reads/writes are passed through to the underlying backend.
//   - Writes invalidate cached blocks that overlap the write range.
//   - Not thread-safe: external synchronization is required for SMP use.
//
// References:
//   - SBC-3 (SCSI Block Commands) for the concept of logical block addressing.
//   - SPC-3 / SAM-2 for general SCSI device models.
// ============================================================================

#include <QtGlobal>
#include <QCache>
#include <QByteArray>
#include <cstdint>

#include "VirtualScsiBackend.H"

// ============================================================================
// VirtualScsiCacheLayer
// ============================================================================
//
// Key concepts:
//   - Wraps any VirtualScsiBackend (e.g., QIODeviceBackend).
//   - Interprets the backend byte stream as a sequence of fixed-size blocks.
//   - Caches blocks in a QCache<quint64, QByteArray> where the key is the
//     block index (offsetBytes / blockSize).
//
// Limitations:
//   - Only caches aligned reads:
//       * offset % blockSize == 0
//       * requested size % blockSize == 0
//   - Unaligned reads are passed through without caching.
//   - Writes are passed through and cause invalidation of overlapping blocks.
//
// Typical usage:
//   VirtualScsiBackend* rawBackend = new QIODeviceBackend(file, true);
//   VirtualScsiBackend* cachedBackend =
//       new VirtualScsiCacheLayer(rawBackend, 512, 1024, true);
//
//   // Then bind cachedBackend into VirtualScsiDisk instead of rawBackend.
//
// ============================================================================

class VirtualScsiCacheLayer : public VirtualScsiBackend
{
public:
	// Constructor
	//
	// Parameters:
	//   backend       - underlying storage backend (must not be null).
	//   blockSize     - logical block size in bytes (e.g., 512 or 2048).
	//   maxBlocks     - maximum number of blocks to keep in cache
	//                   (each block costs 1 in QCache).
	//   takeOwnership - if true, this object deletes backend in destructor.
	//
	VirtualScsiCacheLayer(VirtualScsiBackend* backend,
		quint32            blockSize,
		int                maxBlocks,
		bool               takeOwnership) noexcept
		: m_backend(backend)
		, m_blockSize(blockSize)
		, m_ownBackend(takeOwnership)
		, m_currentOffset(0)
	{
		m_cache.setMaxCost(maxBlocks > 0 ? maxBlocks : 0);
	}

	virtual ~VirtualScsiCacheLayer() noexcept override
	{
		if (m_ownBackend && m_backend)
		{
			delete m_backend;
			m_backend = nullptr;
		}
	}

	// ------------------------------------------------------------------------
	// VirtualScsiBackend interface
	// ------------------------------------------------------------------------

	virtual bool isOpen() const noexcept override
	{
		return (m_backend && m_backend->isOpen());
	}

	virtual qint64 size() const noexcept override
	{
		return m_backend ? m_backend->size() : -1;
	}

	virtual bool seek(qint64 offset) noexcept override
	{
		m_currentOffset = offset;
		return m_backend ? m_backend->seek(offset) : false;
	}

	virtual qint64 read(char* dest, qint64 maxBytes) noexcept override
	{
		if (!m_backend || !dest || maxBytes <= 0)
		{
			return -1;
		}

		// If blockSize is not defined or invalid, just pass through.
		if (m_blockSize == 0)
		{
			const qint64 n = m_backend->read(dest, maxBytes);
			if (n > 0)
			{
				m_currentOffset += n;
			}
			return n;
		}

		// Cache only aligned, full-block reads.
		const bool alignedOffset = (m_currentOffset % static_cast<qint64>(m_blockSize)) == 0;
		const bool alignedSize = (maxBytes % static_cast<qint64>(m_blockSize)) == 0;

		if (!alignedOffset || !alignedSize)
		{
			// Pass-through without caching.
			const qint64 n = m_backend->read(dest, maxBytes);
			if (n > 0)
			{
				m_currentOffset += n;
			}
			return n;
		}

		// Compute block index and count.
		const quint64 firstBlock =
			static_cast<quint64>(m_currentOffset / static_cast<qint64>(m_blockSize));
		const quint32 blockCount =
			static_cast<quint32>(maxBytes / static_cast<qint64>(m_blockSize));

		qint64 bytesCopied = 0;

		// Read each block, using cache when possible.
		for (quint32 i = 0; i < blockCount; ++i)
		{
			const quint64 blockIndex = firstBlock + static_cast<quint64>(i);
			QByteArray* cachedBlock = m_cache.object(blockIndex);

			if (!cachedBlock)
			{
				// Not in cache; read from backend and insert.
				QByteArray* newBlock = new QByteArray();
				newBlock->resize(static_cast<int>(m_blockSize));
				const qint64 offsetBytes =
					static_cast<qint64>(blockIndex * static_cast<quint64>(m_blockSize));

				if (!m_backend->seek(offsetBytes))
				{
					delete newBlock;
					return (bytesCopied > 0) ? bytesCopied : -1;
				}

				const qint64 r =
					m_backend->read(newBlock->data(),
						static_cast<qint64>(m_blockSize));
				if (r != static_cast<qint64>(m_blockSize))
				{
					delete newBlock;
					return (bytesCopied > 0) ? bytesCopied : -1;
				}

				// Insert into cache with cost=1.
				m_cache.insert(blockIndex, newBlock, 1);
				cachedBlock = newBlock;
			}

			// Copy block to destination buffer.
			const qint64 blockOffsetBytes =
				static_cast<qint64>(i) * static_cast<qint64>(m_blockSize);
			std::memcpy(dest + blockOffsetBytes,
				cachedBlock->constData(),
				static_cast<size_t>(m_blockSize));

			bytesCopied += static_cast<qint64>(m_blockSize);
		}

		m_currentOffset += bytesCopied;
		return bytesCopied;
	}

	virtual qint64 write(const char* src, qint64 maxBytes) noexcept override
	{
		if (!m_backend || !src || maxBytes <= 0)
		{
			return -1;
		}

		// If blockSize is invalid, just pass through.
		if (m_blockSize == 0)
		{
			const qint64 n = m_backend->write(src, maxBytes);
			if (n > 0)
			{
				m_currentOffset += n;
			}
			return n;
		}

		// Invalidate any cache blocks that overlap this write.
		invalidateBlocksForWrite(m_currentOffset, maxBytes);

		// We do not attempt write-back caching; we simply forward the write.
		const qint64 n = m_backend->write(src, maxBytes);
		if (n > 0)
		{
			m_currentOffset += n;
		}
		return n;
	}

	// ------------------------------------------------------------------------
	// Cache configuration and diagnostics
	// ------------------------------------------------------------------------

	// Returns the configured block size in bytes.
	inline quint32 blockSize() const noexcept
	{
		return m_blockSize;
	}

	// Returns the current maximum number of cached blocks.
	inline int maxCachedBlocks() const noexcept
	{
		return m_cache.maxCost();
	}

	// Sets a new maximum number of cached blocks. Existing entries may be
	// evicted automatically if cost exceeds maxCost.
	inline void setMaxCachedBlocks(int maxBlocks) noexcept
	{
		m_cache.setMaxCost(maxBlocks > 0 ? maxBlocks : 0);
	}

	// Clears the entire cache.
	inline void clearCache() noexcept
	{
		m_cache.clear();
	}

private:
	// Invalidate cache entries overlapping a write region starting at
	// byte offset 'offset' and extending 'length' bytes.
	inline void invalidateBlocksForWrite(qint64 offset, qint64 length) noexcept
	{
		if (m_blockSize == 0 || length <= 0)
		{
			return;
		}

		const quint64 firstBlock =
			static_cast<quint64>(offset / static_cast<qint64>(m_blockSize));
		const quint64 lastBlock =
			static_cast<quint64>(
				(offset + length - 1) / static_cast<qint64>(m_blockSize));

		for (quint64 block = firstBlock; block <= lastBlock; ++block)
		{
			m_cache.remove(block);
		}
	}

private:
	VirtualScsiBackend* m_backend;
	quint32                    m_blockSize;
	bool                       m_ownBackend;
	qint64                     m_currentOffset;
	QCache<quint64, QByteArray> m_cache;
};

#endif // VIRTUAL_SCSI_CACHE_LAYER_H
