#ifndef PTE_DUAL_CORE_H
#define PTE_DUAL_CORE_H


#include <QtGlobal>

// ============================================================================
// Minimal Bank-Specific TLB Support for Existing SPAM Interface
// ============================================================================
// This adds bank-specific DTB insertion WITHOUT changing your existing
// tlbInsert() signature or breaking any existing code.
// ============================================================================

/**
 * @brief DTB Bank Selection Policy
 *
 * Controls which DTB bank(s) to insert entries into on EV6 (21264).
 *
 * EV6 DTB Architecture:
 * - DTB0 (Bank 0): 64 entries, handles VAs with VA[12] = 0
 * - DTB1 (Bank 1): 64 entries, handles VAs with VA[12] = 1
 * - Total capacity: 128 entries
 */
enum class DTBBankPolicy : quint8
{
    AUTO_SELECT = 0,   // Select bank based on VA[12] (DEFAULT - recommended)
    DUAL_BANK   = 1,   // Insert into both banks (for critical mappings)
    BANK0_ONLY  = 2,   // Force Bank 0 only
    BANK1_ONLY  = 3,   // Force Bank 1 only
};

/**
 * @brief Helper to determine which DTB bank a VA belongs to
 *
 * EV6 uses VA bit 12 to select DTB bank:
 * - VA[12] = 0 -> Bank 0
 * - VA[12] = 1 -> Bank 1
 *
 * @param va Virtual address
 * @return 0 for Bank 0, 1 for Bank 1
 */
inline constexpr quint8 dtbBankForVA(quint64 va) noexcept {
    return (va >> 12) & 0x1;
}

/**
 * @brief Check if VA belongs to Bank 0
 */
inline constexpr bool isBank0VA(quint64 va) noexcept {
    return ((va >> 12) & 0x1) == 0;
}

/**
 * @brief Check if VA belongs to Bank 1
 */
inline constexpr bool isBank1VA(quint64 va) noexcept {
    return ((va >> 12) & 0x1) == 1;
}


/**
 * @brief TLB Realm for insertion operations
 *
 * Maps to your existing Realm enum in SPAM manager:
 * - Realm::I (value 0) -> ITB (Instruction TLB)
 * - Realm::D (value 1) -> DTB (Data TLB)
 */
enum class TLBRealm : quint8
{
    ITB = 0,    // Instruction TLB (maps to Realm::I)
    DTB = 1,    // Data TLB (maps to Realm::D)
};

/**
 * @brief Convert TLBRealm to SPAM Realm
 */
inline constexpr Realm toSPAMRealm(TLBRealm realm) noexcept {
    return (realm == TLBRealm::ITB) ? Realm::I : Realm::D;
}

#endif // pte_dual_core_h__
