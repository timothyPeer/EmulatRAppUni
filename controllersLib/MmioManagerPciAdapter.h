#ifndef MMIO_MANAGER_PCI_ADAPTER_H
#define MMIO_MANAGER_PCI_ADAPTER_H

// ============================================================================
// MmioManagerPciAdapter.H  (aligned with mmio_Manager/mmio_CoreData)
// ============================================================================
// Purpose:
//   Adapt your existing MMIOManager to the generic IMmioRegionMapper interface
//   used by PciMmioRegistrar, so that each PCI BAR window becomes a normal
//   MMIO region with MMIODeviceHandlers forwarding to PciScsiMmioInterface.
//
// Integration path:
//   - Construct MMIOManager as usual
//   - Construct PciSubsystem / PciDeviceManager / PciMmioRegistrar
//   - Wrap MMIOManager with MmioManagerPciAdapter
//   - Call registrar.registerAll() to register all PCI BARs.
//
//     MMIOManager mmio;
//     PciSubsystem pci(...);
//     MmioManagerPciAdapter mmioAdapter(&mmio);
//     PciMmioRegistrar registrar(&pci, &mmioAdapter);
//     registrar.registerAll();
//
// Notes:
//   - Header-only, pure ASCII
//   - Uses your RegionDescriptor, RegionAttributes, MMIODeviceHandlers,
//     MMIOStatus, FenceKind from mmio_CoreData/mmio_Manager.
//   - Does NOT modify mmio_Manager; it just calls its public API.
// ============================================================================

#include <QtGlobal>
#include "mmio_Manager.h"          // MMIOManager, RegionDescriptor, MMIODeviceHandlers
#include "PciMmioRegistrar.H"      // IMmioRegionMapper
#include "PciScsiMmioInterface.H"  // PciScsiMmioInterface
#include "PAL_core.h"

class MmioManagerPciAdapter : public IMmioRegionMapper
{
public:
	explicit MmioManagerPciAdapter(MMIOManager* mgr) noexcept
		: m_mgr(mgr)
	{
	}

	virtual ~MmioManagerPciAdapter() noexcept = default;

	// ------------------------------------------------------------------------
	// mapMmioRegion
	// ------------------------------------------------------------------------
	//
	// Called by PciMmioRegistrar for each MMIO BAR. We:
	//   1) Build a RegionDescriptor
	//   2) Register it with MMIOManager
	//   3) Install MMIODeviceHandlers that forward to the PCI device.
	//
	virtual void mapMmioRegion(quint64               base,
		quint32               size,
		PciScsiMmioInterface* dev) noexcept override
	{
		if (!m_mgr || !dev || size == 0)
			return;

		// --------------------------------------------------------------------
		// Step 1: Build RegionDescriptor
		// --------------------------------------------------------------------
		RegionDescriptor desc;
		desc.deviceUid = dev->deviceUid();    // PCI device must expose a stable UID
		desc.basePA = base;
		desc.size = size;
		desc.debugName = QStringLiteral("PCI_BAR_%1").arg(desc.deviceUid);

		// Conservatively chosen attributes; can be refined later using
		// DeviceCatalog/BarTemplate if desired.
		desc.attrs.minAlignment = 4;                     // 4-byte aligned
		desc.attrs.supportedWidths = 0x0F;                  // 1/2/4/8 bytes
		desc.attrs.cachePolicy = mmio_CachePolicy::Uncacheable;
		desc.attrs.sideEffectOnRead = false;
		desc.attrs.sideEffectOnWrite = true;                  // typical for HBAs
		desc.attrs.stronglyOrdered = false;
		desc.attrs.regEndian = mmio_Endianness::LITTLE;

		if (!m_mgr->registerRegion(desc))
		{
			return;
		}

		// --------------------------------------------------------------------
		// Step 2: Install device handlers
		// --------------------------------------------------------------------
		MMIODeviceHandlers handlers;

		handlers.onRead = [dev](quint64 offset,
			quint8  width,
			quint64& outValue) -> MMIOStatus
			{
				switch (width)
				{
				case 1:
					outValue = dev->mmioRead8(offset);
					break;
				case 2:
					outValue = dev->mmioRead16(offset);
					break;
				case 4:
					outValue = dev->mmioRead32(offset);
					break;
				case 8:
					outValue = dev->mmioRead64(offset);
					break;
				default:
					return MMIOStatus::WIDTH_FAULT;
				}
				return MMIOStatus::OK;
			};

		handlers.onWrite = [dev](quint64 offset,
			quint8  width,
			quint64 value) -> MMIOStatus
			{
				switch (width)
				{
				case 1:
					dev->mmioWrite8(offset, static_cast<quint8>(value));
					break;
				case 2:
					dev->mmioWrite16(offset, static_cast<quint16>(value));
					break;
				case 4:
					dev->mmioWrite32(offset, static_cast<quint32>(value));
					break;
				case 8:
					dev->mmioWrite64(offset, static_cast<quint64>(value));
					break;
				default:
					return MMIOStatus::WIDTH_FAULT;
				}
				return MMIOStatus::OK;
			};

		handlers.onReset = [dev]()
			{
				dev->mmioReset();
			};

		handlers.onFence = [dev](palCore_FenceKind kind)
			{
				dev->mmioFence(kind);
			};

		m_mgr->setDeviceHandlers(desc.deviceUid, handlers);
	}

private:
	MMIOManager* m_mgr;  // non-owning
};

#endif // MMIO_MANAGER_PCI_ADAPTER_H
