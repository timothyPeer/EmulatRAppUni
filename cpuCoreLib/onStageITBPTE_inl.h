#ifndef ONSTAGEITBPTE_INL_H
#define ONSTAGEITBPTE_INL_H
#include <QtGlobal>
#include "../pteLib/Ev6SiliconTLB_Singleton.h"
#include "../pteLib/alpha_pte_core.h"
#include "../coreLib/types_core.h"

inline void onStageITBPTE(CPUIdType cpuId, Realm realm, quint8 gh, VAType va, PFNType pfn, ASNType asn, bool isGlobal)
{
	auto& spam = globalEv6SPAM();

	// Create a PTE with the given parameters
	AlphaPTE pte;
	pte.setValid(true);           // Valid
	pte.setPFN(pfn);          // Physical frame number
	pte.setGH(gh);            // Granularity hint (bits 6:5)
	pte.setAsm(isGlobal);     // Address space match (global bit)

	// Set permissions (you'll need to determine appropriate permissions)
	// For SRM boot, typically kernel read/write/execute
	pte.setReadPermissions(true,false);         // Kernel read enable
	pte.setWritePermissions(true,false);         // Kernel write enable
	// Note: Execute permission is implicit for ITB entries

	// Insert into TLB using the manager's interface
	spam.tlbInsert(cpuId, realm, va, asn, pte);
}
#endif // ONSTAGEITBPTE_INL_H
