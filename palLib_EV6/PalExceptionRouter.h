#ifndef PALEXCEPTIONROUTER_H
#define PALEXCEPTIONROUTER_H
#include "../faultLib/PendingEvent_Refined.h"
#include "../exceptionLib/Exception_Core_Refined.h"
#include "PALVectorId_Refined.h"

/**
 * @brief Check if exception class requires CALL_PAL calculation
 * @param ec ExceptionClass to check
 * @return true if PC calculation required (not a static vector)
 */
inline bool requiresCallPalCalculation(ExceptionClass ec) noexcept
{
    return ec == ExceptionClass::CALL_PAL;
}

/**
 * @brief Check if exception is memory-related
 * @param ec ExceptionClass to check
 * @return true if ITB/DTB exception
 */
inline bool isMemoryException(ExceptionClass ec) noexcept
{
    using EC = ExceptionClass;
    return ec == EC::ITB_MISS || ec == EC::ITB_ACV ||
           ec == EC::DTB_MISS_SINGLE || ec == EC::DTB_MISS_DOUBLE ||
           ec == EC::DFAULT;
}

/**
 * @brief Check if exception is synchronous (fault/trap)
 * @param ec ExceptionClass to check
 * @return true if synchronous, false if asynchronous
 */
inline bool isSynchronousException(ExceptionClass ec) noexcept
{
    using EC = ExceptionClass;
    return ec != EC::INTERRUPT && ec != EC::MCHK && ec != EC::RESET;
}
#endif // PALEXCEPTIONROUTER_H
