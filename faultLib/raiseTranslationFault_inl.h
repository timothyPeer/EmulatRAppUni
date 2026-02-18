#ifndef RAISETRANSLATIONFAULT_INL_H
#define RAISETRANSLATIONFAULT_INL_H
#include <QtGlobal>
#include "fault_core.h"
#include "../coreLib/Axp_Attributes_core.h"
#include "coreLib/VA_types.h"
#include "faultLib/FaultDispatcher.h"

AXP_HOT  AXP_ALWAYS_INLINE void raiseTranslationFault(CPUIdType cpuId, quint64 va, TranslationResult tr, FaultDispatcher* faultSink) noexcept
{
    TrapCode_Class trap = TrapCode_Class::MACHINE_CHECK;

    switch (tr) {
    case TranslationResult::TlbMiss:
        trap = TrapCode_Class::DTB_MISS;
        break;
    case TranslationResult::AccessViolation:
        trap = TrapCode_Class::DTB_ACCESS_VIOLATION;
        break;
    case TranslationResult::NonCanonical:
    case TranslationResult::Unaligned:
        trap = TrapCode_Class::DTB_FAULT;
        break;
    default:
        trap = TrapCode_Class::MACHINE_CHECK;
        break;
    }

    faultSink->setPendingEvent(makeTranslationFault(cpuId,va, tr, false));
}

AXP_HOT  inline TrapCode_Class translateSafeMemStatusToTrap(MEM_STATUS st) noexcept
{
    switch (st) {
    case MEM_STATUS::Ok:
        return TrapCode_Class::NONE;
    case MEM_STATUS::AccessViolation:
        return TrapCode_Class::DTB_ACCESS_VIOLATION;
    case MEM_STATUS::Un_Aligned:
        return TrapCode_Class::UN_ALIGNED;
    case MEM_STATUS::BusError:
    case MEM_STATUS::Time_Out:
    default:
        return TrapCode_Class::MACHINE_CHECK;
    }
}
#endif // RAISETRANSLATIONFAULT_INL_H
