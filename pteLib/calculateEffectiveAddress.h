#ifndef CALCULATEEFFECTIVEADDRESS_H
#define CALCULATEEFFECTIVEADDRESS_H
#include <QtGlobal>
#include "../grainFactoryLib/DecodedInstruction.h"
#include "../grainFactoryLib/DecodedInstruction_inl.h"
#include "../machineLib/PipeLineSlot.h"

#include "../coreLib/Axp_Attributes_core.h"


// ---------------------------------------------------------------------
// calculateEffectiveAddress
// ---------------------------------------------------------------------
    // ReSharper disable once CppMemberFunctionMayBeStatic
AXP_HOT AXP_ALWAYS_INLINE quint64 calculateEffectiveAddress(const PipelineSlot& slot) noexcept
{
    // EA = Rb + sign_extend(disp16)
    quint64 base = slot.readIntReg(slot.di.rb);
    qint16 disp = static_cast<qint16>(extractMemDisp(slot.di.rawBits()) & 0xFFFF);
    return base + static_cast<quint64>(disp);
}

#endif // CALCULATEEFFECTIVEADDRESS_H
