#ifndef RAISEFAULTHELPER_INL_H
#define RAISEFAULTHELPER_INL_H
#include "fault_core.h"
#include <QtGlobal>
#include "../memoryLib/SafeMemory.h"
// 
// 
// TrapCode_Class raiseTranslationFault(quint64 va, TranslationResult tr) noexcept
// {
//     TrapCode_Class trap = TrapCode_Class::MACHINE_CHECK;
// 
//     switch (tr) {
//     case TranslationResult::TlbMiss:
//         trap = TrapCode_Class::DTB_MISS;
//         break;
//     case TranslationResult::PermissionDenied:
//         trap = TrapCode_Class::DTB_ACCESS_VIOLATION;
//         break;
//     case TranslationResult::NonCanonical:
//     case TranslationResult::Misaligned:
//         trap = TrapCode_Class::DTB_FAULT;
//         break;
//     default:
//         trap = TrapCode_Class::MACHINE_CHECK;
//         break;
//     }
// 
//     return trap;
// }

#endif // RAISEFAULTHELPER_INL_H
