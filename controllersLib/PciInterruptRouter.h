// ============================================================================
// PciInterruptRouter.h - ============================================================================
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
// PciInterruptRouter.H
// ============================================================================
// Purpose:
//   Maps PCI device INTx# signals (INTA#/INTB#/INTC#/INTD#) into system IRQ
//   lines using the routing tables defined in PciBus.
//
//   This module does NOT assume any specific IRQ controller implementation.
//   Instead, the emulator supplies an IRQ sink interface with:
//       - assertIrq(quint8 irqLine)
//       - clearIrq(quint8 irqLine)
//
//   This router is the final link in the PCI -> MMIO -> CPU interrupt chain.
//
// Design Constraints:
//   - Header-only
//   - QtCore only
//   - Pure ASCII (UTF-8, no BOM)
//   - No SafeMemory / AlphaCPU / MMIOManager dependencies
//
// Integration Path:
//   1) Emulator starts up, builds PciSubsystem
//   2) Devices registered using PciSubsystem::registerScsiController()
//   3) Emulator installs PciInterruptRouter:
//         PciInterruptRouter router(&pciSubsystem, &irqController);
//   4) When a PCI device asserts interrupt:
//         router.handleDeviceInterrupt(rec, /*assert=*/true);
//   5) When clearing interrupt:
//         router.handleDeviceInterrupt(rec, /*assert=*/false);
//
// ============================================================================

#ifndef PCI_INTERRUPT_ROUTER_H
#define PCI_INTERRUPT_ROUTER_H

#include <QtGlobal>
#include <QString>

#include "PciSubsystem.H"
#include "PciBus.H"
#include "PciDeviceManager.H"

// ============================================================================
// Abstract IRQ Sink Interface
// ============================================================================
//
// The emulator provides an implementation of this. Typically this will
// forward into:
//   � IRQController
//   � CPU interrupt lines
//   � SMP-aware IPI/interrupt distributor
//
// ============================================================================
class IPciIrqSink
{
public:
	virtual ~IPciIrqSink() noexcept = default;

	// Assert system IRQ line corresponding to PCI device interrupt.
	virtual void assertIrq(quint8 irqLine) noexcept = 0;

	// Clear IRQ line.
	virtual void clearIrq(quint8 irqLine) noexcept = 0;
};


// ============================================================================
// PciInterruptRouter
// ============================================================================
//
// The job:
//
//   Device (PciRegisteredDevice) -> PCI Bus (INTx route) -> System IRQ
//
// Notes:
//
//   � PCI standard says each device has an INTx line (A/B/C/D).
//   � Most single-function devices always use INTA#.
//   � Multi-function devices rotate INTA->INTB->INTC->INTD per function number.
//   � DEC machines usually follow standard PCI rotation.
//
// This router implements the standard rotation and uses PciBus::route(intx)
// to get the final IRQ line.
//
// ============================================================================
class PciInterruptRouter
{
public:
	PciInterruptRouter(PciSubsystem* subsystem,
		IPciIrqSink* sink) noexcept
		: m_subsystem(subsystem)
		, m_sink(sink)
	{
	}

	~PciInterruptRouter() noexcept = default;

	// ------------------------------------------------------------------------
	// Core handler (assert or clear device interrupt)
	// ------------------------------------------------------------------------
	//
	// Parameters:
	//   rec    = PciRegisteredDevice* for the device signaling interrupt
	//   assert = true  -> device raises interrupt
	//            false -> device clears interrupt
	//
	void handleDeviceInterrupt(PciRegisteredDevice* rec,
		bool assert) noexcept
	{
		if (!rec || !m_subsystem || !m_sink)
		{
			return;
		}

		const quint8 bus = rec->location.bus;

		// Find bus object
		PciBus* pciBus = nullptr;
		auto it = m_subsystem->buses().find(bus);
		if (it == m_subsystem->buses().end())
		{
			return;
		}
		pciBus = &it.value();

		// Determine INTx line based on function number (PCI spec rotation)
		const PciIntxLine intx = intxForFunction(rec->location.function);

		// Lookup routing entry
		const PciInterruptRoute& route = pciBus->route(intx);

		// Notify IRQ sink
		if (assert)
		{
			m_sink->assertIrq(route.irqLine);
		}
		else
		{
			m_sink->clearIrq(route.irqLine);
		}
	}

private:
	PciSubsystem* m_subsystem;
	IPciIrqSink* m_sink;

	// ------------------------------------------------------------------------
	// PCI INTx# rotation logic
	// ------------------------------------------------------------------------
	//
	// PCI rotates the INTx# line by function number:
	//
	//   func 0 -> INTA
	//   func 1 -> INTB
	//   func 2 -> INTC
	//   func 3 -> INTD
	//   func >=4 -> wrap (mod 4)
	//
	static inline PciIntxLine intxForFunction(quint8 logFunction) noexcept
	{
		switch (logFunction & 0x03)
		{
		case 0: return PciIntxLine::INTA;
		case 1: return PciIntxLine::INTB;
		case 2: return PciIntxLine::INTC;
		default: return PciIntxLine::INTD;
		}
	}
};

#endif // PCI_INTERRUPT_ROUTER_H
