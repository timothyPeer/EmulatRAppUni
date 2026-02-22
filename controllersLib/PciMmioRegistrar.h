// ============================================================================
// PciMmioRegistrar.h - ============================================================================
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
// PciMmioRegistrar.H  -  Map PCI BAR Windows into MMIO Space
// ============================================================================
// Purpose:
//   Bridges the PCI subsystem (PciSubsystem / PciDeviceManager / PciBus)
//   with your MMIO framework, without hard-coding any dependency on
//   SafeMemory or a specific MMIOManager implementation.
//
//   This module walks all PCI devices and their BARs and calls a
//   user-supplied mapper interface to actually install MMIO handlers.
//
// Design constraints:
//   - Header-only, pure ASCII
//   - QtCore only
//   - Depends on: PciSubsystem.H, PciDeviceManager.H, PciBus.H,
//                 PciScsiMmioInterface.H
//   - Does NOT depend on coreLib, SafeMemory, AlphaCPU, or MMIOManager.
//     Instead it uses a small abstract interface (IMmioRegionMapper) that
//     your real MMIOManager can implement.
//
// Integration sketch:
//
//   class MmioManager : public IMmioRegionMapper {
//   public:
//       void mapMmioRegion(quint64 base,
//                          quint32 size,
//                          PciScsiMmioInterface* dev) noexcept override {
//           // Register [base, base+size) with your SafeMemory/MMIO framework
//           // and install dev->mmioReadX/mmioWriteX as callbacks.
//       }
//   };
//
//   PciSubsystem pciSub(mmioBase, mmioLimit, 0x1000);
//   MmioManager  mmioMgr;
//   PciMmioRegistrar registrar(&pciSub, &mmioMgr);
//   registrar.registerAll();
//
// ============================================================================

#ifndef PCI_MMIO_REGISTRAR_H
#define PCI_MMIO_REGISTRAR_H

#include <QtGlobal>
#include <QString>

#include "PciSubsystem.H"
#include "PciDeviceManager.H"
#include "PciBus.H"
#include "PciScsiMmioInterface.H"

// ============================================================================
// IMmioRegionMapper
// ============================================================================
//
// Abstract interface used by PciMmioRegistrar to request that a BAR window be
// mapped into the emulator's MMIO fabric. Your concrete MMIOManager should
// implement this interface and perform the actual mapping into SafeMemory or
// equivalent.
//
// ============================================================================
class IMmioRegionMapper
{
public:
	virtual ~IMmioRegionMapper() noexcept = default;

	// Map a MMIO region into the system MMIO fabric.
	//
	// Parameters:
	//   base  - physical base address of the BAR
	//   size  - size in bytes of the BAR (from PciBarInfo::size)
	//   dev   - MMIO-capable device implementing PciScsiMmioInterface
	//
	// The implementation should:
	//   - Register [base, base+size) as a MMIO region
	//   - Route reads/writes to dev->mmioRead8/16/32/64 and
	//     dev->mmioWrite8/16/32/64.
	//
	virtual void mapMmioRegion(quint64              base,
		quint32              size,
		PciScsiMmioInterface* dev) noexcept = 0;
};

// ============================================================================
// PciMmioRegistrar
// ============================================================================
//
// Walks all devices in a PciSubsystem and registers their BAR windows with
// a user-supplied IMmioRegionMapper. This is the final step in wiring PCI
// devices into the MMIO address space.
//
// Notes:
//   - Only memory-space BARs (isMemory == true) are considered here.
//   - I/O-space BARs (isMemory == false) are ignored by this registrar; they
//     can be handled separately if needed.
//   - The registrar does not do any ownership management or error recovery;
//     it simply calls the supplied mapper for each valid BAR.
//
// ============================================================================
class PciMmioRegistrar
{
public:
	PciMmioRegistrar(PciSubsystem* subsystem,
		IMmioRegionMapper* mapper) noexcept
		: m_subsystem(subsystem)
		, m_mapper(mapper)
	{
	}

	~PciMmioRegistrar() noexcept = default;

	// ------------------------------------------------------------------------
	// registerAll()
	// ------------------------------------------------------------------------
	//
	// Iterates over all registered PCI devices and their BARs, and for each
	// MMIO BAR (isMemory == true, size > 0), invokes:
	//
	//     mapper->mapMmioRegion(bar.base, bar.size, dev->mmioDevice)
	//
	// Returns:
	//   number of BAR regions successfully handed off to the mapper.
	//
	quint32 registerAll() noexcept
	{
		if (!m_subsystem || !m_mapper)
		{
			return 0;
		}

		quint32 mappedCount = 0;

		PciDeviceManager& devMgr = m_subsystem->deviceManager();
		QVector<PciRegisteredDevice>& devs = devMgr.devices();

		for (int i = 0; i < devs.size(); ++i)
		{
			PciRegisteredDevice& rec = devs[i];

			if (!rec.mmioDevice)
			{
				continue;
			}

			const QVector<PciBarInfo>& bars = rec.bars;
			for (int b = 0; b < bars.size(); ++b)
			{
				const PciBarInfo& bar = bars[b];

				// Only map valid memory-space BARs.
				if (!bar.isMemory || bar.size == 0)
				{
					continue;
				}

				m_mapper->mapMmioRegion(bar.base, bar.size, rec.mmioDevice);
				++mappedCount;
			}
		}

		return mappedCount;
	}

private:
	PciSubsystem* m_subsystem; // non-owning
	IMmioRegionMapper* m_mapper;    // non-owning
};

#endif // PCI_MMIO_REGISTRAR_H

