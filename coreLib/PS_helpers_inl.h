// ============================================================================
// PS_helpers_inl.h - ============================================================================
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

#ifndef PS_HELPERS_INL_H
#define PS_HELPERS_INL_H

#include <QtGlobal>

// ============================================================================
// Processor Status (PS) helpers - Alpha EV6
// ----------------------------------------------------------------------------
// Bit layout (per ASA / EV6):
//
// 63-62  Reserved (MBZ)
// 61-56  SP_ALIGN   (stack byte alignment, 0-63)
// 55-13  Reserved (MBZ)
// 12-8   IPL        (interrupt priority level, 0-31)
// 7      VMM        (virtual machine monitor)
// 6-5    Reserved (MBZ)
// 4-3    CM         (current mode: 0=K,1=E,2=S,3=U)
// 2      IP         (interrupt pending)
// 1-0    SW         (software-defined)
//
// Policy notes:
// - MBZ fields must be preserved on writes.
// - PAL mode is NOT encoded here; it is tracked via PC[0] by PalService.
// - CALL_PAL WR_PS may only write a restricted subset.
// ============================================================================

#ifndef AXP_ALWAYS_INLINE 
#define AXP_ALWAYS_INLINE  inline
#endif

// ---------------------------------------------------------------------------
// Bit shifts and masks
// ---------------------------------------------------------------------------

// SP_ALIGN (61-56)
static constexpr quint8  PS_SP_ALIGN_SHIFT = 56;
static constexpr quint64 PS_SP_ALIGN_MASK = (0x3FULL << PS_SP_ALIGN_SHIFT);

// IPL (12-8)
static constexpr quint8  PS_IPL_SHIFT = 8;
static constexpr quint64 PS_IPL_MASK = (0x1FULL << PS_IPL_SHIFT);

// VMM (7)
static constexpr quint8  PS_VMM_SHIFT = 7;
static constexpr quint64 PS_VMM_MASK = (1ULL << PS_VMM_SHIFT);

// CM (4-3)
static constexpr quint8  PS_CM_SHIFT = 3;
static constexpr quint64 PS_CM_MASK = (0x3ULL << PS_CM_SHIFT);

// IP (2)
static constexpr quint8  PS_IP_SHIFT = 2;
static constexpr quint64 PS_IP_MASK = (1ULL << PS_IP_SHIFT);

// SW (1-0)
static constexpr quint8  PS_SW_SHIFT = 0;
static constexpr quint64 PS_SW_MASK = (0x3ULL << PS_SW_SHIFT);

// PS<IV> - Integer Overflow Trap Enable
// When set, integer arithmetic instructions with /V
// generate an arithmetic exception on overflow.
static constexpr quint64 PS_IV_BIT = (1ULL << 6);

// ---------------------------------------------------------------------------
// Extractors
// ---------------------------------------------------------------------------
AXP_ALWAYS_INLINE  quint8 ps_getSPAlign(quint64 ps) noexcept
{
	return static_cast<quint8>((ps & PS_SP_ALIGN_MASK) >> PS_SP_ALIGN_SHIFT);
}

AXP_ALWAYS_INLINE  quint8 ps_getIPL(quint64 ps) noexcept
{
	return static_cast<quint8>((ps & PS_IPL_MASK) >> PS_IPL_SHIFT);
}

AXP_ALWAYS_INLINE  bool ps_getVMM(quint64 ps) noexcept
{
	return (ps & PS_VMM_MASK) != 0;
}

AXP_ALWAYS_INLINE  quint8 ps_getCM(quint64 ps) noexcept
{
	return static_cast<quint8>((ps & PS_CM_MASK) >> PS_CM_SHIFT);
}

AXP_ALWAYS_INLINE  bool ps_getIP(quint64 ps) noexcept
{
	return (ps & PS_IP_MASK) != 0;
}

AXP_ALWAYS_INLINE  quint8 ps_getSW(quint64 ps) noexcept
{
	return static_cast<quint8>((ps & PS_SW_MASK) >> PS_SW_SHIFT);
}

// ---------------------------------------------------------------------------
// Mutators (preserve all other bits)
// ---------------------------------------------------------------------------
AXP_ALWAYS_INLINE  quint64 ps_setSPAlign(quint64 ps, quint8 align) noexcept
{
	align &= 0x3F;
	ps &= ~PS_SP_ALIGN_MASK;
	ps |= (static_cast<quint64>(align) << PS_SP_ALIGN_SHIFT);
	return ps;
}

AXP_ALWAYS_INLINE  quint64 ps_setIPL(quint64 ps, quint8 ipl) noexcept
{
	ipl &= 0x1F;
	ps &= ~PS_IPL_MASK;
	ps |= (static_cast<quint64>(ipl) << PS_IPL_SHIFT);
	return ps;
}

AXP_ALWAYS_INLINE  quint64 ps_setVMM(quint64 ps, bool vmm) noexcept
{
	ps &= ~PS_VMM_MASK;
	if (vmm) ps |= PS_VMM_MASK;
	return ps;
}

AXP_ALWAYS_INLINE  quint64 ps_setCM(quint64 ps, quint8 cm) noexcept
{
	cm &= 0x3;
	ps &= ~PS_CM_MASK;
	ps |= (static_cast<quint64>(cm) << PS_CM_SHIFT);
	return ps;
}

AXP_ALWAYS_INLINE  quint64 ps_setIP(quint64 ps, bool ip) noexcept
{
	ps &= ~PS_IP_MASK;
	if (ip) ps |= PS_IP_MASK;
	return ps;
}

AXP_ALWAYS_INLINE  quint64 ps_setSW(quint64 ps, quint8 sw) noexcept
{
	sw &= 0x3;
	ps &= ~PS_SW_MASK;
	ps |= (static_cast<quint64>(sw) << PS_SW_SHIFT);
	return ps;
}

// ---------------------------------------------------------------------------
// Predicates
// ---------------------------------------------------------------------------
AXP_ALWAYS_INLINE  bool ps_isKernelMode(quint64 ps) noexcept
{
	return ps_getCM(ps) == 0;
}

AXP_ALWAYS_INLINE  bool ps_interruptsMasked(quint64 ps, quint8 level) noexcept
{
	return ps_getIPL(ps) >= (level & 0x1F);
}

// ---------------------------------------------------------------------------
// CALL_PAL WR_PS policy
// ----------------------------------------------------------------------------
// Writable via WR_PS (recommended default):
//   - IPL
//   - SW bits
//
// Optional (PAL/OS dependent):
//   - CM (if you want WR_PS to allow mode changes)
//
// Never writable via WR_PS:
//   - SP_ALIGN
//   - VMM
//   - IP
//   - Reserved bits
// ---------------------------------------------------------------------------

static constexpr quint64 PS_WR_PS_WRITABLE_MASK =
PS_IPL_MASK
| PS_SW_MASK
// | PS_CM_MASK   // OPTIONAL: enable only if OS PAL requires it
;

// Canonical sanitizer
AXP_ALWAYS_INLINE  quint64 sanitizePS_ForWR_PS(quint64 currentPS,
	quint64 requestedPS) noexcept
{
	const quint64 preserved = currentPS & ~PS_WR_PS_WRITABLE_MASK;
	const quint64 applied = requestedPS & PS_WR_PS_WRITABLE_MASK;
	return preserved | applied;
}

#endif // PS_HELPERS_INL_H
