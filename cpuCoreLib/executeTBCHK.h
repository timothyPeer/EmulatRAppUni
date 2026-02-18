// =============================================================================
// executeTBCHK.h
// -----------------------------------------------------------------------------
// PAL helper: emulate MFPR TBCHK
//
// Semantics from Alpha Architecture Reference Manual, Sec. 13.3.19 TBCHK,
// page 13-24 (II-A), and Alpha AXP System Reference Manual V6, Sec. 13.3.19.
//
// Operation (architectural):
//   R0 <- 0
//   IF {TBCHK implemented} THEN
//       R0<0> <- {indicator that VA in R16 is in TB}
//   ELSE
//       R0<63> <- 1
//
// Description (summary):
//   - VA to be checked is in R16 (any address within the page).
//   - If ASNs are implemented, only entries with current ASN are considered.
//   - If TBCHK is not implemented in hardware, MFPR TBCHK returns bit63=1,
//     bit0=0 to indicate "function not implemented".
//   - If implemented, bit63=0 and bit0 indicates presence (1) or absence (0)
//     of the VA in the Translation Buffer.
//
// Emulator design choice:
//   - We *do* implement TBCHK in the emulator.
//   - Therefore, we always return bit63=0 and use bit0 = presence flag.
//   - All other bits are returned as zero.
//
// Integration assumptions:
//   - CPUStateIPRInterface provides:
//       - quint64 getIntReg(quint8 index) const;
//       - void    setIntReg(quint8 index, quint64 value);
//       - quint16 getCurrentASN() const;         // current ASN IPR value
//       - quint16 getCpuId() const;              // current CPU index
//   - Ev6SiliconTLB_Singleton + Ev6TLBInterface (or SPAM manager) provide
//       - bool tbchk(quint16 cpuId, quint64 va, quint16 asn);
//         which returns true iff there is a valid TB entry covering VA for ASN.
//     If you do not have tbchk(...), you can implement it using your existing
//     lookupPTE(...) or equivalent TLB/SPAM lookup.
//
// TODO for integration:
//   - If your Ev6TLBInterface (or SPAMShardManager) does not yet expose a
//     tbchk(...) helper, add one as a thin wrapper around your existing
//     TLB lookup that ignores access permissions and just reports "mapped?".
//   - If ASNs are not yet implemented, getCurrentASN() can simply return 0
//     and tbchk(...) can ignore the ASN parameter.
// =============================================================================
#include "CPUStateIPRInterface.h"
#include "HWPCB_helpers_inline.h"
#include "Ev6SiliconTLB_Singleton.h"
#include "AlphaCPU.h"


inline void executeTBCHK(AlphaCPU* argCpu) noexcept
{
	// Defensive: null check (should not happen in normal PAL usage)
	if (!argCpu) {
		return;
	}

	quint8 cpuId = argCpu->cpuId();
	// Architectural step 1:
	//   R0 <- 0
	//
	// We do this first so that any unimplemented bits are naturally 0.
	argCpu->registerBankInteger().write(0u, 0u);

	// Architectural: "The virtual address to be checked is specified in R16
	// and may be any address within the desired page."
	// (Alpha AXP System Reference Manual V6, Sec. 13.3.19 TBCHK).
	const quint64 va = argCpu->registerBankInteger().read(16u);

	// Architectural: "If ASNs are implemented, only those Translation Buffer
	// entries that are associated with the current value of the ASN IPR will
	// be checked for the virtual address."
	//
	// In the emulator we can always supply some ASN (0 if not implemented).
	const quint16 asn = getASN_Active(cpuId);
	
	// Emulator policy: TBCHK is *implemented*.
	// So we do not need to synthesize the "not implemented" case (bit63=1).
	// Instead we actually interrogate the TLB / SPAM cache.

	bool present = false;

	// Query the Ev6 TLB / SPAM manager.
	// This assumes you add a tbchk(...) helper to your Ev6 TLB facade.
	{
		auto& tlb = Ev6SiliconTLB_Singleton::interface();
		
		// Recommended facade in Ev6TLBInterface:
		//
		//   bool tbchk(quint16 cpuId, quint64 va, quint16 asn) const noexcept;
		//
		// Internally, it would:
		//   - normalize VA to page tag
		//   - pick ITB/DTB realm as appropriate for TBCHK (usually both or "any")
		//   - consult SPAMShardManager / PTE cache for a valid entry
		//
		// For now we assume such a helper exists.
		present = tlb.tbchkAny(cpuId, va, asn);
	}

	// Architectural result formatting (implemented case):
	//
	//   - If implemented:
	//       bit63 = 0
	//       bit0  = 1 if VA present in TB; 0 if not present
	//
	//   - If *not* implemented:
	//       bit63 = 1, bit0 = 0
	//
	// In this emulator we always behave as "implemented".
	quint64 result = 0u;

	if (present) {
		result |= 0x1u;   // set bit 0
	}

	// bit63 stays 0, all other bits 0.

	// Write back to R0, as required by MFPR TBCHK semantics.
	argCpu->registerBankInteger().write(0u, result);
}
