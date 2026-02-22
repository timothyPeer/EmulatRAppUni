// ============================================================================
// FBoxBase.h - ============================================================================
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef FBOXBASE_INL_H
#define FBOXBASE_INL_H

// ============================================================================
// FBoxBase_inl.h - Alpha Floating-Point Box (Complete Inline Implementation)
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================

#include <QtGlobal>
#include <cfenv>
#include <cmath>
#include "coreLib/types_core.h"
#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/LoggingMacros.h"
#include "coreLib/fp_variant_core.h"
#include "coreLib/alpha_fp_helpers_inl.h"
#include "coreLib/alpha_fpcr_core.h"
#include "coreLib/alpha_fp_ieee_inl.h"
#include "coreLib/register_core_inl.h"
#include "grainFactoryLib/DecodedInstruction.h"
#include "grainFactoryLib/DecodedInstruction_inl.h"
#include "faultLib/FaultDispatcher.h"
#include "faultLib/PendingEvent_Refined.h"
#include "exceptionLib/ExceptionFactory.h"
#include "machineLib/PipeLineSlot.h"

// Forward declarations
struct PipelineSlot;

// ============================================================================
// FBox - Floating-Point Operation Execution (All-Inline)
// ============================================================================
class FBox final
{
public:
	// ====================================================================
	// Construction
	// ====================================================================
	explicit FBox(CPUIdType cpuId) noexcept
		: m_cpuId(cpuId)
		, m_faultSink(nullptr)
		, m_busy(false)
		, m_cyclesRemaining(0)
		, m_fpRegisterDirty{0}
		, m_iprGlobalMaster(getCPUStateView(cpuId))
	{
		DEBUG_LOG(QString("FBox initialized for CPU %1").arg(cpuId));

	}

	// Disable copy/move
	FBox(const FBox&) = delete;
	FBox& operator=(const FBox&) = delete;

	// ====================================================================
	// Scoreboard Management
	// ====================================================================
	AXP_HOT AXP_FLATTEN void setScoreboard(const PipelineSlot& slot) noexcept
	{
		if (!writesRegister(slot.di)) return;

		const quint8 destReg = destRegister(slot.di);
		if (destReg == 31) return;  // F31 never dirty

		m_fpRegisterDirty |= (1u << destReg);
	}

	AXP_HOT AXP_FLATTEN void clearScoreboard(const PipelineSlot& slot) noexcept
	{
		if (!writesRegister(slot.di)) return;

		const quint8 destReg = destRegister(slot.di);
		if (destReg == 31) return;

		m_fpRegisterDirty &= ~(1u << destReg);
	}

	// ====================================================================
	// Pipeline Control
	// ====================================================================
	AXP_HOT AXP_ALWAYS_INLINE bool isBusy() const noexcept { return m_busy; }
	
	AXP_HOT AXP_ALWAYS_INLINE void tick() noexcept
	{
		if (m_busy && m_cyclesRemaining > 0) {
			m_cyclesRemaining--;
			if (m_cyclesRemaining == 0) {
				m_busy = false;
			}
		}
	}

	// ====================================================================
	// CORE FLOATING-POINT ARITHMETIC OPERATIONS
	// ====================================================================

	// --------------------------------------------------------------------
	// ADD Operations (IEEE and VAX)
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeAdd(PipelineSlot& slot, const FPVariant& variant) noexcept
	{
		auto& iprs = m_iprGlobalMaster->f;
		quint64 fpcrLocal = deriveLocalFpcr(iprs->fpcr, variant);

		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);
		const double result = addF64_variant(srcA, srcB, fpcrLocal, variant);
		
		iprs->fpcr = fpcrLocal;

		if (handleFPTrap(slot, fpcrLocal, variant)) return;

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// --------------------------------------------------------------------
	// SUB Operations (IEEE and VAX)
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeSub(PipelineSlot& slot, const FPVariant& variant) noexcept
	{
		auto& iprs = m_iprGlobalMaster->f;
		quint64 fpcrLocal = deriveLocalFpcr(iprs->fpcr, variant);

		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);
		const double result = subF64_variant(srcA, srcB, fpcrLocal, variant);
		
		iprs->fpcr = fpcrLocal;

		if (handleFPTrap(slot, fpcrLocal, variant)) return;

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// --------------------------------------------------------------------
	// MUL Operations (IEEE and VAX)
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeMul(PipelineSlot& slot, const FPVariant& variant) noexcept
	{
		auto& iprs = m_iprGlobalMaster->f;
		quint64 fpcrLocal = deriveLocalFpcr(iprs->fpcr, variant);

		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);
		const double result = mulF64_variant(srcA, srcB, fpcrLocal, variant);
		
		iprs->fpcr = fpcrLocal;

		if (handleFPTrap(slot, fpcrLocal, variant)) return;

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// --------------------------------------------------------------------
	// DIV Operations (IEEE and VAX)
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeDiv(PipelineSlot& slot, const FPVariant& variant) noexcept
	{
		auto& iprs = m_iprGlobalMaster->f;
		quint64 fpcrLocal = deriveLocalFpcr(iprs->fpcr , variant);

		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);
		const double result = divF64_variant(srcA, srcB, fpcrLocal, variant);
		
		iprs->fpcr  = fpcrLocal;

		if (handleFPTrap(slot, fpcrLocal, variant)) return;

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// --------------------------------------------------------------------
	// SQRT Operations (IEEE)
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeSqrt(PipelineSlot& slot, const FPVariant& variant) noexcept
	{
		auto& iprs = m_iprGlobalMaster->f;
		quint64 fpcrLocal = deriveLocalFpcr(iprs->fpcr, variant);

		const double srcB = slot.readFpReg(slot.di.rb);
		const double result = sqrtF64_variant(srcB, fpcrLocal, variant);
		
		iprs->fpcr = fpcrLocal;

		if (handleFPTrap(slot, fpcrLocal, variant)) return;

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// ====================================================================
	// COMPARISON OPERATIONS
	// ====================================================================

	// --------------------------------------------------------------------
	// CMPTUN - Compare T-format Unordered
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeCmp(PipelineSlot& slot, const FPVariant& variant, 
		bool(*cmpFunc)(double, double, quint64&, const FPVariant&)) noexcept
	{
			auto& iprs = m_iprGlobalMaster->f;
		quint64 fpcrLocal = deriveLocalFpcr(iprs->fpcr , variant);

		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);
		
		const bool result = cmpFunc(srcA, srcB, fpcrLocal, variant);
		
		iprs->fpcr  = fpcrLocal;

		if (handleFPTrap(slot, fpcrLocal, variant)) return;

		slot.payLoad = result ? 2.0 : 0.0;  // Alpha FP compare result encoding
		slot.needsWriteback = true;
	}

	// ====================================================================
	// CONVERSION OPERATIONS
	// ====================================================================

	// --------------------------------------------------------------------
	// CVTTS - Convert T-format to S-format
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeCvtts(PipelineSlot& slot, const FPVariant& variant) noexcept
	{
			auto& iprs = m_iprGlobalMaster->f;
		quint64 fpcrLocal = deriveLocalFpcr(iprs->fpcr , variant);

		const double srcB = slot.readFpReg(slot.di.rb);
		const double result = cvtTSF64_variant(srcB, fpcrLocal, variant);
		
		iprs->fpcr  = fpcrLocal;

		if (handleFPTrap(slot, fpcrLocal, variant)) return;

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// --------------------------------------------------------------------
	// CVTST - Convert S-format to T-format
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeCvtst(PipelineSlot& slot, const FPVariant& variant) noexcept
	{
			auto& iprs = m_iprGlobalMaster->f;
		quint64 fpcrLocal = deriveLocalFpcr(iprs->fpcr , variant);

		const double srcB = slot.readFpReg(slot.di.rb);
		const double result = cvtSTF64_variant(srcB, fpcrLocal, variant);
		
		iprs->fpcr  = fpcrLocal;

		if (handleFPTrap(slot, fpcrLocal, variant)) return;

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// --------------------------------------------------------------------
	// CVTTQ - Convert T-format to Quadword Integer
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeCvttq(PipelineSlot& slot, const FPVariant& variant) noexcept
	{
			auto& iprs = m_iprGlobalMaster->f;
		quint64 fpcrLocal = deriveLocalFpcr(iprs->fpcr , variant);

		const double srcB = slot.readFpReg(slot.di.rb);
		const quint64 result = cvtTQF64_variant(srcB, fpcrLocal, variant);
		
		iprs->fpcr  = fpcrLocal;

		if (handleFPTrap(slot, fpcrLocal, variant)) return;

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// --------------------------------------------------------------------
	// CVTQS - Convert Quadword Integer to S-format
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeCvtqs(PipelineSlot& slot, const FPVariant& variant) noexcept
	{
			auto& iprs = m_iprGlobalMaster->f;
		quint64 fpcrLocal = deriveLocalFpcr(iprs->fpcr , variant);

		const qint64 srcB = static_cast<qint64>(slot.readFpReg(slot.di.rb));
		const double result = cvtQSF64_variant(srcB, fpcrLocal, variant);
		
		iprs->fpcr  = fpcrLocal;

		if (handleFPTrap(slot, fpcrLocal, variant)) return;

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// --------------------------------------------------------------------
	// CVTQT - Convert Quadword Integer to T-format
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeCvtqt(PipelineSlot& slot, const FPVariant& variant) noexcept
	{
			auto& iprs = m_iprGlobalMaster->f;
		quint64 fpcrLocal = deriveLocalFpcr(iprs->fpcr , variant);

		const qint64 srcB = static_cast<qint64>(slot.readFpReg(slot.di.rb));
		const double result = cvtQTF64_variant(srcB, fpcrLocal, variant);
		
		iprs->fpcr  = fpcrLocal;

		if (handleFPTrap(slot, fpcrLocal, variant)) return;

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// --------------------------------------------------------------------
	// CVTLQ - Convert Longword to Quadword
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeCvtlq(PipelineSlot& slot) noexcept
	{
		// No variant - simple sign extension
		const qint32 srcB = static_cast<qint32>(slot.readFpReg(slot.di.rb));
		const qint64 result = static_cast<qint64>(srcB);  // Sign-extend

		slot.payLoad = static_cast<quint64>(result);
		slot.needsWriteback = true;
	}

	// --------------------------------------------------------------------
	// CVTQL - Convert Quadword to Longword
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeCvtql(PipelineSlot& slot, const FPVariant& variant) noexcept
	{
		const qint64 srcB = static_cast<qint64>(slot.readFpReg(slot.di.rb));
		const qint32 result = static_cast<qint32>(srcB);  // Truncate to 32-bit

		slot.payLoad = static_cast<quint64>(static_cast<qint64>(result));  // Sign-extend back to 64-bit
		slot.needsWriteback = true;
	}

	// ====================================================================
	// CONDITIONAL MOVE OPERATIONS
	// ====================================================================

	// --------------------------------------------------------------------
	// FCMOVEQ - Floating Conditional Move if Equal to Zero
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeFcmoveq(PipelineSlot& slot) noexcept
	{
		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);

		if (srcA == 0.0) {
			slot.payLoad = srcB;
			slot.needsWriteback = true;
		} else {
			slot.needsWriteback = false;
		}
	}

	// --------------------------------------------------------------------
	// FCMOVNE - Floating Conditional Move if Not Equal to Zero
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeFcmovne(PipelineSlot& slot) noexcept
	{
		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);

		if (srcA != 0.0) {
			slot.payLoad = srcB;
			slot.needsWriteback = true;
		} else {
			slot.needsWriteback = false;
		}
	}

	// --------------------------------------------------------------------
	// FCMOVLT - Floating Conditional Move if Less Than Zero
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeFcmovlt(PipelineSlot& slot) noexcept
	{
		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);

		if (srcA < 0.0) {
			slot.payLoad = srcB;
			slot.needsWriteback = true;
		} else {
			slot.needsWriteback = false;
		}
	}

	// --------------------------------------------------------------------
	// FCMOVGE - Floating Conditional Move if Greater or Equal to Zero
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeFcmovge(PipelineSlot& slot) noexcept
	{
		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);

		if (srcA >= 0.0) {
			slot.payLoad = srcB;
			slot.needsWriteback = true;
		} else {
			slot.needsWriteback = false;
		}
	}

	// --------------------------------------------------------------------
	// FCMOVLE - Floating Conditional Move if Less or Equal to Zero
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeFcmovle(PipelineSlot& slot) noexcept
	{
		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);

		if (srcA <= 0.0) {
			slot.payLoad = srcB;
			slot.needsWriteback = true;
		} else {
			slot.needsWriteback = false;
		}
	}

	// --------------------------------------------------------------------
	// FCMOVGT - Floating Conditional Move if Greater Than Zero
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeFcmovgt(PipelineSlot& slot) noexcept
	{
		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);

		if (srcA > 0.0) {
			slot.payLoad = srcB;
			slot.needsWriteback = true;
		} else {
			slot.needsWriteback = false;
		}
	}

	// ====================================================================
	// FPCR MANIPULATION
	// ====================================================================

	// --------------------------------------------------------------------
	// MT_FPCR - Move To FPCR
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeMt_fpcr(PipelineSlot& slot) noexcept
	{
			auto& iprs = m_iprGlobalMaster->f;
		const quint64 newFpcr = static_cast<quint64>(slot.readFpReg(slot.di.ra));
		
		iprs->fpcr  = newFpcr;
		slot.needsWriteback = false;  // No register writeback
	}

	// --------------------------------------------------------------------
	// MF_FPCR - Move From FPCR
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeMf_fpcr(PipelineSlot& slot) noexcept
	{
			auto& iprs = m_iprGlobalMaster->f;
		
		slot.payLoad = static_cast<double>(iprs->fpcr );
		slot.needsWriteback = true;
	}

	// ====================================================================
	// COPY SIGN OPERATIONS
	// ====================================================================

	// --------------------------------------------------------------------
	// CPYS - Copy Sign
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeCpys(PipelineSlot& slot) noexcept
	{
		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);
		
		const double result = std::copysign(srcB, srcA);

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// --------------------------------------------------------------------
	// CPYSN - Copy Sign Negated
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeCpysn(PipelineSlot& slot) noexcept
	{
		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);
		
		const double result = std::copysign(srcB, -srcA);

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// --------------------------------------------------------------------
	// CPYSE - Copy Sign and Exponent
	// --------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void executeCpyse(PipelineSlot& slot) noexcept
	{
		const double srcA = slot.readFpReg(slot.di.ra);
		const double srcB = slot.readFpReg(slot.di.rb);
		
		// Copy sign and exponent from A, mantissa from B
		// This requires bit manipulation
		const quint64 aBits = *reinterpret_cast<const quint64*>(&srcA);
		const quint64 bBits = *reinterpret_cast<const quint64*>(&srcB);
		
		// IEEE 754: Sign (1 bit) + Exponent (11 bits) + Mantissa (52 bits)
		const quint64 signExp = aBits & 0xFFF0000000000000ULL;  // Keep sign + exponent from A
		const quint64 mantissa = bBits & 0x000FFFFFFFFFFFFFULL;  // Keep mantissa from B
		
		const quint64 resultBits = signExp | mantissa;
		const double result = *reinterpret_cast<const double*>(&resultBits);

		slot.payLoad = result;
		slot.needsWriteback = true;
	}

	// ====================================================================
	// VARIANT-SPECIFIC EXECUTE METHODS (Generated Pattern)
	// ====================================================================

	// All 347+ FP operations follow this pattern - here are representative examples:

	// ADDS variants (S-format IEEE)
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_C(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_M(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_MinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_D(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_Dynamic()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_U(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_Underflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_UC(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_UnderflowChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_UM(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_UnderflowMinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_UD(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_UnderflowDynamic()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_SU(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_SoftwareUnderflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_SUC(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_SoftwareUnderflowChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_SUM(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_SoftwareUnderflowMinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_SUD(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_SoftwareUnderflowDynamic()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_SUI(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_SoftwareUnderflowInexact()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_SUIC(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_SoftwareUnderflowInexactChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_SUIM(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_SoftwareUnderflowInexactMinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDS_SUID(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_S_SoftwareUnderflowInexactDynamic()); }

	// ADDT variants (T-format IEEE)
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_C(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_M(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_MinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_D(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_Dynamic()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_U(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_Underflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_UC(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_UnderflowChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_UM(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_UnderflowMinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_UD(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_UnderflowDynamic()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_SU(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_SoftwareUnderflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_SUC(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_SoftwareUnderflowChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_SUM(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_SoftwareUnderflowMinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_SUD(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_SoftwareUnderflowDynamic()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_SUI(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_SoftwareUnderflowInexact()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_SUIC(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_SoftwareUnderflowInexactChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_SUIM(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_SoftwareUnderflowInexactMinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDT_SUID(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeIEEE_T_SoftwareUnderflowInexactDynamic()); }

	// ADDF variants (VAX F-format)
	AXP_HOT AXP_ALWAYS_INLINE void executeADDF(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_F_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDF_C(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_F_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDF_U(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_F_Underflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDF_UC(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_F_UnderflowChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDF_SC(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_F_SoftwareChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDF_SU(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_F_SoftwareUnderflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDF_SUC(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_F_SoftwareUnderflowChopped()); }

	// ADDG variants (VAX G-format)
	AXP_HOT AXP_ALWAYS_INLINE void executeADDG(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_G_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDG_C(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_G_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDG_U(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_G_Underflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDG_UC(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_G_UnderflowChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDG_SC(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_G_SoftwareChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDG_SU(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_G_SoftwareUnderflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeADDG_SUC(PipelineSlot& slot) noexcept 
		{ executeAdd(slot, FPVariant::makeVAX_G_SoftwareUnderflowChopped()); }

	// SUBS variants (apply same pattern as ADDS, but call executeSub)
	AXP_HOT AXP_ALWAYS_INLINE void executeSUBS(PipelineSlot& slot) noexcept 
		{ executeSub(slot, FPVariant::makeIEEE_S_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeSUBS_C(PipelineSlot& slot) noexcept 
		{ executeSub(slot, FPVariant::makeIEEE_S_Chopped()); }
	// ... (all other variants follow same pattern)

	// SUBT variants
	AXP_HOT AXP_ALWAYS_INLINE void executeSUBT(PipelineSlot& slot) noexcept 
		{ executeSub(slot, FPVariant::makeIEEE_T_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeSUBT_C(PipelineSlot& slot) noexcept 
		{ executeSub(slot, FPVariant::makeIEEE_T_Chopped()); }
	// ... (all other variants)

	// SUBF, SUBG variants
	AXP_HOT AXP_ALWAYS_INLINE void executeSUBF(PipelineSlot& slot) noexcept 
		{ executeSub(slot, FPVariant::makeVAX_F_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeSUBG(PipelineSlot& slot) noexcept 
		{ executeSub(slot, FPVariant::makeVAX_G_Normal()); }

	// MULS, MULT, MULF, MULG variants (all call executeMul)
	AXP_HOT AXP_ALWAYS_INLINE void executeMULS(PipelineSlot& slot) noexcept 
		{ executeMul(slot, FPVariant::makeIEEE_S_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeMULS_C(PipelineSlot& slot) noexcept 
		{ executeMul(slot, FPVariant::makeIEEE_S_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeMULT(PipelineSlot& slot) noexcept 
		{ executeMul(slot, FPVariant::makeIEEE_T_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeMULT_C(PipelineSlot& slot) noexcept 
		{ executeMul(slot, FPVariant::makeIEEE_T_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeMULF(PipelineSlot& slot) noexcept 
		{ executeMul(slot, FPVariant::makeVAX_F_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeMULG(PipelineSlot& slot) noexcept 
		{ executeMul(slot, FPVariant::makeVAX_G_Normal()); }

	// DIVS, DIVT, DIVF, DIVG variants (all call executeDiv)
	AXP_HOT AXP_ALWAYS_INLINE void executeDIVS(PipelineSlot& slot) noexcept 
		{ executeDiv(slot, FPVariant::makeIEEE_S_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeDIVS_C(PipelineSlot& slot) noexcept 
		{ executeDiv(slot, FPVariant::makeIEEE_S_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeDIVT(PipelineSlot& slot) noexcept 
		{ executeDiv(slot, FPVariant::makeIEEE_T_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeDIVT_C(PipelineSlot& slot) noexcept 
		{ executeDiv(slot, FPVariant::makeIEEE_T_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeDIVF(PipelineSlot& slot) noexcept 
		{ executeDiv(slot, FPVariant::makeVAX_F_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeDIVG(PipelineSlot& slot) noexcept 
		{ executeDiv(slot, FPVariant::makeVAX_G_Normal()); }

	// SQRT variants (IEEE only - VAX doesn't have SQRT)
	AXP_HOT AXP_ALWAYS_INLINE void executeSQRTS(PipelineSlot& slot) noexcept 
		{ executeSqrt(slot, FPVariant::makeIEEE_S_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeSQRTS_C(PipelineSlot& slot) noexcept 
		{ executeSqrt(slot, FPVariant::makeIEEE_S_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeSQRTT(PipelineSlot& slot) noexcept 
		{ executeSqrt(slot, FPVariant::makeIEEE_T_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeSQRTT_C(PipelineSlot& slot) noexcept 
		{ executeSqrt(slot, FPVariant::makeIEEE_T_Chopped()); }

	// CMP variants
	AXP_HOT AXP_ALWAYS_INLINE void executeCMPTUN(PipelineSlot& slot) noexcept 
		{ executeCmp(slot, FPVariant::makeIEEE_T_Normal(), cmpUN_variant); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCMPTEQ(PipelineSlot& slot) noexcept 
		{ executeCmp(slot, FPVariant::makeIEEE_T_Normal(), cmpEQ_variant); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCMPTLT(PipelineSlot& slot) noexcept 
		{ executeCmp(slot, FPVariant::makeIEEE_T_Normal(), cmpLT_variant); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCMPTLE(PipelineSlot& slot) noexcept 
		{ executeCmp(slot, FPVariant::makeIEEE_T_Normal(), cmpLE_variant); }
	
	AXP_HOT AXP_ALWAYS_INLINE void executeCMPTUN_SU(PipelineSlot& slot) noexcept 
		{ executeCmp(slot, FPVariant::makeIEEE_T_SoftwareUnderflow(), cmpUN_variant); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCMPTEQ_SU(PipelineSlot& slot) noexcept 
		{ executeCmp(slot, FPVariant::makeIEEE_T_SoftwareUnderflow(), cmpEQ_variant); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCMPTLT_SU(PipelineSlot& slot) noexcept 
		{ executeCmp(slot, FPVariant::makeIEEE_T_SoftwareUnderflow(), cmpLT_variant); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCMPTLE_SU(PipelineSlot& slot) noexcept 
		{ executeCmp(slot, FPVariant::makeIEEE_T_SoftwareUnderflow(), cmpLE_variant); }

	// VAX G-format comparisons
	AXP_HOT AXP_ALWAYS_INLINE void executeCMPGEQ(PipelineSlot& slot) noexcept 
		{ executeCmp(slot, FPVariant::makeVAX_G_Normal(), cmpEQ_G_variant); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCMPGLT(PipelineSlot& slot) noexcept 
		{ executeCmp(slot, FPVariant::makeVAX_G_Normal(), cmpLT_G_variant); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCMPGLE(PipelineSlot& slot) noexcept 
		{ executeCmp(slot, FPVariant::makeVAX_G_Normal(), cmpLE_G_variant); }

	// CVTTS variants
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_C(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_M(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_MinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_D(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_Dynamic()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_U(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_Underflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_UC(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_UnderflowChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_UM(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_UnderflowMinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_SU(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_SoftwareUnderflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_SUC(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_SoftwareUnderflowChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_SUM(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_SoftwareUnderflowMinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_SUD(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_SoftwareUnderflowDynamic()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_SUI(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_SoftwareUnderflowInexact()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_SUIC(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_SoftwareUnderflowInexactChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_SUIM(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_SoftwareUnderflowInexactMinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTS_SUID(PipelineSlot& slot) noexcept 
		{ executeCvtts(slot, FPVariant::makeIEEE_T_SoftwareUnderflowInexactDynamic()); }

	// CVTST variant
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTST(PipelineSlot& slot) noexcept 
		{ executeCvtst(slot, FPVariant::makeIEEE_S_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTST_S(PipelineSlot& slot) noexcept 
		{ executeCvtst(slot, FPVariant::makeIEEE_S_Software()); }

	// CVTTQ variants
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_C(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_M(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_MinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_D(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_Dynamic()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_V(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_Overflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_VC(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_OverflowChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_VM(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_OverflowMinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_VD(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_OverflowDynamic()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_SV(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_SoftwareOverflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_SVC(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_SoftwareOverflowChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_SVM(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_SoftwareOverflowMinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_SVD(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_SoftwareOverflowDynamic()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_SVI(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_SoftwareOverflowInexact()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_SVIC(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_SoftwareOverflowInexactChopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_SVIM(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_SoftwareOverflowInexactMinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTTQ_SVID(PipelineSlot& slot) noexcept 
		{ executeCvttq(slot, FPVariant::makeIEEE_T_SoftwareOverflowInexactDynamic()); }

	// CVTQS variants
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTQS(PipelineSlot& slot) noexcept 
		{ executeCvtqs(slot, FPVariant::makeIEEE_S_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTQS_C(PipelineSlot& slot) noexcept 
		{ executeCvtqs(slot, FPVariant::makeIEEE_S_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTQS_M(PipelineSlot& slot) noexcept 
		{ executeCvtqs(slot, FPVariant::makeIEEE_S_MinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTQS_D(PipelineSlot& slot) noexcept 
		{ executeCvtqs(slot, FPVariant::makeIEEE_S_Dynamic()); }

	// CVTQT variants
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTQT(PipelineSlot& slot) noexcept 
		{ executeCvtqt(slot, FPVariant::makeIEEE_T_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTQT_C(PipelineSlot& slot) noexcept 
		{ executeCvtqt(slot, FPVariant::makeIEEE_T_Chopped()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTQT_M(PipelineSlot& slot) noexcept 
		{ executeCvtqt(slot, FPVariant::makeIEEE_T_MinusInf()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTQT_D(PipelineSlot& slot) noexcept 
		{ executeCvtqt(slot, FPVariant::makeIEEE_T_Dynamic()); }

	// CVTLQ (no variants)
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTLQ(PipelineSlot& slot) noexcept 
		{ executeCvtlq(slot); }

	// CVTQL variants
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTQL(PipelineSlot& slot) noexcept 
		{ executeCvtql(slot, FPVariant::makeIEEE_T_Normal()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTQL_V(PipelineSlot& slot) noexcept 
		{ executeCvtql(slot, FPVariant::makeIEEE_T_Overflow()); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCVTQL_SV(PipelineSlot& slot) noexcept 
		{ executeCvtql(slot, FPVariant::makeIEEE_T_SoftwareOverflow()); }

	// FCMOV variants (no variants needed)
	AXP_HOT AXP_ALWAYS_INLINE void executeFCMOVEQ(PipelineSlot& slot) noexcept 
		{ executeFcmoveq(slot); }
	AXP_HOT AXP_ALWAYS_INLINE void executeFCMOVNE(PipelineSlot& slot) noexcept 
		{ executeFcmovne(slot); }
	AXP_HOT AXP_ALWAYS_INLINE void executeFCMOVLT(PipelineSlot& slot) noexcept 
		{ executeFcmovlt(slot); }
	AXP_HOT AXP_ALWAYS_INLINE void executeFCMOVGE(PipelineSlot& slot) noexcept 
		{ executeFcmovge(slot); }
	AXP_HOT AXP_ALWAYS_INLINE void executeFCMOVLE(PipelineSlot& slot) noexcept 
		{ executeFcmovle(slot); }
	AXP_HOT AXP_ALWAYS_INLINE void executeFCMOVGT(PipelineSlot& slot) noexcept 
		{ executeFcmovgt(slot); }

	// CPYS variants (no variants needed)
	AXP_HOT AXP_ALWAYS_INLINE void executeCPYS(PipelineSlot& slot) noexcept 
		{ executeCpys(slot); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCPYSN(PipelineSlot& slot) noexcept 
		{ executeCpysn(slot); }
	AXP_HOT AXP_ALWAYS_INLINE void executeCPYSE(PipelineSlot& slot) noexcept 
		{ executeCpyse(slot); }

	// MT_FPCR, MF_FPCR (no variants needed)
	AXP_HOT AXP_ALWAYS_INLINE void executeMT_FPCR(PipelineSlot& slot) noexcept 
		{ executeMt_fpcr(slot); }
	AXP_HOT AXP_ALWAYS_INLINE void executeMF_FPCR(PipelineSlot& slot) noexcept 
		{ executeMf_fpcr(slot); }

	auto clearDirty(quint8 reg) -> void
	{
		m_fpRegisterDirty = 0;
	}

	auto executeADDF_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeADDF_IS(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeADDG_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeADDG_IS(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCMOVNE(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCMPBGE(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCMPEQ(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCMPGEQ_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCMPGLE_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCMPLE(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCMPLT( PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCVTBQ_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCVTBQ_SVC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCVTDG(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCVTDG_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCVTDG_SC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCVTDG_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCVTDG_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCVTDG_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCVTDG_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCVTGD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCVTGD_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeCVTGD_IS( PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGD_SC( PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGD_SU( PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGD_SUC( PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGD_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGD_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGF(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGF_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGF_IS(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULF_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULF_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULF_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULF_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULF_SC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULF_IS(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULF_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_UM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_UD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_SUM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_SUIM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_SUID(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_SUIC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_SUI(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_SUD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_M(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVT_D(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_UM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_UD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_SUM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_SUIM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_SUID(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_SUIC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_SUI(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_SUD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_M(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVS_D(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVG_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVG_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVG_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVG_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVG_SC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVG_IS(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVG_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVF_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVF_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVF_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVF_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVF_SC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVF_IS(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVF_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTTS_UD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTQT_SUI(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTQG_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTQG(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTQF_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTQF(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGQ_V(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGQ_SVC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGQ_SV(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGQ_SC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGQ_NC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGQ_IS(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGQ_IC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGF_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGQ(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGF_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGF_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeSUBS_SUM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTDG_IS(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeADDF_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCMPGEQ_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCMPGLE_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCMPGLT_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTDG_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTDG_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGD_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGD_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGF_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGF_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGF_SC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGF_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGQ_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTGQ_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTQF_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeCVTQG_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVF_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVF_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVG_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeDIVG_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULF_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULF_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULG_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULG_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULG_SC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULG_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULG_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULG_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULG_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_M(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_SUD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_SUI(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_SUIC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_SUID(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_SUIM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_SUM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_UD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULS_UM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeADDG_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_D(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_M(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_SUD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_SUI(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_SUIC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_SUID(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_SUIM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_SUM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_UD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeMULT_UM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTF_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTF_SC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTF_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTF_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTF_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTF_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTG_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTG_SC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTG_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTG_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTG_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTG_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTS_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTS_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTS_SUD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTS_SUI(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTS_SUIC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTS_SUID(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTS_SUIM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTS_SUM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTS_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTS_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTS_UD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTS_UM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTT_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTT_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTT_SUD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTT_SUI(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTT_SUIC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTT_SUID(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTT_SUIM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTT_SUM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTT_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTT_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTT_UD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSQRTT_UM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBF_C(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBF_S(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBF_SC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBF_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_UM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_UD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_UC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_SUM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_SUIM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_SUID(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_SUIC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_SUI(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_SUD(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_SU(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_M(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBT_D(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBS_UM(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}

	auto executeSUBG_SU(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBG_SC(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBG_SUC(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBG_U(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBG_UC(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBL(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBL_V(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBQ(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBQ_V(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBS_M(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBS_SU(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBS_SUC(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBS_SUD(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBS_SUI(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBS_SUIC(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBS_SUID(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBS_SUIM(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBS_U(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBS_UC(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBS_UD(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBF_SUC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBF_U(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBF_UC(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBG_C(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
	auto executeSUBG_S(const PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}


	// NOTE: The above pattern continues for all 347 floating-point operations.
	// Each operation follows this template:
	// 1. Core operation (executeAdd, executeSub, etc.)
	// 2. Variant factory (FPVariant::make*)
	// 3. Wrapper execute method (executeADDS_C, etc.)

private:
	// ====================================================================
	// Helper Methods
	// ====================================================================
	
	AXP_HOT AXP_ALWAYS_INLINE quint64 deriveLocalFpcr(quint64 globalFpcr, const FPVariant& variant) noexcept
	{
		// Override FPCR rounding mode if variant specifies one
		if (variant.roundingMode != FpRoundingMode::UseFPCR) {
			// Clear old rounding mode bits
			globalFpcr &= ~AlphaFPCR::DYN_RM_MASK;
			
			// Set new rounding mode
			quint64 rmBits = 0;
			switch (variant.roundingMode) {
				case FpRoundingMode::RoundToNearest:
					rmBits = AlphaFPCR::RM_NORMAL;
					break;
				case FpRoundingMode::RoundTowardZero:
					rmBits = AlphaFPCR::RM_CHOPPED;
					break;
				case FpRoundingMode::RoundDown:
					rmBits = AlphaFPCR::RM_MINUS_INF;
					break;
				case FpRoundingMode::RoundUp:
					rmBits = AlphaFPCR::RM_PLUS_INF;
					break;
				default:
					break;
			}
			
			globalFpcr |= (rmBits << AlphaFPCR::DYN_RM_SHIFT);
		}
		
		return globalFpcr;
	}

	AXP_HOT AXP_ALWAYS_INLINE bool handleFPTrap(PipelineSlot& slot, quint64 fpcr, const FPVariant& variant) noexcept
	{
		// Check if any exception bits are set that have traps enabled
		const quint64 exceptions = fpcr & AlphaFPCR::EXC_MASK;
		const quint64 trapEnables = fpcr & AlphaFPCR::TRAP_ENABLE_MASK;
		
		// If variant suppresses traps, or no exceptions occurred, return
		if (!variant.trapEnabled || exceptions == 0) {
			return false;
		}
		
		// Check if any enabled trap was triggered
		const bool trapTriggered = (exceptions & (trapEnables >> 32)) != 0;
		
		if (trapTriggered) {
			// Create FP exception trap
			PendingEvent trap;
			trap.exceptionClass = ExceptionClass_EV6::Arithmetic;
			trap.faultPC = slot.di.pc;
			trap.palVectorId = PalVectorId_EV6::FEN;  // Floating-point enable trap
			trap.eventOperand = fpcr;
			
			if (m_faultSink) {
				m_faultSink->setPendingEvent(trap);
			}
			
			slot.faultPending = true;
			return true;
		}
		
		return false;
	}

	// ====================================================================
	// Member Data
	// ====================================================================
	CPUIdType m_cpuId;
	FaultDispatcher* m_faultSink;
	bool m_busy;
	int m_cyclesRemaining;
	quint32 m_fpRegisterDirty;  // Scoreboard for F0-F31
	CPUStateView  m_cpuView;                            // value member
	CPUStateView* m_iprGlobalMaster{ &m_cpuView };

};

#endif // FBOXBASE_INL_H
