#ifndef EMULATRAPPUNI_EXCEPTIONLIB_EXCEPTIONSTATEUPDATE_INL_H
#define EMULATRAPPUNI_EXCEPTIONLIB_EXCEPTIONSTATEUPDATE_INL_H

#include <QtGlobal>

#include "machineLib/PipeLineSlot.h"


// ============================================================================
// Exception State Update Helpers
// ============================================================================
// Functions to update IPRs and HWPCB state during exception preparation.
// These are called by preparePendingEventForDelivery().
//

/**
 * @brief Update EXC_SUM register based on exception type
 *
 * EXC_SUM encodes exception-specific information:
 * - Bit 0: TLB miss
 * - Bit 1: Access violation
 * - Bit 2: Unaligned access
 * - Bit 3: D-stream fault
 * - Bit 4: Illegal opcode
 * - Bits [15:8]: FP exception summary (for ARITH)
 *
 * @param cpuId
 * @param iprs IPR storage structure
 * @param ev Pending event
 */
inline void updateExceptionSummary(CPUIdType cpuId, const PendingEvent& ev) noexcept
{
    using EC = ExceptionClass_EV6;
    auto& iprs = globalIPRHotExt(cpuId);
    quint64 excSum = iprs.exc_sum;

    switch (ev.exceptionClass)
    {
    // TB miss events
    case EC::ItbMiss:
    case EC::Dtb_miss_single:
    case EC::Dtb_miss_double_4:
        excSum |= (1ULL << 0);  // TB miss bit
        break;

    // Access violations
    case EC::ItbAcv:
    case EC::Dfault:
        excSum |= (1ULL << 1);  // ACV bit
        break;

    // Unaligned access
    case EC::Unalign:
        excSum |= (1ULL << 2);  // Unaligned bit
        break;

    // Data stream fault
    case EC::DStream:
        excSum |= (1ULL << 3);  // D-stream fault bit
        break;

    // Illegal opcode
    case EC::OpcDec:
        excSum |= (1ULL << 4);  // Opcode error bit
        break;

    // Arithmetic/FP exception
    case EC::Arithmetic:
        // extraInfo contains FP exception summary bits
        excSum |= (ev.extraInfo & 0xFFFFULL);
        break;

    default:
        break;
    }

    iprs.exc_sum = excSum;
}

/**
 * @brief Update MM_STAT register for memory management faults
 *
 * MM_STAT encodes memory fault details:
 * - Bit 0: Write access
 * - Bit 1: Execute access
 * - Bits [7:4]: Fault type code
 * - Bit 8: ITB vs DTB (1=ITB, 0=DTB)
 *
 * @param cpuId
 * @param iprs IPR storage structure
 * @param ev Pending event
 */
inline void updateMemoryManagementStatus(CPUIdType cpuId,  const PendingEvent& ev) noexcept
{
    if (!ev.isMemoryFault())
        return;


	auto& iprs = globalIPRHotExt(cpuId);
    quint64 mmStat = 0;

    // Encode access type
    if (ev.pendingEvent_Info.isWrite)
        mmStat |= (1ULL << 0);  // Write access

    if (ev.pendingEvent_Info.isExecute)
        mmStat |= (1ULL << 1);  // Execute access

    // Encode fault type
    quint64 faultTypeCode = static_cast<quint64>(ev.pendingEvent_Info.faultType);
    mmStat |= (faultTypeCode << 4);

    // Encode TB domain
    if (ev.pendingEvent_Info.isInstruction)
        mmStat |= (1ULL << 8);  // ITB domain

    iprs.mm_stat = mmStat;
}

/**
 * @brief Save fault virtual address to IPRs
 * @param cpuId
 * @param iprs IPR storage structure
 * @param hwpcb HWPCB structure
 * @param faultVA Virtual address of fault
 */
inline void saveFaultVirtualAddress(
	const CPUIdType cpuId,
	quint64 faultVA) noexcept
{
    auto& iprs = globalIPRHotExt(cpuId);
	if (faultVA != 0) {
		iprs.va = faultVA;   // Updated during exception delivery 
        /*setVA_FAULT(ctx.cpuId(), faultVA);*/ // RTI (Return from Interrupt) does NOT restore VA
    }
}
/**
 * @brief Save exception address to IPRs
 * @param iprs IPR storage structure
 * @param hwpcb HWPCB structure
 * @param excAddr PAL entry PC
 */
inline void saveExceptionAddress(
    CPUIdType cpuId,
    quint64 excAddr) noexcept
{

    auto& iprs = globalIPRHotExt(cpuId);
    iprs.exc_addr = excAddr;            // IPR_HOT::exc_addr is used for exception saves only
    setEXC_ADDR_Active(cpuId,excAddr);   // HWBCP:: the context block
}

/**
 * @brief Save current PC/PS to HWPCB before exception delivery
 * @param hwpcb HWPCB structure
 * @param faultPC PC at time of exception
 * @param faultPS PS at time of exception
 */
inline void saveProcessorState(
    CPUIdType cpuId,
    quint64 faultPC,
    quint64 faultPS) noexcept
{
    setPC_Active(cpuId,faultPC);
    setPS_Active(cpuId,faultPS);
}

/**
 * @brief Update exception-specific IPRs (per exception type)
 *
 * This is a comprehensive update that handles all exception-specific
 * IPR state modifications.
 *
 * @param cpuId
 * @param iprs IPR storage structure
 * @param hwpcb HWPCB structure
 * @param ev Pending event
 */
inline void updateExceptionIPRs(  CPUIdType cpuId,
    const PendingEvent& ev) noexcept
{
    // Update EXC_SUM
    updateExceptionSummary(cpuId, ev);
    

    // Update MM_STAT for memory faults
    if (ev.isMemoryFault()) {
        updateMemoryManagementStatus(cpuId, ev);
    }

    // Save fault VA if present
    if (ev.faultVA != 0) {
        saveFaultVirtualAddress(cpuId, ev.faultVA);
    }

    // TODO Machine check specific updates
    if (ev.exceptionClass == ExceptionClass_EV6::MachineCheck) {
        // Update MCES (Machine Check Error Summary)
        // Implementation depends on your MCES structure
        // iprs.mces |= ...;
    }
}

#endif
