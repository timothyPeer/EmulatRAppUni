#ifndef AMASK_CONSTANTS_INL_H
#define AMASK_CONSTANTS_INL_H
#include <QtGlobal>
// ============================================================================
// AMASK feature bits (Architectural)
// ============================================================================
static constexpr quint64 AMASK_BWX = (1ULL << 0);  // Byte/Word extensions
static constexpr quint64 AMASK_FIX = (1ULL << 1);  // Integer extensions
static constexpr quint64 AMASK_CIX = (1ULL << 2);  // Count extensions
static constexpr quint64 AMASK_MVI = (1ULL << 3);  // Multimedia extensions
static constexpr quint64 AMASK_PAT = (1ULL << 4);  // Prefetch assist
static constexpr quint64 AMASK_PM  = (1ULL << 5);  // Performance monitoring


static constexpr quint64 AMASK_EMULATOR_SUPPORTED =
    AMASK_BWX |
    AMASK_FIX |
    AMASK_CIX |
    AMASK_PAT;   // add/remove deliberately

#endif // AMASK_CONSTANTS_INL_H
