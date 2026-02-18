// ============================================================================
// IPRStorage_Core_New.h - Backward-Compatible IPR Storage Wrapper
// ============================================================================
// This file provides backward compatibility with existing code while
// using the new cache-optimized PerCpuIPRState structure underneath.
//
// MIGRATION STRATEGY:
//   1. Old code continues to use IPRStorage& ipr = globalIPRBank()[cpuId]
//   2. Accessors forward to appropriate banks (hot64, hotExt, etc.)
//   3. New hot-path code uses globalIPRHot64(cpuId) directly
//   4. Gradually migrate to bank-specific access patterns
// ============================================================================

#ifndef IPRSTORAGE_CORE_NEW_H
#define IPRSTORAGE_CORE_NEW_H

#include "PerCpuIPRState.h"
#include "Ipr_Shift_Helpers.h"
#include "ICCSR_Bits_inl.h"

// ============================================================================
// IPRStorage - Backward Compatibility Wrapper
// ============================================================================
// This structure provides the same interface as the old IPRStorage,
// but forwards all accesses to the appropriate banks in PerCpuIPRState.
//
// NOTE: This is a REFERENCE WRAPPER - it contains a pointer to PerCpuIPRState
// and forwards all operations. Zero overhead when inlined.
// ============================================================================

class IPRStorage
{
private:
    PerCpuIPRState* m_state;  // Pointer to actual state
    
public:
    // ========================================================================
    // CONSTRUCTOR - wraps a PerCpuIPRState
    // ========================================================================
    
    explicit IPRStorage(PerCpuIPRState& state) noexcept
        : m_state(&state)
    {
    }
    
    // ========================================================================
    // BANK ACCESS - Direct access to underlying banks
    // ========================================================================
    // NEW: Provides direct access to cache-optimized banks
    
    inline IPRStorage_Hot64& hot64() noexcept {
        return m_state->hot64;
    }
    
    inline const IPRStorage_Hot64& hot64() const noexcept {
        return m_state->hot64;
    }
    
    inline IPRStorage_HotExt& hotExt() noexcept {
        return m_state->hotExt;
    }
    
    inline const IPRStorage_HotExt& hotExt() const noexcept {
        return m_state->hotExt;
    }
    
    inline IPRStorage_CBox& cbox() noexcept {
        return m_state->cbox;
    }
    
    inline const IPRStorage_CBox& cbox() const noexcept {
        return m_state->cbox;
    }
    
    inline IPRStorage_Cold& cold() noexcept {
        return m_state->cold;
    }
    
    inline const IPRStorage_Cold& cold() const noexcept {
        return m_state->cold;
    }
    
    inline IPRStorage_IBox& iBox() noexcept {
        return m_state->iBox;
    }
    
    inline const IPRStorage_IBox& iBox() const noexcept {
        return m_state->iBox;
    }
    
    inline IPRStorage_Hot_OSF& hot_osf() noexcept {
        return m_state->hot_osf;
    }
    
    inline const IPRStorage_Hot_OSF& hot_osf() const noexcept {
        return m_state->hot_osf;
    }
    
    // ========================================================================
    // BACKWARD COMPATIBILITY - Field Accessors
    // ========================================================================
    // These maintain the old interface but route to appropriate banks
    
    // Hot64 fields (most frequent access)
    inline quint64& fpcr() { return m_state->hot64.fpcr; }
    inline quint64& asn() { return m_state->hot64.asn; }
    inline quint64& cc() { return m_state->hot64.cc; }
    
    // HotExt fields (exception/PAL paths)
    inline quint64& va() { return m_state->hotExt.va; }
    inline quint64& exc_addr() { return m_state->hotExt.exc_addr; }
    inline quint64& ptbr() { return m_state->hotExt.ptbr; }
    inline quint64& vptb() { return m_state->hotExt.vptb; }
    inline quint64& pal_base() { return m_state->hotExt.pal_base; }
    inline quint64& scbb() { return m_state->hotExt.scbb; }
    inline quint64& pcbb() { return m_state->hotExt.pcbb; }
    
    inline quint64& usp() { return m_state->hotExt.usp; }
    inline quint64& ksp() { return m_state->hotExt.ksp; }
    inline quint64& esp() { return m_state->hotExt.esp; }
    inline quint64& ssp() { return m_state->hotExt.ssp; }
    
    inline quint64* pal_temp() { return m_state->hotExt.pal_temp; }
    inline quint64& iccsr() { return m_state->hotExt.iccsr; }
    inline quint64& i_ctl() { return m_state->hotExt.i_ctl; }
    inline quint64& m_ctl() { return m_state->hotExt.m_ctl; }
    inline quint64& dc_ctl() { return m_state->hotExt.dc_ctl; }
    inline quint64& mm_stat() { return m_state->hotExt.mm_stat; }
    inline quint64& exc_sum() { return m_state->hotExt.exc_sum; }
    inline quint64& exc_mask() { return m_state->hotExt.exc_mask; }
    
    // Cold fields (rare access)
    inline quint64& mces() { return m_state->cold.mces; }
    inline quint64& biu_addr() { return m_state->cold.biu_addr; }
    inline quint64& sl_rcv() { return m_state->cold.sl_rcv; }
    inline quint64& c_data() { return m_state->cold.c_data; }
    inline quint64& c_shift() { return m_state->cold.c_shift; }
    
    // ========================================================================
    // BACKWARD COMPATIBILITY - Composite Accessors
    // ========================================================================
    
    #pragma region FPCR Accessors
    
    inline quint64 getFpcr() const noexcept { return m_state->hot64.fpcr; }
    inline void setFpcr(quint64 fpcr_) noexcept { m_state->hot64.fpcr = fpcr_; }
    
    inline quint8 getFPCR_RoundingMode() const noexcept {
        return static_cast<quint8>((m_state->hot64.fpcr >> FPCR_RM_SHIFT) & FPCR_RM_MASK);
    }
    
    inline void setFPCR_RoundingMode(quint8 rm) noexcept {
        quint64 value = m_state->hot64.fpcr;
        value &= ~(FPCR_RM_MASK << FPCR_RM_SHIFT);
        value |= (static_cast<quint64>(rm) & FPCR_RM_MASK) << FPCR_RM_SHIFT;
        m_state->hot64.fpcr = value;
    }
    
    #pragma endregion
    
    #pragma region ICCSR Accessors
    
    inline bool iccsr_getFPE() const {
        return (m_state->hotExt.iccsr & ICCSR_Bits::FPE_MASK) != 0;
    }
    
    inline void iccsr_setFPE(bool enable) {
        if (enable)
            m_state->hotExt.iccsr |= ICCSR_Bits::FPE_MASK;
        else
            m_state->hotExt.iccsr &= ~ICCSR_Bits::FPE_MASK;
    }
    
    inline bool iccsr_getHWE() const {
        return (m_state->hotExt.iccsr & ICCSR_Bits::HWE_MASK) != 0;
    }
    
    inline void iccsr_setHWE(bool enable) {
        if (enable)
            m_state->hotExt.iccsr |= ICCSR_Bits::HWE_MASK;
        else
            m_state->hotExt.iccsr &= ~ICCSR_Bits::HWE_MASK;
    }
    
    inline bool iccsr_getBPE() const {
        return (m_state->hotExt.iccsr & ICCSR_Bits::BPE_MASK) != 0;
    }
    
    inline void iccsr_setBPE(bool enable) {
        if (enable)
            m_state->hotExt.iccsr |= ICCSR_Bits::BPE_MASK;
        else
            m_state->hotExt.iccsr &= ~ICCSR_Bits::BPE_MASK;
    }
    
    inline void iccsr_setBHE(bool enable) {
        if (enable)
            m_state->hotExt.iccsr |= ICCSR_Bits::BHE_MASK;
        else
            m_state->hotExt.iccsr &= ~ICCSR_Bits::BHE_MASK;
    }
    
    inline quint8 iccsr_getPC0() const {
        return (m_state->hotExt.iccsr >> ICCSR_Bits::PC0_SHIFT) & 0x3;
    }
    
    inline void iccsr_setPC0(quint8 value) {
        m_state->hotExt.iccsr = (m_state->hotExt.iccsr & ~ICCSR_Bits::PC0_MASK) |
                    ((quint64(value) & 0x3) << ICCSR_Bits::PC0_SHIFT);
    }
    
    #pragma endregion
    
    #pragma region DTB/ITB Staging Accessors
    
    inline void setDtb_Tag_Raw(quint64 tag) noexcept { m_state->hotExt.dtbTagScratch.raw = tag; }
    inline quint64 getDtb_Tag_Raw() const noexcept { return m_state->hotExt.dtbTagScratch.raw; }
    
    inline void setDtb_Tag_Vpn(quint64 vpn) noexcept { m_state->hotExt.dtbTagScratch.vpn = vpn; }
    inline quint64 getDtb_Tag_Vpn() const noexcept { return m_state->hotExt.dtbTagScratch.vpn; }
    
    inline void setDtb_Tag_Asn(quint8 asn) noexcept { m_state->hotExt.dtbTagScratch.asn = asn; }
    inline quint8 getDtb_Tag_Asn() const noexcept { return m_state->hotExt.dtbTagScratch.asn; }
    
    inline void setDtb_Tag_Gh(quint8 gh) noexcept { m_state->hotExt.dtbTagScratch.gh = gh; }
    inline quint8 getDtb_Tag_Gh() const noexcept { return m_state->hotExt.dtbTagScratch.gh; }
    
    inline void setItb_Tag_Va(VAType tag) noexcept { m_state->hotExt.itbTagScratch.va = tag; }
    inline VAType getItb_Tag_Va() const noexcept { return m_state->hotExt.itbTagScratch.va; }
    
    inline void setItb_Tag_Asn(ASNType tag) noexcept { m_state->hotExt.itbTagScratch.asn = tag; }
    inline ASNType getItb_Tag_Asn() const noexcept { return m_state->hotExt.itbTagScratch.asn; }
    
    inline void setDtb_Tag_Bank(quint8 bank_) noexcept {
        m_state->hotExt.dtbTagScratch.bank1 = (bank_ != 0);
    }
    inline quint8 getDtb_Tag_Bank() const noexcept {
        return m_state->hotExt.dtbTagScratch.bank1 ? 1U : 0U;
    }
    
    #pragma endregion
    
    inline void setMM_STAT(quint64 rawValue) noexcept {
        m_state->hotExt.mm_stat = rawValue;
    }
    
    // ========================================================================
    // RESET
    // ========================================================================
    
    inline void reset() noexcept {
        m_state->reset();
    }
    
    // ========================================================================
    // SIZE INFORMATION
    // ========================================================================
    
    static constexpr qsizetype getHotSize() {
        return sizeof(IPRStorage_Hot64) + sizeof(IPRStorage_HotExt);
    }
    
    static constexpr qsizetype getColdSize() {
        return sizeof(IPRStorage_Cold);
    }
    
    static constexpr qsizetype getTotalSize() {
        return sizeof(PerCpuIPRState);
    }
};

#endif // IPRSTORAGE_CORE_NEW_H
