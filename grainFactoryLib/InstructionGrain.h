#ifndef INSTRUCTIONGRAIN_H
#define INSTRUCTIONGRAIN_H

#include <QtGlobal>
#include <QString>

#include "InstructionGrain_core.h"
#include "iGrain_helper_inl.h"
#include "coreLib/Axp_Attributes_core.h"

struct DecodedInstruction;
struct PipelineSlot;

enum GrainFlags : quint8 {
    GF_None = 0,
    GF_OperateFormat = 1 << 0,
    GF_MemoryFormat = 1 << 1,
    GF_BranchFormat = 1 << 2,
    GF_PALFormat = 1 << 3,
    GF_CanDualIssue = 1 << 4, // eligible to be paired (scheduler still enforces hazards)
    GF_NeedsStall = 1 << 5, // serializing / cannot pair; may require separation
};

struct InstructionGrain
{
    quint8 flags = GF_None;
    quint8 latency = 1;
    quint8 throughput = 1;
    quint8 _pad = 0;
   // not used. -- use DecodedInstruction::rawBits()
    InstructionGrain(quint32 raw, quint8 f, quint8 lat, quint8 thr)
        : flags(f), latency(lat), throughput(thr)
    {
    }
    virtual ~InstructionGrain() = default;

    virtual quint16 functionCode() const = 0;
    virtual QString mnemonic() const = 0;
    virtual quint8  opcode() const = 0;
    /**
     * @brief Get instruction classification
     *
     * @return GrainType (IntegerOperate, FloatOperate, Memory, Branch, etc.)
     */
    virtual GrainType grainType() const = 0;
    virtual void execute(PipelineSlot& slot) const noexcept = 0;

    virtual GrainPlatform platform() const noexcept { return GrainPlatform::NONE; }

    AXP_HOT AXP_ALWAYS_INLINE bool hasFlag(GrainFlags f) const noexcept {
        return (flags & static_cast<quint8>(f)) != 0;
    }
    AXP_HOT AXP_ALWAYS_INLINE bool eligibleForDualIssue() const noexcept {
        return hasFlag(GF_CanDualIssue);
    }

    
};


#endif // INSTRUCTIONGRAIN_H
