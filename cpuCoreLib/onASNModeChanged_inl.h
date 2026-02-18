#ifndef ONASNMODECHANGED_INL_H
#define ONASNMODECHANGED_INL_H

#include "types_core.h"
#include <QtGlobal>
#include "AlphaCPU.h"
#include "../coreLib/HWPCB_helpers_inline.h"
#include "../pteLib/Ev6SiliconTLB_Singleton.h"
#include "../coreLib/LoggingMacros.h"

/// \brief Handle ASN register change (process context switch)
/// \param cpuId The CPU whose ASN is changing
/// \param oldASN The previous ASN value
/// \param newASN The new ASN value
/// \details
/// Called when the OS writes to the ASN IPR during a context switch.
/// This does NOT invalidate TLB entries - that's the entire point of ASNs!
/// TLB entries from multiple ASNs coexist; hardware automatically filters
/// based on the current ASN value.

inline void onASNModeChanged(CPUIdType cpuId, quint8 oldASN, quint8 newASN)
{
	// 1. Validate new ASN
	if (newASN >= ASN_MAX) {
		ERROR_LOG(std::format("CPU{} invalid ASN {} (max={})",
			cpuId, newASN, ASN_MAX));
		// Alpha behavior: typically wraps or traps
		// For safety, clamp to valid range
		newASN = newASN & (ASN_MAX - 1);
	}

	// 2. Update the current ASN tracking
	setASN_Active(cpuId, newASN);

	// 3. Log the context switch
	TRACE_LOG(std::format("CPU{} ASN context switch: {} -> {}",
		cpuId, oldASN, newASN));

	// NOTE: We do NOT bump the ASN epoch here!
	// TLB entries remain valid and hardware filters by ASN tag.
	// Only explicit invalidation operations (IAP, TBIA, etc.) bump epochs.
}

#endif // ONASNMODECHANGED_INL_H
