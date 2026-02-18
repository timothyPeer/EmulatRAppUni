#ifndef ONASTCHANGED_INL_H
#define ONASTCHANGED_INL_H
#include <QtGlobal>
#include "AlphaCPU.h"


inline void onASTChanged(AlphaCPU* cpu, quint8 oldMask, quint8 newMask)
{
    const CPUIdType cpuId = cpu->cpuId();
    auto& iprs = globalIPRBank()[cpuId];

    iprs.setASTMask(newMask);

    if ((newMask & iprs.astsr) != 0) {
        // There is now a pending AST that became visible
        global_IRQController().postASTInterrupt(cpuId);
    }

    // Optional logging
    DEBUG_LOG(std::format("AST changed CPU {}: {:02x} -> {:02x}",
                          cpuId, oldMask, newMask));
}


#endif // ONASTCHANGED_INL_H
