#pragma once
#include <QtGlobal>

// ============================================================================
// FPCR helpers
//
// FPCR layout (logical view, summarized):
//   - Rounding mode (RM) field
//   - Trap enable bits for INV, DZE, OVF, UNF, INE
//   - Sticky exception flags for same conditions
//   - Underflow mode and other implementation bits
//
// The exact bit positions differ slightly across documents, but the common
// Alpha definition (IEEE mode) is:
//
//   Bits 63:61  Reserved
//   Bit  60     INV summary flag (sticky)
//   Bit  59     DZE summary flag
//   Bit  58     OVF summary flag
//   Bit  57     UNF summary flag
//   Bit  56     INE summary flag
//   Bits 55:52  Trap enables for INV, DZE, OVF, UNF (INE usually shares)
//   Bits 51:48  Rounding mode and underflow mode
//   Remaining bits reserved or implementation specific.
//
// For emulator purposes we expose clear get/set helpers for:
//   - Rounding mode
//   - Trap enable bits
//   - Sticky status bits
//
// Reference: ASA Appendix B, Section B.2 IEEE Floating Point Environment.
// ============================================================================

// Rounding mode: 2-bit field (example encoding) at bits [49:48]
// 00 = Round to nearest
// 01 = Round toward zero
// 10 = Round toward +infinity
// 11 = Round toward -infinity
//
// Note: If you are using a different FPCR layout from your HRM, adjust these
// shifts/masks to match your chosen reference.

static constexpr quint64 FPCR_RM_SHIFT = 48;
static constexpr quint64 FPCR_RM_MASK = 0x3ULL;

// Trap enable bits; assume bits 55:52 hold TE_INV, TE_DZE, TE_OVF, TE_UNF
// and INE is handled in the same group. You can refine if your HRM differs.

static constexpr quint64 FPCR_TE_INV_SHIFT = 52;
static constexpr quint64 FPCR_TE_DZE_SHIFT = 53;
static constexpr quint64 FPCR_TE_OVF_SHIFT = 54;
static constexpr quint64 FPCR_TE_UNF_SHIFT = 55;
// Optional: in some descriptions, INE trap enable is bit 56 or shares field.
// Here we expose a logical helper and you can wire it to your actual bit.

static constexpr quint64 FPCR_TE_INV_MASK = 0x1ULL;
static constexpr quint64 FPCR_TE_DZE_MASK = 0x1ULL;
static constexpr quint64 FPCR_TE_OVF_MASK = 0x1ULL;
static constexpr quint64 FPCR_TE_UNF_MASK = 0x1ULL;

// Sticky exception flags; assume 60:56 are sticky flags for INV,DZE,OVF,UNF,INE
static constexpr quint64 FPCR_ST_INV_SHIFT = 60;
static constexpr quint64 FPCR_ST_DZE_SHIFT = 59;
static constexpr quint64 FPCR_ST_OVF_SHIFT = 58;
static constexpr quint64 FPCR_ST_UNF_SHIFT = 57;
static constexpr quint64 FPCR_ST_INE_SHIFT = 56;

static constexpr quint64 FPCR_ST_MASK = 0x1ULL;




