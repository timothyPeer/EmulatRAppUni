#ifndef _EMULATRAPPUNI_FAULTLIB_ISVAADDRESSTRANSLATIONFAULT_H
#define _EMULATRAPPUNI_FAULTLIB_ISVAADDRESSTRANSLATIONFAULT_H

#pragma once
#include "../coreLib/types_core.h"
#include "../coreLib/enum_header.h"
#include "../PteLib/alpha_pte_core.h"
#include "../pteLib/ev6TranslateFullVA.h"
#include "../coreLib/BoxRequest.h"
#include "EBoxLib/VA_types.h"
#include "coreLib/enum_header.h"


// ---------------------------------------------------------------------------
// isAddressTranslationFault
//
// Convenience wrapper around ev6TranslateFullVA() that only reports
// whether a translation *would* fault.
//
//  - Returns 'true'  if the access SHOULD fault.
//  - Returns 'false' if the translation is valid.
//
//  This is suitable for trap staging, pre-fault checks, etc.
// ---------------------------------------------------------------------------
inline BoxResult isAddressTranslationFault(
	CPUIdType cpuId,
	VAType va,
	AccessKind access,
	Mode_Privilege mode,
	AlphaPTE& outPte) noexcept
{
	PAType pa_dummy = 0;
	TranslationResult ok = ev6TranslateFullVA(cpuId, va, access, mode, pa_dummy, outPte);
	if (ok != TranslationResult::Success)
		return BoxResult()
				.requestEnterPalMode()
				.faultDispatched()
	return BoxResult();
}

// ---------------------------------------------------------------------------
// isAddressTranslationFault (2-argument convenience version)
//
// Defaults:
//   - AccessKind::READ        (safe generic assumption for probe checks)
//   - Mode_Privilege::Kernel  (CPUStateIPRInterface reports current mode)
// 
// Returns:
//   true  = the VA *would* fault
//   false = translation valid
// ---------------------------------------------------------------------------
AXP_HOT AXP_FLATTEN BoxResult isAddressTranslationFault(
	CPUIdType cpuId,
	VAType va) noexcept
{

	// Default assumptions for the 2-argument variant:
	//   - AccessKind::READ      -> safe fallback for "probe" conditions
	//   - Privilege from IPRs   -> ask CPU state for current PS.CM
	const AccessKind  access = AccessKind::READ;
	const Mode_Privilege  mode = static_cast<Mode_Privilege>(getASN_Active(cpuId));

	PAType  pa_dummy = 0;
	AlphaPTE pte_dummy{};

	TranslationResult ok = 	ev6TranslateFullVA(cpuId, va, access, mode,	pa_dummy, pte_dummy);

	return BoxResult()
	.setTrapCodeFaultClass(mapITranslationFault(ok))
	.faultDispatched();
}
#endif
