#ifndef PAGEBOUNDARY_ALIGNMENTCHECK_H
#define PAGEBOUNDARY_ALIGNMENTCHECK_H

#include <QtGlobal>
#include "../coreLib/Axp_Attributes_core.h"

AXP_HOT AXP_FLATTEN bool checkPageCrossing(quint64 va, quint8 accessSize, bool& crossesPage) noexcept {
    static constexpr quint64 PAGE_SIZE = 8192;  // 8KB Alpha pages
    static constexpr quint64 PAGE_MASK = PAGE_SIZE - 1;

    const quint64 startPage = va & ~PAGE_MASK;           // Page start of first byte
    const quint64 endPage = (va + accessSize - 1) & ~PAGE_MASK;  // Page start of last byte

    crossesPage = (startPage != endPage);
    return !crossesPage;  // Return true if access stays within single page
}

AXP_HOT AXP_FLATTEN bool validatePageBoundaryAccess(quint64 va, quint8 accessSize, bool& shouldTrap) noexcept {
    shouldTrap = false;

    // First check basic alignment
    bool basicAlignTrap;
    if (!validateVAAlignment(va, accessSize, basicAlignTrap)) {
        shouldTrap = basicAlignTrap;
        return false;
    }

    // Then check page boundary crossing
    bool crossesPage;
    if (!checkPageCrossing(va, accessSize, crossesPage)) {
        if (crossesPage) {
            // Always trap on page boundary crossing - this is policy
            shouldTrap = true;
            return false;
        }
    }

    return true;  // Access is both aligned and within single page
}

#endif // PAGEBOUNDARY_ALIGNMENTCHECK_H
