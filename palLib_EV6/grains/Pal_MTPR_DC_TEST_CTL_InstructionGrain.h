#ifndef PAL_MTPR_DC_TEST_CTL_INSTRUCTIONGRAIN_H
#define PAL_MTPR_DC_TEST_CTL_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_DC_TEST_CTL_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_DC_TEST_CTL
//
// Function code:
//   CALL_PAL 0x57  (implementation-specific, EV6 cache test)
//
// Summary:
//   Writes the Dcache Test Control register. This is an implementation-specific
//   register used to control diagnostic or test modes of the data cache.
//
// References:
//   - Alpha EV6 Hardware Reference Manual, DC_TEST_CTL.
//   - Implementation-specific cache test features.
// ============================================================================


#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_DC_TEST_CTL_InstructionGrain : public PalInstructionBase<0x57>
{
public:
    Pal_MTPR_DC_TEST_CTL_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_DC_TEST_CTL"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        const quint64 pc  = getPC(cpu);
        const quint64 val = cpu.readIntReg(di.ra);
        const int id      = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Provide accessor:
        //        cpu.writeIPR_DC_TEST_CTL_Active(id, val);
        //   2) Define which bits in DC_TEST_CTL are meaningful in your model
        //      (enable test modes, select index, etc.).
        //   3) Decide if your emulator honors DC_TEST_CTL or treats it as a
        //      no-op (except for logging).
        //   4) Add tracing for test control changes.
        //
        // Example:
        //   cpu.writeIPR_DC_TEST_CTL_Active(id, val);

        Q_UNUSED(val);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MTPR_DC_TEST_CTL_InstructionGrain);

#endif // PAL_MTPR_DC_TEST_CTL_INSTRUCTIONGRAIN_H
