/*
* 
Two copies are maintained.  Processor context-MMU and process context. 
The PTBR currently active in the CPU's MMU
The PTBR belonging to the process

Context Switch: 
HWPCB[oldCPU].ptbr -> saved
HWPCB[newCPU].ptbr -> IPRStorage.ptbr (becomes active)

PTBR -> Level 0 (L1) page directory
		 |
		 Level 1 (L2)
			 |
			 Level 2 (L3)
				 |
				 PTE -> PFN + flags


 VA[42:13] -> VPN
VPN[42:35]  -> L1 index
VPN[34:23]  -> L2 index
VPN[22:13]  -> L3 index

*/

#pragma once
#include "CPUStateIPRInterface.h"
#include "Global_HWPCBBank_Interface.h"
#include "IPRStorage_core.h"
#include "Ev6SiliconTLB_Singleton.h"
#include "AlphaCPU.h"



inline void executePTBR(AlphaCPU* argCpu)
{
	if (!argCpu) {
		return;
	}

	const quint8 cpuId = argCpu->cpuId();

	// PTBR value is written via MT_PR PTBR
	// PAL convention: R16 holds the new PTBR

	quint64 newPTBR = argCpu->registerBankInteger().read(16);

	// Update HWPCB
	{
		auto& hwpcb = globalHWPCBController()(cpuId);
		hwpcb.setPTBR(newPTBR);
	}

	// Update IPR Storage (if PTBR is mirrored there)
	{
		auto& ipr = globalIPRBank()[cpuId];
		ipr.ptbr = newPTBR;   // correct: mirror architectural PTBR
	}

	// OSF/1 / Tru64 PAL requires TLB flush on PTBR change
	// VMS PAL sometimes uses ASN techniques to avoid global flush
	// For safety: invalidate the DTB for this CPU
	{
		auto& tlb = Ev6SiliconTLB_Singleton::interface();
		tlb.tbia(cpuId);     // Per-CPU TBIA
	}

	// Done – new page table base active
}
