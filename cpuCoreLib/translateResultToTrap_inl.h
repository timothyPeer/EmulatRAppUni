#ifndef TRANSLATERESULTTOTRAP_INL_H
#define TRANSLATERESULTTOTRAP_INL_H
#include "EBoxLib/VA_types.h"
#include "enum_header.h"
#include "faultLib/fault_core.h"
#include "../memoryLib/SafeMemory.h"

inline TrapCode_Class translateResultToTrap(TranslationResult r, AccessKind k) noexcept
{
    switch (r) {
    case TranslationResult::TlbMiss:
        return (k == AccessKind::EXECUTE)
                   ? TrapCode_Class::ITB_MISS
                   : TrapCode_Class::DTB_MISS;

    case TranslationResult::PermissionDenied:
        return (k == AccessKind::EXECUTE)
                   ? TrapCode_Class::ITB_ACCESS_VIOLATION
                   : TrapCode_Class::DTB_ACCESS_VIOLATION;

    case TranslationResult::NonCanonical:
    case TranslationResult::Misaligned:
        return TrapCode_Class::DTB_FAULT; // EV6 rule

    default:
        return TrapCode_Class::MACHINE_CHECK; // panic translation
    }
}

inline TrapCode_Class translateStatusToTrap(SafeMemory::MEM_STATUS st) noexcept {
    switch (st) {
    case SafeMemory::MEM_STATUS::Ok:
        return TrapCode_Class::NONE;

    case SafeMemory::MEM_STATUS::AccessViolation:
        return TrapCode_Class::DTB_ACCESS_VIOLATION;

    case SafeMemory::MEM_STATUS::Misaligned:
        return TrapCode_Class::DTB_FAULT;

    case SafeMemory::MEM_STATUS::BusError:
    default:
        return TrapCode_Class::MACHINE_CHECK; // Machine check
    }
}

#endif // TRANSLATERESULTTOTRAP_INL_H
