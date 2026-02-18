#ifndef Pal_dtb_miss_handler_ev6_h__
#define Pal_dtb_miss_handler_ev6_h__

#include "../pteLib/walkPageTable_EV6.h"
#include "../pteLib/Ev6SiliconTLB_Singleton.h"
#include "../coreLib/CurrentCpuTls.h"
#include "../memoryLib/GuestMemory.h"
#include "../cpuCoreLib/AlphaProcessorContext.h"
#include "../faultLib/IFaultSink.h"
#include "../coreLib/enum_header.h"
#include "../coreLib/types_core.h"
#include "../pteLib/alpha_pte_core.h"
#include "../exceptionLib/ExceptionFactory.h"
#include "../configLib/global_EmulatorSettings.h"
#include "../SmpManager.h"
#include "../pteLib/global_Ev6TLB_Singleton.h"

// ============================================================================
// Page Table Walk Memory Reader
// ============================================================================
inline quint64 readPageTableEntry(GM_ALPHA::GuestMemory& memory, quint64 physicalAddr) noexcept
{
	quint64 data = 0;
	MEM_STATUS status = memory.read64(physicalAddr, data);

	if (status != MEM_STATUS::Ok) {
		WARN_LOG(QString("Page table walk: read failed at PA 0x%1 (status=%2)")
			.arg(physicalAddr, 16, 16, QChar('0'))
			.arg(static_cast<int>(status)));
		return 0;
	}

	return data;
}

// ============================================================================
// PAL DTB_MISS Handler - EV6 Implementation
// ============================================================================
class PAL_DTB_MISS_Handler_EV6 final  // x Added final
{
public:
	static void handle(
		CPUIdType cpuId,
		AlphaProcessorContext& ctx,
		IFaultSink& faultSink,
		GM_ALPHA::GuestMemory& memory) noexcept
	{
		// ----------------------------------------------------------------
		// 1. Extract fault information
		// ----------------------------------------------------------------
		const quint64 faultVA = ctx.readIntReg(16);   // a0 = Faulting VA
		const quint64 mmcsr = ctx.readIntReg(17);     // a1 = MMCSR
		const quint64 faultPC = ctx.readIntReg(18);   // a2 = Faulting PC

		// Decode MMCSR to determine access type
		const bool isWrite = (mmcsr & (1 << 4)) != 0;
		const AccessKind access = isWrite ? AccessKind::WRITE : AccessKind::READ;

		const ASNType asn = getCurrentASN(cpuId);
		const quint8 cm = getCM_Active(cpuId);
		const Mode_Privilege mode = (cm == 0) ? Mode_Privilege::Kernel
			: Mode_Privilege::User;

		// ----------------------------------------------------------------
		// 2. Get PTBR
		// ----------------------------------------------------------------
		const quint64 ptbr = getPTBR_Active(cpuId);

		// ----------------------------------------------------------------
		// 3. Perform page table walk
		// ----------------------------------------------------------------
		WalkResultEV6 walkResult = walkPageTable_EV6(
			faultVA,
			ptbr,
			mode,
			access,  // x Data access (READ or WRITE)
			[&memory](quint64 pa) { return readPageTableEntry(memory, pa); }
		);

		// ----------------------------------------------------------------
		// 4. Handle walk results
		// ----------------------------------------------------------------
		if (!walkResult.success) {
			handleWalkFailure(cpuId, faultVA, faultPC, walkResult.fault, isWrite, faultSink);
			return;
		}

		// ----------------------------------------------------------------
		// 5. Validate PTE
		// ----------------------------------------------------------------
		if (!validatePTE(walkResult.pte, mode, access)) {  // x Full validation
			PendingEvent ev = makeDTBAccessViolationEvent(cpuId, faultVA, isWrite);  // x DTB fault
			faultSink.setPendingEvent(ev);
			return;
		}

		// ----------------------------------------------------------------
		// 6. Insert PTE into TLB
		// ----------------------------------------------------------------
		AlphaPTE pte{ walkResult.pte };

		bool insertSuccess = globalSPAM(cpuId).tlbInsert(
			cpuId,
			Realm::D,  // x Data TLB
			faultVA,
			asn,
			pte
		);

		if (!insertSuccess) {
			PendingEvent ev = makeMachineCheckEvent(
				cpuId,
				MachineCheckReason::TLB_INSERTION_FAILURE,
				faultPC
			);
			faultSink.setPendingEvent(ev);
			return;
		}

		// ----------------------------------------------------------------
		// 7. TLB Shootdown
		// ----------------------------------------------------------------
		if (isMultiCPU()) {
			sendTLBShootdown(cpuId, faultVA, asn, Realm::D);  // x Data realm
		}
	}

	static bool isMultiCPU() noexcept {
		return global_EmulatorSettings().podData.system.processorCount > 1;
	}

	static void sendTLBShootdown(
		CPUIdType sourceCpu,
		quint64 va,
		ASNType asn,
		Realm realm) noexcept
	{
		auto& smpMgr = SMPManager::instance();

		quint32 vaHigh = static_cast<quint32>(va >> 32);
		quint32 vaLow = static_cast<quint32>(va & 0xFFFFFFFF);
		quint64 asnRealm = (static_cast<quint64>(asn) << 32) |
			(static_cast<quint64>(realm) & 0xFF);

		quint16 successCount = smpMgr.broadcastIPI(
			sourceCpu,
			IPIQueue::MessageType::TLB_SHOOTDOWN,
			vaHigh,
			vaLow,
			asnRealm
		);

		if (successCount == 0 && smpMgr.cpuCount() > 1) {
			WARN_LOG(QString("CPU%1: TLB shootdown broadcast failed").arg(sourceCpu));
		}
	}

private:
	static void handleWalkFailure(
		CPUIdType cpuId,
		quint64 faultVA,
		quint64 faultPC,
		WalkResultEV6::FaultType fault,
		bool isWrite,
		IFaultSink& faultSink) noexcept
	{
		PendingEvent ev;

		switch (fault) {
		case WalkResultEV6::TNV:
			ev = makeDTBAccessViolationEvent(cpuId, faultVA, isWrite);
			ev.pendingEvent_Info.isInvalidPTE = true;
			break;
		case WalkResultEV6::FOW:
			ev = makeFaultOnWriteEvent(cpuId, faultVA);
			break;
		case WalkResultEV6::FOR_:
			ev = makeFaultOnReadEvent(cpuId, faultVA);
			break;
		case WalkResultEV6::FOE:
			ev = makeFaultOnExecuteEvent(cpuId, faultVA);
			break;
		default:
			ev = makeDTBAccessViolationEvent(cpuId, faultVA, isWrite);
			break;
		}

		faultSink.setPendingEvent(ev);
	}

	static bool validatePTE(
		const AlphaPTE& pte,
		Mode_Privilege mode,
		AccessKind access) noexcept
	{
		if (!pte.valid()) {
			return false;
		}

		switch (mode) {
		case Mode_Privilege::Kernel:
			if (access == AccessKind::READ && !pte.bitKRE()) {
				return false;
			}
			if (access == AccessKind::WRITE && !pte.bitKWE()) {
				return false;
			}
			break;

		case Mode_Privilege::User:
		case Mode_Privilege::Executive:
			if (access == AccessKind::READ && !pte.bitURE()) {
				return false;
			}
			if (access == AccessKind::WRITE && !pte.bitUWE()) {
				return false;
			}
			break;

		default:
			return false;
		}

		return true;
	}
};

// ============================================================================
// ITB_MISS Handler (Instruction TLB Miss)
// ============================================================================
class PAL_ITB_MISS_Handler_EV6 final  // x Added final
{
public:
	static void handle(
		CPUIdType cpuId,
		AlphaProcessorContext& ctx,
		IFaultSink& faultSink,
		GM_ALPHA::GuestMemory& memory) noexcept
	{
		const quint64 faultVA = ctx.readIntReg(16);
		const quint64 mmcsr = ctx.readIntReg(17);
		const quint64 faultPC = ctx.readIntReg(18);

		const ASNType asn = getCurrentASN(cpuId);
		const quint8 cm = getCM_Active(cpuId);
		const Mode_Privilege mode = (cm == 0) ? Mode_Privilege::Kernel
			: Mode_Privilege::User;

		const quint64 ptbr = getPTBR_Active(cpuId);

		// x Use shared function
		WalkResultEV6 walkResult = walkPageTable_EV6(
			faultVA,
			ptbr,
			mode,
			AccessKind::EXECUTE,
			[&memory](quint64 pa) { return readPageTableEntry(memory, pa); }
		);

		if (!walkResult.success) {
			handleWalkFailure(cpuId, faultVA, faultPC, walkResult.fault, faultSink);
			return;
		}

		if (!walkResult.pte.valid() || !walkResult.pte.bitURE()) {
			PendingEvent ev = makeITBAccessViolationEvent(cpuId, faultVA);
			faultSink.setPendingEvent(ev);
			return;
		}

		AlphaPTE pte{ walkResult.pte };  // x Create AlphaPTE object

		bool insertSuccess = globalSPAM(cpuId).tlbInsert(  // x Correct function
			cpuId,
			Realm::I,
			faultVA,
			asn,
			pte  // x AlphaPTE object
		);

		if (!insertSuccess) {
			PendingEvent ev = makeMachineCheckEvent(
				cpuId,
				MachineCheckReason::TLB_INSERTION_FAILURE,
				faultPC
			);
			faultSink.setPendingEvent(ev);
			return;
		}

		if (PAL_DTB_MISS_Handler_EV6::isMultiCPU()) {
			PAL_DTB_MISS_Handler_EV6::sendTLBShootdown(cpuId, faultVA, asn, Realm::I);
		}
	}

private:
	static void handleWalkFailure(
		CPUIdType cpuId,
		quint64 faultVA,
		quint64 faultPC,
		WalkResultEV6::FaultType fault,
		IFaultSink& faultSink) noexcept
	{
		PendingEvent ev;

		switch (fault) {
		case WalkResultEV6::TNV:
			ev = makeITBMissEvent(cpuId, faultVA);
			ev.pendingEvent_Info.isInvalidPTE = true;
			break;
		case WalkResultEV6::FOE:
			ev = makeFaultOnExecuteEvent(cpuId, faultVA);
			break;
		default:
			ev = makeITBAccessViolationEvent(cpuId, faultVA);
			break;
		}

		faultSink.setPendingEvent(ev);
	}
};

#endif // Pal_dtb_miss_handler_ev6_h__