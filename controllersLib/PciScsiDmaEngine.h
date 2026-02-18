// ============================================================================
// PciScsiDmaEngine.H  -  Abstract DMA Descriptor Engine for PCI SCSI Devices
// ============================================================================
// Purpose:
//   PCI SCSI controllers typically implement DMA-based data movement between
//   guest memory and the device's internal buffers. This header defines a
//   generic, device-agnostic DMA descriptor engine that a PCI SCSI controller
//   can use to represent:
//
//     • Scatter/gather entries
//     • Transfer direction (read/write)
//     • Current DMA position
//     • Descriptor fetch completion
//     • DMA start/stop/reset semantics
//
//   *Actual memory transfers are NOT performed here.*
//   They will be bound to your emulator's SafeMemory/MMIOManager later.
//
// Design constraints:
//   - Header-only, no .CPP.
//   - Pure ASCII, UTF-8 (no BOM).
//   - Depends only on QtCore types (QtGlobal, QVector).
//   - NO dependency on coreLib, AlphaCPU, SafeMemory, or MMIOManager.
//   - Intended for use inside derived classes of PciScsiDevice.
//
// ============================================================================

#ifndef PCI_SCSI_DMA_ENGINE_H
#define PCI_SCSI_DMA_ENGINE_H

#include <QtGlobal>
#include <QVector>
#include <QString>

// ============================================================================
// PciScsiDmaDirection
// ============================================================================
enum class PciScsiDmaDirection : quint8
{
	None = 0,
	ReadFromHost,     // guest-memory -> controller
	WriteToHost       // controller -> guest-memory
};

// ============================================================================
// PciScsiDmaDescriptor
// ============================================================================
// A single scatter/gather DMA descriptor (guest-visible).
// ============================================================================
struct PciScsiDmaDescriptor
{
	quint64  guestAddress;   // guest physical (or bus) address
	quint32  length;         // bytes to transfer
	bool     last;           // true if this is the final element of a chain

	PciScsiDmaDescriptor() noexcept
		: guestAddress(0)
		, length(0)
		, last(false)
	{
	}
};

// ============================================================================
// PciScsiDmaState
// ============================================================================
enum class PciScsiDmaState : quint8
{
	Idle = 0,
	Active = 1,
	Completed = 2,
	Error = 3
};

// ============================================================================
// PciScsiDmaEngine
// ============================================================================
//
// This class stores DMA descriptors + state, but does NOT perform real memory
// operations. DMA execution is modeled by calling:
//
//     onDmaStart()
//     performDmaTransfer()     <-- implemented by PCI/MMIO layer later
//     onDmaComplete()
//
// Derived PCI SCSI controllers can:
//
//   • Load DMA descriptors from MMIO registers
//   • Bind actual memory copy calls via their emulator's memory subsystem
//   • Update SCSI transactions based on DMA results
//
// ============================================================================
class PciScsiDmaEngine
{
public:
	PciScsiDmaEngine() noexcept
		: m_state(PciScsiDmaState::Idle)
		, m_direction(PciScsiDmaDirection::None)
		, m_totalLength(0)
		, m_transferred(0)
	{
	}

	virtual ~PciScsiDmaEngine() noexcept = default;

	// ------------------------------------------------------------------------
	// Descriptor management
	// ------------------------------------------------------------------------

	inline void clearDescriptors() noexcept
	{
		m_descriptors.clear();
		m_totalLength = 0;
		m_transferred = 0;
		m_state = PciScsiDmaState::Idle;
	}

	inline void addDescriptor(const PciScsiDmaDescriptor& desc) noexcept
	{
		m_descriptors.append(desc);
		m_totalLength += desc.length;
	}

	inline const QVector<PciScsiDmaDescriptor>& descriptors() const noexcept
	{
		return m_descriptors;
	}

	inline bool empty() const noexcept
	{
		return m_descriptors.isEmpty();
	}

	// ------------------------------------------------------------------------
	// Direction
	// ------------------------------------------------------------------------

	inline void setDirection(PciScsiDmaDirection dir) noexcept
	{
		m_direction = dir;
	}

	inline PciScsiDmaDirection direction() const noexcept
	{
		return m_direction;
	}

	// ------------------------------------------------------------------------
	// State transitions
	// ------------------------------------------------------------------------

	inline void start() noexcept
	{
		m_state = PciScsiDmaState::Active;
		m_transferred = 0;
		onDmaStart();
	}

	inline void complete() noexcept
	{
		m_state = PciScsiDmaState::Completed;
		onDmaComplete();
	}

	inline void error() noexcept
	{
		m_state = PciScsiDmaState::Error;
		onDmaError();
	}

	inline PciScsiDmaState state() const noexcept
	{
		return m_state;
	}

	// ------------------------------------------------------------------------
	// Transfer bookkeeping
	// ------------------------------------------------------------------------

	inline quint64 totalLength() const noexcept
	{
		return m_totalLength;
	}

	inline quint64 transferred() const noexcept
	{
		return m_transferred;
	}

	inline void addTransferred(quint32 bytes) noexcept
	{
		m_transferred += bytes;
	}

	inline bool isComplete() const noexcept
	{
		return (m_state == PciScsiDmaState::Completed);
	}

	inline bool isActive() const noexcept
	{
		return (m_state == PciScsiDmaState::Active);
	}

	// ------------------------------------------------------------------------
	// Abstract virtual callbacks
	// ------------------------------------------------------------------------

protected:
	// Called when DMA is set to Active.
	virtual void onDmaStart() noexcept = 0;

	// Called once the DMA engine is marked Completed.
	virtual void onDmaComplete() noexcept = 0;

	// Called if the DMA engine enters Error state.
	virtual void onDmaError() noexcept = 0;

	// Called by derived PCI device to perform real memory copies.
	// This is intentionally NOT implemented here.
	virtual void performDmaTransfer() noexcept = 0;

	// ------------------------------------------------------------------------

private:
	QVector<PciScsiDmaDescriptor> m_descriptors;
	PciScsiDmaState               m_state;
	PciScsiDmaDirection           m_direction;
	quint64                       m_totalLength;
	quint64                       m_transferred;
};

#endif // PCI_SCSI_DMA_ENGINE_H

