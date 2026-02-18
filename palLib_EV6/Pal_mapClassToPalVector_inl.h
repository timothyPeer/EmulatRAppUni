#ifndef PAL_MAPCLASSTOPALVECTOR_INL_H
#define PAL_MAPCLASSTOPALVECTOR_INL_H
#include "PALVectorId_Refined.h"
#include "../faultLib/PendingEvent_Refined.h"
#include "../exceptionLib/Exception_Core_Refined.h"
#include "Global_PALVectorTable.h"




// ====================================================================
// Map ExceptionClass to MemoryFaultType
// ====================================================================
AXP_HOT AXP_FLATTEN PalVectorId mapClassToPalVector(const PendingEvent& ev) noexcept
{
    switch (ev.exceptionClass)
    {
    case ExceptionClass::ITB_MISS:          return PalVectorId::ITB_MISS;
    case ExceptionClass::ITB_ACV:           return PalVectorId::IACCVIO;
    case ExceptionClass::DTB_MISS_SINGLE:   return PalVectorId::DTB_MISS_SINGLE;
    case ExceptionClass::DTB_MISS_DOUBLE: return PalVectorId::DTB_MISS_DOUBLE;
    case ExceptionClass::DFAULT:            return PalVectorId::DFAULT;
    case ExceptionClass::UNALIGN:           return PalVectorId::UNALIGN;
    case ExceptionClass::OPCDEC:            return PalVectorId::OPCDEC;
    case ExceptionClass::FEN:               return PalVectorId::FEN;
    case ExceptionClass::ARITH:             return PalVectorId::ARITH;
    case ExceptionClass::INTERRUPT:         return PalVectorId::INTERRUPT;
    case ExceptionClass::MCHK:              return PalVectorId::MCHK;
    case ExceptionClass::RESET:             return PalVectorId::RESET;
    case ExceptionClass::CALL_PAL:          return PalVectorId::INVALID; // Computed dynamically
    default:                                return PalVectorId::INVALID;
    }
}





#endif // PAL_MAPCLASSTOPALVECTOR_INL_H
