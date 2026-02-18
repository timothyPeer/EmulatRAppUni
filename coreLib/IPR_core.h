#ifndef IPR_core_h__
#define IPR_core_h__


#include "coreLib/Axp_Attributes_core.h"
#include <QtGlobal>

#include "BitUtils.h"

// defaults for IRQ

AXP_HOT AXP_ALWAYS_INLINE quint8 findHighestSetBit(const quint32 mask) noexcept
{
	return BitUtils::highestSetBit(mask);
}
/*
// ============================================================================
// IPR Numbers (Complete list for MFPR and MTPR)
// ============================================================================

readIPR() / writeIPR() exist to implement MFPR_xxx / MTPR_xxx only
The ASA/OpenVMS PALcode definition makes MFPR/MTPR a PAL entry whose function field selects the IPR, and whose operands are in integer registers (not “method parameters” in your C++ sense):
MFPR_xxx: reads the IPR “specified by the PALcode function field” and writes the result to R0; privileged-only (PS<CM> must be 0).

Alpha_AXP_System_Reference_Manu…
MTPR_xxx: writes IPR-specific operands from R16 (and R17 reserved for future use) to the IPR; privileged-only; effect guaranteed active on the next instruction; may also return a value in R0 depending on which IPR it is.
Alpha_AXP_System_Reference_Manu…
So, from a software architecture perspective:
PalService::readIPR(iprId, ctx) should be called only by the MFPR grain/handler.
PalService::writeIPR(iprId, ctx) should be called only by the MTPR grain/handler.
Everything else should be separate PAL handlers, not shoved through IPR read/write.

*/
enum HW_IPR : quint16
{
	IPR_MFPR_ASN = 0x0006,
	IPR_MFPR_ESP = 0x001E,  // Read: 0x001E, Write: 0x001F
	IPR_MFPR_IPL = 0x000E,
	IPR_MFPR_MCES = 0x0010,  // Read: 0x0010, Write: 0x0011
	IPR_MFPR_PCBB = 0x0012,
	IPR_MFPR_PRBR = 0x0013,
	IPR_MFPR_PTBR = 0x0015,
	IPR_MFPR_SCBB = 0x0016,
	IPR_MFPR_SISR = 0x0019,
	IPR_MFPR_SSP = 0x0020,  // Read: 0x0020, Write: 0x0021
	IPR_MFPR_SYSPTBR = 0x0032,  // Read: 0x0032, Write: 0x0033
	IPR_MFPR_TBCHK = 0x001A,
	IPR_MFPR_USP = 0x0022,  // Read: 0x0022, Write: 0x0023
	IPR_MFPR_VIRBND = 0x0030,  // Read: 0x0030, Write: 0x0031
	IPR_MFPR_VPTB = 0x0029,  // Read: 0x0029, Write: 0x002A
	
	IPR_MFPR_WHAMI = 0x003F,
	IPR_MTPR_ASTEN = 0x0026,
	IPR_MTPR_ASTSR = 0x0027,
	IPR_MTPR_DATFX = 0x002E,
	IPR_MTPR_ESP = 0x001F,  // Read: 0x001E, Write: 0x001F

	IPR_MTPR_IPIR = 0x000D,
	IPR_MTPR_IPL = 0X000E,
	IPR_MTPR_MCES = 0x0011,  // Read: 0x0010, Write: 0x0011
	IPR_MTPR_PERFMON = 0X002B,
	IPR_MTPR_PRBR = 0X0014,
	IPR_MTPR_SCBB = 0X0017,
	IPR_MTPR_SIRR = 0x0018,
	IPR_MTPR_SSP = 0x0021,  // Read: 0x0020, Write: 0x0021
	IPR_MTPR_SYSPTBR = 0x0033,  // Read: 0x0032, Write: 0x0033

	IPR_MTPR_TBIA = 0x001B, // Triggers 
	IPR_MTPR_TBIAP = 0x001C, // Triggers 
	IPR_MTPR_TBIS = 0x001D,// Triggers 
	IPR_MTPR_TBISD = 0x0024,// Triggers 
	IPR_MTPR_TBISI = 0x0025,// Triggers 
	IPR_MTPR_USP = 0x0023,  // Read: 0x0022, Write: 0x0023
	IPR_MTPR_VIRBND = 0x0031,
	IPR_MTPR_VPTB = 0x002A,  // Read: 0x0029, Write: 0x002A

	// common function codes.
	IPR_FEN = 0X000B,
};

#endif