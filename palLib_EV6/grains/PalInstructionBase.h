#ifndef PALINSTRUCTIONBASE_H
#define PALINSTRUCTIONBASE_H

// ============================================================================
// PalInstructionBase.h
// Alpha AXP PALcode Instruction Base Template
// ============================================================================
//
// Each PAL instruction is encoded as CALL_PAL <function#> with primary opcode
// 0x00 and low 21 bits defining the PAL function number.
//
// This class provides:
//   * opcode()  -> always 0x00
//   * functionCode() -> template param PalFunc
//   * fastExecThunk()
//   * execute() -> calls executePAL()
//   * Inline/hot modifiers (AXP_FLATTEN, AXP_HOT)
//   * Helpers for PC, PS, HWPCB via AlphaProcessorContext::*_Active()
//   * All-ASCII output only
//
// Requirements per project guidelines:
//   1) Extract PC via AlphaProcessorContext::getPC_Active(cpuId)
//   2) Each PAL instruction must implement executePAL()
//   3) Include TODO section in PAL grains
//   4) File names: Pal_<MNEMONIC>_InstructionGrain.h
//   5) REGISTER_GRAIN using __COUNTER__
// ============================================================================

#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/InstructionGrainRegistry.h"
#include "corelib/Axp_attributes_core.h"
#include <QtGlobal>
#include "../../coreLib/HWPCB_helpers_inline.h"
#include "machineLib/PipeLineSlot.h"
#include "grainFactoryLib/executionBoxDecoder_inl.h"

class AlphaProcessorContext;

// ============================================================================
// Template Base
// ============================================================================
template<int PalFunc>
class PalInstructionBase : public InstructionGrain
{
public:
	PalInstructionBase() noexcept = default;
	ExecutionBox box{ ExecutionBox::Unknown };
	// PAL instructions = CALL_PAL (opcode 0x00)
	AXP_FLATTEN quint8 opcode() const override
	{
		return 0x00;
	}

	// Function code is compile-time fixed
	AXP_FLATTEN quint16 functionCode() const override
	{
		return PalFunc;
	}

	QString mnemonic() const override
	{
		return "CALL_PAL";
	}

	GrainType grainType() const override
	{
		return GrainType::Pal;
	}

	GrainPlatform platform() const override
	{
		return GrainPlatform::NONE;
	}


	// Hot path trampoline
// 	AXP_HOT void fastExecThunk(PipelineSlot& slot, 	AlphaProcessorContext& argCpu) {
// 		if (slot.di.grain)
// 			slot.di.grain->execute(slot.di, argCpu);
// 	}

	// Unified entry point
	virtual void execute(PipelineSlot& slot) const noexcept = 0;


protected:

	// ---------------------------------------------------------------------
	// Architectural inline helpers
	// ---------------------------------------------------------------------

// 	// Extract active PC
// 	AXP_FLATTEN quint64 getPC(const AlphaProcessorContext& argCpu) const
// 	{
// 		const int id = argCpu.cpuId();
// 		return getPC_Active(id);
// 	}
// 
// 	// Processor status register (PS)
// 	AXP_FLATTEN quint64 getPS(const AlphaProcessorContext& argCpu) const
// 	{
// 		const int id = argCpu.cpuId();
// 		return getPS_Active(id);
// 	}

// 	Hardware PCB pointer
// 	AXP_FLATTEN quint64 getHWPCB(const AlphaProcessorContext& argCpu) const
// 	{
// 		const int id = argCpu.cpuId();
// 		return getHWPCB_Active(id);
// 	}

// 	// Each PAL instruction must implement this
// 	virtual void executePAL(const DecodedInstruction& di,
// 		AlphaProcessorContext& argCpu) const = 0;
};


// ============================================================================
// IPR Numbers (from Alpha Architecture Reference Manual)
// ============================================================================
enum HW_MFPR_IPR : quint16
{
	IPR_ASN = 0x0006,
	IPR_FEN = 0x000B,
	IPR_IPIR = 0x000D,
	IPR_IPL = 0x000E,
	IPR_MCES = 0x0010,
	IPR_PCBB = 0x0012,
	IPR_PRBR = 0x0013,
	IPR_PTBR = 0x0015,
	IPR_SCBB = 0x0016,
	IPR_SISR = 0x0019,
	IPR_TBCHK = 0x001A,
	IPR_ESP = 0x001E,
	IPR_SSP = 0x0020,
	IPR_USP = 0x0022,
	IPR_VPTB = 0x0029,
	IPR_VIRBND = 0x0030,
	IPR_SYSPTBR = 0x0032,
	IPR_WHAMI = 0x003F,
};


#endif // PALINSTRUCTIONBASE_H
