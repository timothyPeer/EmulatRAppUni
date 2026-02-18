// ============================================================================
// IPRStorage_IBox.h
// ============================================================================
// IBox (Instruction Box) IPR storage structure for Alpha AXP emulation
// - I_CTL register with bitfield accessors
// - I_STAT register
// - IC_FLUSH controls
// - NO dependencies on AlphaCPU (removed circular dependency)
// ============================================================================

#ifndef IPRSTORAGE_IBOX_H
#define IPRSTORAGE_IBOX_H

#include <QtGlobal>

// Forward declaration (to break circular dependency with AlphaCPU)
// If you need flushICache functionality, it should be in a separate module
struct PipelineSlot;

// ============================================================================
// I_CTL Register Structure
// ============================================================================
// I_CTL bit layout (from IPR_Storage.h comments):
//   Bits 63:48 - SEXT(VPTB[47])     (RW,0)
//   Bits 47:30 - VPTB[47:30]        (RW,0)
//   Bits 29:24 - CHIP_ID[5:0]       (RO)
//   Bit  23    - BIST_FAIL          (RO,0)
//   Bit  22    - TB_MB_EN           (RW,0)
//   Bit  21    - MCHK_EN            (RW,0)
//   Bit  20    - CALL_PAL_R23       (RW,0)
//   Bit  19    - PCT1_EN            (RW,0)
//   Bit  18    - PCT0_EN            (RW,0)
//   Bit  17    - SINGLE_ISSUE_H     (RW,0)
//   Bit  16    - VA_FORM_32         (RW,0)
//   Bit  15    - VA_48              (RW,0)
//   Bit  14    - SL_RCV             (RO)
//   Bit  13    - SL_XMIT            (WO)
//   Bit  12    - HWE                (RW,0)
//   Bits 11:10 - BP_MODE[1:0]       (RW,0)
//   Bits 9:8   - SBE[1:0]           (RW,0)
//   Bits 7:6   - SDE[1:0]           (RW,0)
//   Bits 5:3   - SPE[2:0]           (RW,0)
//   Bits 2:1   - IC_EN[1:0]         (RW,3)
//   Bit  0     - SPCE               (RW,0)
// ============================================================================

struct ICtlRegister
{
    quint64 raw = 0;

    // ---- FIELD DEFINITIONS ----

    // Bits 63:48 - Sign-extended VPTB high bits
    quint16 vptb_sext() const noexcept { return (raw >> 48) & 0xFFFF; }
    void set_vptb_sext(quint16 val) noexcept {
        raw = (raw & ~(0xFFFFULL << 48)) | (quint64(val & 0xFFFF) << 48);
    }

    // Bits 47:30 - Virtual Page Table Base
    quint32 vptb() const noexcept { return (raw >> 30) & 0xFFFF; }
    void set_vptb(quint32 val) noexcept {
        raw = (raw & ~(0xFFFFULL << 30)) | (quint64(val & 0xFFFF) << 30);
    }

    // Bits 29:24 - CHIP_ID (RO)
    quint8 chip_id() const noexcept { return (raw >> 24) & 0x3F; }

    // Bit 23 - BIST_FAIL (RO)
    bool bist_fail() const noexcept { return (raw >> 23) & 1; }

    // Bit 22 - TB_MB_EN
    bool tb_mb_en() const noexcept { return (raw >> 22) & 1; }
    void set_tb_mb_en(bool en) noexcept {
        raw = (raw & ~(1ULL << 22)) | (quint64(en) << 22);
    }

    // Bit 21 - MCHK_EN
    bool mchk_en() const noexcept { return (raw >> 21) & 1; }
    void set_mchk_en(bool en) noexcept {
        raw = (raw & ~(1ULL << 21)) | (quint64(en) << 21);
    }

    // Bit 20 - CALL_PAL_R23
    bool call_pal_r23() const noexcept { return (raw >> 20) & 1; }
    void set_call_pal_r23(bool en) noexcept {
        raw = (raw & ~(1ULL << 20)) | (quint64(en) << 20);
    }

    // Bit 19 - PCT1_EN
    bool pct1_en() const noexcept { return (raw >> 19) & 1; }
    void set_pct1_en(bool en) noexcept {
        raw = (raw & ~(1ULL << 19)) | (quint64(en) << 19);
    }

    // Bit 18 - PCT0_EN
    bool pct0_en() const noexcept { return (raw >> 18) & 1; }
    void set_pct0_en(bool en) noexcept {
        raw = (raw & ~(1ULL << 18)) | (quint64(en) << 18);
    }

    // Bit 17 - SINGLE_ISSUE_H
    bool single_issue() const noexcept { return (raw >> 17) & 1; }
    void set_single_issue(bool en) noexcept {
        raw = (raw & ~(1ULL << 17)) | (quint64(en) << 17);
    }

    // Bit 16 - VA_FORM_32
    bool va_form_32() const noexcept { return (raw >> 16) & 1; }
    void set_va_form_32(bool en) noexcept {
        raw = (raw & ~(1ULL << 16)) | (quint64(en) << 16);
    }

    // Bit 15 - VA_48
    bool va_48() const noexcept { return (raw >> 15) & 1; }
    void set_va_48(bool en) noexcept {
        raw = (raw & ~(1ULL << 15)) | (quint64(en) << 15);
    }

    // Bit 14 - SL_RCV (RO)
    bool sl_rcv() const noexcept { return (raw >> 14) & 1; }

    // Bit 13 - SL_XMIT (WO)
    void set_sl_xmit(bool en) noexcept {
        raw = (raw & ~(1ULL << 13)) | (quint64(en) << 13);
    }

    // Bit 12 - HWE
    bool hwe() const noexcept { return (raw >> 12) & 1; }
    void set_hwe(bool en) noexcept {
        raw = (raw & ~(1ULL << 12)) | (quint64(en) << 12);
    }

    // Bits 11:10 - BP_MODE
    quint8 bp_mode() const noexcept { return (raw >> 10) & 0x3; }
    void set_bp_mode(quint8 val) noexcept {
        raw = (raw & ~(0x3ULL << 10)) | (quint64(val & 0x3) << 10);
    }

    // Bits 9:8 - SBE
    quint8 sbe() const noexcept { return (raw >> 8) & 0x3; }
    void set_sbe(quint8 val) noexcept {
        raw = (raw & ~(0x3ULL << 8)) | (quint64(val & 0x3) << 8);
    }

    // Bits 7:6 - SDE
    quint8 sde() const noexcept { return (raw >> 6) & 0x3; }
    void set_sde(quint8 val) noexcept {
        raw = (raw & ~(0x3ULL << 6)) | (quint64(val & 0x3) << 6);
    }

    // Bits 5:3 - SPE
    quint8 spe() const noexcept { return (raw >> 3) & 0x7; }
    void set_spe(quint8 val) noexcept {
        raw = (raw & ~(0x7ULL << 3)) | (quint64(val & 0x7) << 3);
    }

    // Bits 2:1 - IC_EN
    quint8 ic_en() const noexcept { return (raw >> 1) & 0x3; }
    void set_ic_en(quint8 val) noexcept {
        raw = (raw & ~(0x3ULL << 1)) | (quint64(val & 0x3) << 1);
    }

    // Bit 0 - SPCE
    bool spce() const noexcept { return raw & 1; }
    void set_spce(bool en) noexcept {
        raw = (raw & ~1ULL) | quint64(en);
    }
};

// ============================================================================
// IPRStorage_IBox Structure
// ============================================================================

struct IPRStorage_IBox
{
    ICtlRegister i_ctl{};    // I-box control
    quint64 i_stat = 0;      // I-box status
    quint64 ic_flush = 0;    // write-only, triggers flush
    quint64 ic_flush_asm = 0; // alt flush for ASM routines

    // NOTE: flushICache() functionality removed to break circular dependency
    // If you need ICache flushing, implement it in a separate service/helper
    // that takes AlphaProcessorContext& as a parameter
    //
    // Example:
    //   void flushICache(AlphaProcessorContext* ctx) {
    //       ctx.invalidateAllLines_iCache();
    //       ctx.clearInstructionPrefetchState();
    //       ctx.resetBranchPredictorHistory();
    //   }
};

#endif // IPRSTORAGE_IBOX_H
