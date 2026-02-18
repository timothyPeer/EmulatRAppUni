#ifndef TRAPTRANSLATION_INL_H
#define TRAPTRANSLATION_INL_H
#pragma once
#include "../coreLib/TrapCodes.h"
#include "../vaLib/TranslationResult.h"
#include "../memory/SafeMemory.h"

inline TrapCode_Class translateResultToTrap(TranslationResult r, AccessKind k) noexcept
{
    switch (r) {
    case TranslationResult::TlbMiss:
        return (k == AccessKind::EXECUTE)
                   ? TrapCode_Class::ITB_MISS
                   : TrapCode_Class::DTB_MISS;

    case TranslationResult::PermissionDenied:
        return (k == AccessKind::EXECUTE)
                   ? TrapCode_Class::ITB_ACV
                   : TrapCode_Class::DTB_ACV;

    case TranslationResult::NonCanonical:
    case TranslationResult::BadAlignment:
        return TrapCode_Class::DTB_FAULT; // EV6 rule

    default:
        return TrapCode_Class::MCHK; // panic translation
    }
}

inline TrapCode_Class translateStatusToTrap(SafeMemory::MEM_STATUS st) noexcept {
    switch (st) {
    case SafeMemory::MEM_STATUS::Ok:
        return TrapCode_Class::NONE;

    case SafeMemory::MEM_STATUS::AccessViolation:
        return TrapCode_Class::DTB_ACV;

    case SafeMemory::MEM_STATUS::Unaligned:
        return TrapCode_Class::DTB_FAULT;

    case SafeMemory::MEM_STATUS::BusError:
    default:
        return TrapCode_Class::MCHK; // Machine check
    }
}

#endif // TRAPTRANSLATION_INL_H
