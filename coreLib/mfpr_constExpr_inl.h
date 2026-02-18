#ifndef MFPR_CONSTEXPR_INL_H
#define MFPR_CONSTEXPR_INL_H
#include <QtGlobal>
// ============================================================================
// CALL_PAL MFPR selector constants (EV6 baseline)
// R16 contains one of these values
// ============================================================================
static constexpr quint64 MFPR_ASN = 0x0006; // Address Space Number+

static constexpr quint64 MFPR_ESP = 0x001E; // User Stack Pointer
static constexpr quint64 MFPR_FEN = 0x000B; // User Stack Pointer
static constexpr quint64 MFPR_IPL = 0x000E; // Current IPL
static constexpr quint64 MFPR_MCES = 0x0010; // Current MCES
static constexpr quint64 MFPR_PCBB = 0x0012; // Current PCBB
static constexpr quint64 MFPR_PRBR = 0x0013; // Current PRBR
static constexpr quint64 MFPR_PTBR = 0x0015; // Current PTBR
static constexpr quint64 MFPR_SCBB = 0x0016; // Current SCBB
static constexpr quint64 MFPR_SISR = 0x0019; // Current sisr
static constexpr quint64 MFPR_SSP = 0x0020; // Current SSP
static constexpr quint64 MFPR_SYSPTBR = 0x0032;// Move from processor register SYSPTBR
static constexpr quint64 MFPR_TBCHK = 0x001A;// Move from processor register TBCHK
static constexpr quint64 MFPR_USP = 0x0022;// Move from processor register USP
static constexpr quint64 MFPT_VIRBND = 0x0030;// Move from processor register VIRBND
static constexpr quint64 MFPR_VPTB = 0x0029;// Move from processor register VPTB
static constexpr quint64 MFPR_WHAMI = 0x003F; // Move from processor register WHAM

static constexpr quint64 MTPR_ASTEN = 0x0026;//
static constexpr quint64 MTPR_ASTSR = 0x0027;//
static constexpr quint64 MTPR_DATFX = 0x002E;//
static constexpr quint64 MTPR_ESP = 0x001F;//
static constexpr quint64 MTPR_FEN = 0x000B;//
static constexpr quint64 MTPR_IPIR = 0x000D;//
static constexpr quint64 MTPR_IPL = 0x000E;//
static constexpr quint64 MTPR_MCES = 0x0011;//
static constexpr quint64 MTPR_PERFMON = 0x002B;//
static constexpr quint64 MTPR_PRBR = 0x0014;//
static constexpr quint64 MTPR_SCBB = 0x0017;//
static constexpr quint64 MTPR_SIRR = 0x0018;//
static constexpr quint64 MTPR_SSP = 0x0021;//
static constexpr quint64 MTPR_SYSPTBR = 0x0033;//
static constexpr quint64 MTPR_TBIA = 0x001B;//
static constexpr quint64 MTPR_TBIAP = 0x001C;//
static constexpr quint64 MTPR_TBIS = 0x0001D;//
static constexpr quint64 MTPR_TBISD = 0x0024;//
static constexpr quint64 MTPR_TBISI = 0x0025;//
static constexpr quint64 MTPR_USP = 0x0023;//
static constexpr quint64 MTPR_VIRBND = 0x0031;//
static constexpr quint64 MTPR_VPTB = 0x002A;//


// -----------------------------
// PAL base / vectoring
// -----------------------------
static constexpr quint64 MFPR_PAL_BASE  = 0x000E; // PAL base address

#endif // MFPR_CONSTEXPR_INL_H
