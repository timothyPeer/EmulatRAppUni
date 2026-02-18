#ifndef _EMULATRAPPUNI_CORELIB_IPR_AST_MASK_SEMANTICS_H
#define _EMULATRAPPUNI_CORELIB_IPR_AST_MASK_SEMANTICS_H

#pragma once
#include <QtGlobal>

#include "Axp_Attributes_core.h"

/*
    ipr_AST_mask_semantics.H

    Purpose:
      Provide a single, canonical implementation of the Alpha ASTEN/ASTSR
      masked read-modify-write (MTPR) semantics.

    Why:
      EmulatR has two entry paths into PAL/IPR mutation:
        (1) CALL_PAL grains (pipeline path)
        (2) Fault handling entry (runloop -> PalBox direct)
      Both MUST apply identical ASTEN/ASTSR semantics or you will see divergence.

    Alpha System Reference Manual (SRM) / Alpha Architecture:
      ASTEN and ASTSR are 4-bit masks and are written via an MTPR operation
      that uses bits in R16:
        - R16[3:0]  -> "keep" mask (when 1, preserve old bit; when 0, clear it)
        - R16[7:4]  -> "set"  mask (when 1, force bit on)
      NewValue = (OldValue AND KeepMask) OR SetMask
      Return value: R0 gets the old 4-bit value zero-extended.

    Source reference:
      Alpha AXP System Reference Manual, Version 6.0 (1994),
      IPR descriptions for ASTEN and ASTSR (MTPR semantics and R0 return).
      (Use the ASTEN / ASTSR IPR entries; they spell out the AND/OR mask form.)

    TODO: None
*/



#endif
