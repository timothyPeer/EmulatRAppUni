#ifndef ONIAP_INSTRUCTION_INL_H
#define ONIAP_INSTRUCTION_INL_H
#include "pteLib/Ev6SiliconTLB_Singleton.h"
#include "types_core.h"
#include <QtGlobal>
#include "../coreLib/LoggingMacros.h"

/// \brief Explicit ASN invalidation (IAP instruction)
inline void onIAP_Instruction(CPUIdType cpuId, ASNType targetASN)
{
    TRACE_LOG(std::format("CPU{} IAP invalidate ASN {}", cpuId, targetASN));

    // NOW we bump the epoch - this invalidates all entries for this ASN
    auto& spam = globalEv6SPAM();
    spam.invalidateASN(cpuId, targetASN);
}

#endif // ONIAP_INSTRUCTION_INL_H
