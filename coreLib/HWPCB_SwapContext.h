// ============================================================================
// HWPCB_SwapContext.h - ============================================================================
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

#ifndef HWPCB_SWAPCONTEXT_H
#define HWPCB_SWAPCONTEXT_H

// ============================================================================
// HWPCB_SwapContext.h
// ============================================================================
// Context switch implementation for CALL_PAL SWPCTX
//
// Translates between the internal HWPCB C++ struct layout and the
// architectural physical memory layout defined by EV6 PALcode.
//
// The physical HWPCB is a quadword-aligned structure in guest physical
// memory. The C++ HWPCB struct is cache-line optimized and does NOT
// match the physical layout. This file bridges the two.
//
// Usage from PAL handler:
//
//   auto result = hwpcbSwapContext(
//       cpuId, hwpcb, oldPCBB_PA, newPCBB_PA,
//       guestMemory, tlb, hwCycleCounter
//   );
//
// ============================================================================

#include "Axp_Attributes_core.h"
#include "memoryLib/GuestMemory.h"

// ============================================================================
// Physical Memory Access Helpers
// ============================================================================
// These perform untranslated physical address reads/writes through
// GuestMemory -> SafeMemory. No TLB, no virtual translation.
// Equivalent to EV6 HW_LD/HW_ST with physical bit set.
// ============================================================================

namespace HWPCBPhysical {

    // Read a quadword from guest physical memory
    AXP_HOT AXP_ALWAYS_INLINE
        quint64 hwLoad(GuestMemory* guestMem, quint64 pa) noexcept
    {
        quint64 value = 0;
        MEM_STATUS status = guestMem->readPA(pa, &value, sizeof(quint64));
        return value;
    }

    // Write a quadword to guest physical memory
    AXP_HOT AXP_ALWAYS_INLINE
        void hwStore(GuestMemory* guestMem, quint64 pa, quint64 value) noexcept
    {
        MEM_STATUS status = guestMem->writePA(pa, &value, sizeof(quint64));
    }

} // namespace HWPCBPhysical

#endif 

