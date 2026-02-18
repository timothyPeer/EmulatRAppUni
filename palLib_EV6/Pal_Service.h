#ifndef PAL_SERVICE_H
#define PAL_SERVICE_H
// ============================================================================
// Pal_Service.h  (header-only)
// ============================================================================
// ASCII text, UTF-8 (no BOM)
// NOTE: This header centralizes ALL PAL entry/exit, PAL mode (PC bit[0]),
//       vectoring, and related state transitions behind a single interface.
//
// Rationale (ASA):
// - PALcode must be replaceable/modular (chip/platform/OS components). See
//   "PALcode Replacement" and modular guidance.
// - PALcode environment differs from normal: interrupts disabled, complete
//   machine state control, implementation-specific enables, prevent I-stream
//   MM traps.
//
// This also aligns with your project change request:
// "PAL no longer lives in APC, MBox or AlphaCPU" and "All updates to PAL mode,
// vectoring, and bit[0|1] state should occur through this interface."
//
// Existing behavior to migrate out of Pal_AlphaProcessorContext (examples):
// - enterPALVector currently sets PAL mode directly via globalIPRHot().setInPalMode(true)
//   and jumps to entry point.
//
// TODO policy:
// - If you do not implement something, list it explicitly in the TODO section.
// ============================================================================


/*
  PAL CALL REQUEST HELPERS

  Architectural reference:
	Alpha AXP System Reference Manual (Version 6, 1994)
	Section 4.11.1 "Call Privileged Architecture Library", p. 4-120.

  Key rules:
	- CALL_PAL is not issued until all prior instructions are guaranteed to complete without exceptions.
	- CALL_PAL itself generates no architectural exceptions.
	- CALL_PAL causes a trap to PALcode.

  Design intent in this emulator:
	- Do NOT model CALL_PAL as an exception-class fault.
	- Model CALL_PAL as a control-transfer request into PAL, delivered via a dedicated PendingEventKind::PalCall
	  using a non-fault enqueue API (setPendingEvent or enqueueEvent).
	- AlphaCPU runloop consumes the PalCall event and invokes PalBox / PalService.
*/



#include "coreLib/types_core.h"
#include <utility>

#include "PAL_core.h"
#include "PalArgumentPack_str.h"
#include "coreLib/astEligibility_inl.h"
#include "faultLib/FaultDispatcher.h"
#include "faultLib/GlobalFaultDispatcherBank.h"
#include "configLib/EmulatorSettingsInline.h"
#include "coreLib/Ast_core_inl.h"
#include "coreLib/IPR_core.h"
#include "pteLib/ev6Translation_struct.h"
#include "emulatrLib/IpiManager.h"
#include "memoryLib/GuestMemory.h"
#include <coreLib/AMASK_Constants_inl.h>
#include <deviceLib/global_ConsoleManager.h>
#include "deviceLib/SRMEnvStore.h"
#include "deviceLib/ConsoleManager.h"
#include "configLib/global_EmulatorSettings.h"
#include "coreLib/EXECTRACE_Macros.h"
#include "coreLib/HWPCB_SwapContext.h"
#include "coreLib/IRQPendingState.h"
#include "coreLib/InterruptRouter.h"
#include "coreLib/IprStorage_core.h"
#include "coreLib/PS_helpers_inl.h"
#include "coreLib/global_RegisterMaster_hot.h"
#include "deviceLib/global_SRMEnvStore.h"
#include "emulatrLib/global_IPIManager.h"
#include "palLib_Ev6/global_PalVectorTable.h"
#include "palLib_EV6/Pal_CSERVE_ConsoleHandlers_inl.h"
#include "pteLib/Ev6SiliconTypes.h"
#include "coreLib/IRQAstandSCBHelpers.h"

// Forward decls (keep PAL modular; do not "own" CPU/APC/MBox).
struct PipelineSlot;
struct PendingEvent;
struct PalArgumentPack;
struct PalResult;
struct BoxResult;


enum class PalVectorId_EV6 : quint16;
enum class PalCallPalFunction;
enum class ProbeResult : quint8;

#define COMPONENT_NAME "PalService"

// ============================================================================
// PalService
// Central authority for:
// - Entering/exiting PAL environment.
// - Managing PAL mode flag + PC[0] semantics (single source of truth).
// - Vector dispatch (PAL_BASE + implementation offset table).
//
// IMPORTANT: No other subsystem should call globalIPRHot(cpuId).setInPalMode()
//            directly once this is adopted.
// 
// Only events listed in Table 21264-Alpha Datasheet.pdf, sec. 5-8 use named PAL vectors;
// all CALL_PAL functions use calculated entry addresses.
// ============================================================================
class alignas(16) PalService final
{

    FaultDispatcher* m_faultDispatcher;
	bool m_cachedInPalMode{ false };
	CPUIdType m_cpuId;
	int m_cpuCount{ 4 };			// default to ES40
	EmulatorSettingsInline* m_emulatorSettings;



	IPIManager* m_ipiManager;
	// IRQ Controllers
	IRQPendingState* m_pending;
	InterruptRouter* m_router;
	//
	QScopedPointer<Ev6Translator> m_ev6Translation{ nullptr };
	Ev6SPAMShardManager* m_tlb;           // TLB lookup/insert
	HWPCB* m_hwpcb;
	quint64 m_pc;
	GuestMemory* m_guestMemory;
	ReservationManager* m_reservationManager;
	ConsoleManager* m_consoleManager;
	SRMEnvStore* m_srmEnvStore; 
	CPUStateView  m_cpuView;                            // value member
	CPUStateView* m_iprGlobalMaster{ &m_cpuView };
	Ev6Translator m_ev6_translator;

	// Platform

	GrainPlatform m_palVariant{ GrainPlatform::VMS };
public:
	PalService(const PalService&)            = delete;
	PalService& operator=(const PalService&) = delete;


	// Move operations also deleted (or implement if needed)
	PalService(PalService&&) = delete;
	PalService& operator=(PalService&&) = delete;
	PalService(const CPUIdType cpuId, IRQPendingState* pendingState, InterruptRouter* interruptRouter) noexcept
		: m_faultDispatcher(&globalFaultDispatcher())
		, m_cpuId(cpuId)
		, m_cpuCount(global_EmulatorSettings().podData.system.processorCount)
		, m_emulatorSettings(&global_EmulatorSettings())
		, m_router{ (interruptRouter) }
		, m_pending{(pendingState)}
		, m_ipiManager(&global_IPIManager())
		, m_hwpcb(&globalHWPCBController(cpuId))
	
		, m_tlb(&globalSPAM(cpuId))           // TLB lookup/insert
		, m_guestMemory(&global_GuestMemory())
		, m_reservationManager(&globalReservationManager())
		, m_consoleManager(&global_ConsoleManager())
		, m_srmEnvStore(&global_SRMEnvStore())
	, m_ev6_translator{cpuId}
		, m_iprGlobalMaster(getCPUStateView(cpuId))
	{

		m_ev6Translation.reset(new Ev6Translator(m_cpuId));
		DEBUG_LOG(QString("PalService: Initialized for CPU %1 (system has %2 CPUs)")
			.arg(cpuId).arg(m_cpuCount));

	}

	// ---------------------------------------------------------------------------
	// PalService special-case "MxPR" implementations (consume R16, return R0 old)
	// ---------------------------------------------------------------------------

	AXP_HOT AXP_ALWAYS_INLINE void updateAstEligibility(quint32 cpuId) noexcept
	{
		const quint8 asten = static_cast<quint8>(m_iprGlobalMaster->h->aster & 0x0Fu);
		const quint8 astsr = static_cast<quint8>(m_iprGlobalMaster->h->astsr & 0x0Fu);
		const quint8 cm    = static_cast<quint8>(m_iprGlobalMaster->h->cm & 0x03u);
		const quint8 ipl   = static_cast<quint8>(m_iprGlobalMaster->h->ipl & 0x1Fu);

		const asa_ast::AstEligibilityResult e =
			asa_ast::computeAstEligibility(asten, astsr, cm, ipl);

		if (e.anyEligible)
			m_router->raiseAST(static_cast<int>(cpuId));
		else
			m_router->clearAST(static_cast<int>(cpuId));
	}

	// NOTE: naming here assumes these are MTPR-style operations (read+modify+write).
	// -----------------------------------------------------------------------------
	// Integration helper: updateAstEligibility()
	// -----------------------------------------------------------------------------
	// This version assumes:
	//  - m_iprGlobalMaster->h points to the active HWPCB for this CPU
	//
	// If you do NOT have setAstPending/setAstTargetMode, keep the returned result
	// and integrate into whatever structure you call IRQPendingState.
	//
	// IMPORTANT: Use slot.cpuId, not m_cpuId, for SMP correctness.
	// -----------------------------------------------------------------------------

	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_ASTEN(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 r16 = readIntReg(slot, 16);
		const quint8 oldASTEN =
			static_cast<quint8>(m_iprGlobalMaster->h->aster & 0x0F);
		const quint8 newASTEN =
			AXP_IPR_AST::applyMaskedRmw4(oldASTEN, r16);

		m_iprGlobalMaster->h->aster =
			(m_iprGlobalMaster->h->aster & ~quint64(0x0F)) | quint64(newASTEN);

		// Re-evaluate AST eligibility with new enable mask
		updateAstEligibility(m_cpuId);
		result = PalResult::Return(PalReturnReg::R0, quint64(oldASTEN));
		result.flushPendingTraps();   // AFTER the Return assignment
	}

	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_ASTSR(PipelineSlot& slot, PalResult& result) noexcept
	{
		quint64 astsr = m_iprGlobalMaster->h->astsr;
		quint64 r16 = readIntReg(slot, 16);
		const quint8 old4 = AXP_IPR_AST::mtpr_update4(astsr, r16);

		// Store the updated value back (astsr was modified by mtpr_update4)
		m_iprGlobalMaster->h->astsr = astsr;

		// Re-evaluate AST eligibility with new pending mask
		const quint8 newASTSR = static_cast<quint8>(astsr & 0x0F);
		updateAstEligibility(m_cpuId);
		result = PalResult::Return(PalReturnReg::R0, quint64(newASTSR));
		result.flushPendingTraps();   // AFTER the Return assignment
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	AXP_HOT AXP_ALWAYS_INLINE void executeTB_FILL_ITB(PipelineSlot& slot) const
	{
		// 1. Flush any pending IPR writes
		slot.palResult.flushPendingIPRWrites();

		// 2. Extract ALL parameters from staged registers
		quint64 itbTag = m_iprGlobalMaster->x->itb_tag;
		quint64 va = Ev6Translator::extractVAFromTLBTag(itbTag);
		ASNType asn = Ev6Translator::extractASNFromTLBTag(itbTag);
		quint64 pte = m_iprGlobalMaster->x->itb_pte_temp;

		// 3. Commit to hardware TLB
		m_tlb->tlbInsert(slot.cpuId, Realm::I, asn, va, pte);

		// 4. Clear staging registers  
		m_iprGlobalMaster->x->itb_tag = 0;
		m_iprGlobalMaster->x->itb_pte_temp = 0;
	

		DEBUG_LOG(QString("TB_FILL_ITB: CPU %1 filled VA=0x%2 ASN=%3")
			.arg(slot.cpuId).arg(va, 16, 16).arg(asn));
	}


	AXP_HOT AXP_ALWAYS_INLINE  void executePalCall(quint8 palFunction, quint64 r16, quint64 r17) noexcept
	{
		const quint64 r16q64 = m_iprGlobalMaster->readInt(16);
		const quint64 r17q64 = m_iprGlobalMaster->readInt(17);

		requestPalCallEvent(palFunction, r16q64, r17q64, m_iprGlobalMaster->h->pc);
	}
	
	AXP_HOT AXP_ALWAYS_INLINE  void executePalCall(PipelineSlot& slot) noexcept
	{
		// Extract PAL function from instruction
		// CALL_PAL format: bits [25:0] contain the PAL function code
		const quint32 rawBits = slot.di.rawBits();
		const quint8 palFunction = static_cast<quint8>(rawBits & 0xFF);  // Low 8 bits
		const PalCallPalFunction pal_function = static_cast<PalCallPalFunction>(palFunction);	// this is an enumeration of function codes for CALL_PAL 0x00

		// Read arguments from integer registers
		const quint64 r16q64 = m_iprGlobalMaster->readInt(16);
		const quint64 r17q64 = m_iprGlobalMaster->readInt(17);

		// Get PC where CALL_PAL was issued
		const quint64 callPalPC = slot.di.pc;

		execute(pal_function, slot, slot.palResult);			// execute the CALL_PAL Function

		// Delegate to the existing implementation
		//requestPalCallEvent(palFunction, r16q64, r17q64, callPalPC);
	}

	/*
	  Helper used by PalService to REQUEST a PAL entry.

	  Requirements:
		- The caller supplies palFunction (low 16 bits of CALL_PAL instruction).
		- R16/R17 are captured for PAL calling convention usage by PALcode services (your ABI choice).
		- The PC recorded is the CALL_PAL instruction PC (precise trap point).

	  IMPORTANT:
		This is a request/event enqueue, not a "fault raise".
	*/
	AXP_HOT AXP_ALWAYS_INLINE void requestPalCallEvent(quint8 palFunction, quint64 r16, quint64 r17, quint64 callPalPC) const noexcept
	{
		PendingEvent ev{};
		ev.kind = PendingEventKind::PalCall;
		ev.palFunction = palFunction;
		ev.palR16 = r16;
		ev.palR17 = r17;

		// For CALL_PAL, record the trap point PC (ASA 4.11.1, p. 4-120).
		ev.faultPC = callPalPC;

		// Use a non-fault "pending event" API.
		// If your dispatcher only has setPendingEvent, use that.
		m_faultDispatcher->setPendingEvent(ev);
	}
	// Report memory fault
	AXP_HOT AXP_ALWAYS_INLINE  void reportMemoryFault(quint64 va, bool isWrite) noexcept
	{
		PendingEvent ev{};
		ev.kind = PendingEventKind::Exception;
		ev.exceptionClass = ExceptionClass_EV6::Dfault;
		ev.faultVA = va;
		ev.faultPC = m_iprGlobalMaster->h->pc; 
		ev.cm = m_iprGlobalMaster->h->cm;

		m_faultDispatcher->raiseFault(ev);
	}

	AXP_HOT AXP_ALWAYS_INLINE void dispatchPendingEvent(PipelineSlot& slot, const PendingEvent& ev) noexcept
	{
		const quint64 entryPC = resolvePalEntryPC(slot.cpuId, ev);

		// Save fault PC
		m_iprGlobalMaster->h->exc_addr = ev.faultPC;

		// Snapshot state
		// Is performed in AlphaCPU

		// Enter PAL
		setPalMode(true);
		m_iprGlobalMaster->h->setCM( 0) ; // kernel

		m_iprGlobalMaster->h->forcePalPC(canonicalizePalPC(entryPC));
		// Jump to PAL entry (PC[0] = 1)

		slot.needsWriteback = false;
	}


	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_ASTEN(PipelineSlot& slot, PalResult& result) const noexcept
	{
		result = PalResult::Return(PalReturnReg::R0,
			static_cast<quint64>(m_iprGlobalMaster->h->aster & 0x0F));
	}

	// ------------------------------------------------------------------------
	// Vector entry
	// Implements the "save state, enter PAL env, dispatch to handler" flow.
	//
	// ASA requirements/guidance:
	// - PAL needs a mechanism to save machine state and dispatch into PALcode.
	// - PAL env: interrupts disabled; complete control; prevent I-stream MM traps.
	// - PAL modular replacement requirement.
	// ------------------------------------------------------------------------
	// ========================================================================
	// UNIFIED PAL ENTRY - Single Entry Point
	// ========================================================================
	AXP_HOT AXP_ALWAYS_INLINE void enterPALVector(PipelineSlot& slot, PalVectorId_EV6 vecId, quint64 exceptionPC, const PalArgumentPack& args) noexcept
	{
		// ====================================================================
		// 1. Lookup PAL vector entry
		// ====================================================================
		const PalVectorEntry* entry = global_PalVectorTable().lookup(vecId);
		if (!entry || entry->entryPC == 0xDEADBEEFDEADBEEF) {
			CRITICAL_LOG(QString("CPU%1: Invalid PAL vector %2")
				.arg(slot.cpuId)
				.arg(static_cast<int>(vecId)));

			// Escalate to machine check
			PendingEvent mchk{};
			mchk.kind = PendingEventKind::MachineCheck;
			mchk.exceptionClass = ExceptionClass_EV6::MachineCheck;
			mchk.faultPC = exceptionPC;
			m_faultDispatcher->setPendingEvent(mchk);  // ? Use setPendingEvent, not raiseFault
			return;
		}

		// ====================================================================
		// 2. Save architectural state to HWPCB
		// ====================================================================
		m_iprGlobalMaster->h->exc_addr = exceptionPC;

		// Snapshot current processor state
		const quint64 currentPS = m_iprGlobalMaster->h->ps;

		// context is saved in AlphaCPU

		// ====================================================================
		// 3. Enter PAL mode (blocks non-critical interrupts)
		// ====================================================================
		setPalMode(true);

		// Force kernel mode (CM=0) in PAL
		m_iprGlobalMaster->h->setCM( 0);

		// ====================================================================
		// 4. Set target IPL if vector modifies it
		// ====================================================================
		if (entry->flags & PalVectorEntry::MODIFIES_IPL) {
			m_iprGlobalMaster->h->setIPL_Unsynced(entry->targetIPL);
		}

		// ====================================================================
		// 5. Load PAL arguments into R16-R21 (a0-a5)
		// ====================================================================
		writeIntReg(16, args.a0);
		writeIntReg(17, args.a1);
		writeIntReg(18, args.a2);
		writeIntReg(19, args.a3);
		writeIntReg(20, args.a4);
		writeIntReg(21, args.a5);

		// ====================================================================
		// 6. Transfer control to PAL entry point
		// ====================================================================
		
		m_iprGlobalMaster->h->forcePalPC(canonicalizePalPC(entry->entryPC));
		
		DEBUG_LOG(QString("CPU%1: Entered PAL vector %2 at PC 0x%3")
			.arg(slot.cpuId)
			.arg(entry->name ? entry->name : "unknown")
			.arg(entry->entryPC, 16, 16, QChar('0')));
	}


	// ReSharper disable once CppMemberFunctionMayBeStatic
	AXP_HOT AXP_ALWAYS_INLINE  bool readVirtualString(quint64 va, quint64 maxLength,
		CPUIdType cpuId,
		QString& outString) const noexcept
	{
		QByteArray buffer;

		for (quint64 i = 0; i < maxLength; i++) {
			quint8 ch;
			if (m_ev6Translation->readVirtualByteFromVA( va + i, ch) != MEM_STATUS::Ok) {
				return false;
			}

			if (ch == 0) break;
			buffer.append(static_cast<char>(ch));
		}

		outString = QString::fromUtf8(buffer);
		return true;
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	inline MEM_STATUS writeVirtualQword(quint64 va, quint64 value,
		CPUIdType cpuId) const noexcept
	{
		PAType pa_out;
		AlphaPTE outpte{};
		TranslationResult tr = m_ev6Translation->ev6TranslateFastVA(va, AccessKind::WRITE,	static_cast<Mode_Privilege>(m_iprGlobalMaster->h->cm),
			pa_out, &outpte
		);

		if (tr != TranslationResult::Success) {
			return MEM_STATUS::TranslationFault;
		}

		return m_guestMemory->write64(pa_out, value);
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	inline MEM_STATUS readVirtualQword(quint64 va, quint64& value,
		CPUIdType cpuId) const noexcept
	{
		quint64 pa;
		AlphaPTE pte{};

		TranslationResult tr = m_ev6Translation->ev6TranslateFastVA(
			 va, AccessKind::READ,
			static_cast<Mode_Privilege>(m_iprGlobalMaster->h->cm),
			pa, &pte
		);

		if (tr != TranslationResult::Success) {
			return MEM_STATUS::TranslationFault;
		}

		return m_guestMemory->read64(pa, value);
	}

	inline MEM_STATUS writeVirtualLongword(quint64 va, quint32 value,
		CPUIdType cpuId) const noexcept
	{
		quint64 pa;
		AlphaPTE pte{};


		TranslationResult tr = m_ev6Translation->ev6TranslateFastVA(
			 va, AccessKind::WRITE,
			static_cast<Mode_Privilege>(m_iprGlobalMaster->h->cm),
			pa, &pte
		);

		if (tr != TranslationResult::Success) {
			return MEM_STATUS::TranslationFault;
		}

		return m_guestMemory->write32(pa, value);
	}
	inline MEM_STATUS readVirtualLongword(quint64 va, quint32& value, CPUIdType cpuId) const noexcept
	{
		quint64 pa;
		AlphaPTE pte{};


		TranslationResult tr = m_ev6Translation->ev6TranslateFastVA(
			 va, AccessKind::READ,
			static_cast<Mode_Privilege>(m_iprGlobalMaster->h->cm),
			pa, &pte
		);

		if (tr != TranslationResult::Success) {
			return MEM_STATUS::TranslationFault;
		}

		return m_guestMemory->read32(pa, value);
	}


	#pragma region PalBox Helpers


	AXP_HOT AXP_ALWAYS_INLINE void  PAL_SWPCTX_Write_ISUM(quint64 new_isum) const noexcept {
		quint8 astsr = static_cast<quint8>(m_iprGlobalMaster->h->astsr);

		// Extract AST bits from ISUM and update ASTSR (SSOT)
		updateASTSRFromISUM(astsr, new_isum);
	}

	// READING: OS/PAL reads AST state via HWPCB ISUM
	AXP_HOT AXP_ALWAYS_INLINE quint64 PAL_SWPCTX_Read_ISUM() const noexcept {
		quint8 astsr;
		astsr = static_cast<quint8>(m_iprGlobalMaster->h->astsr);

		// Build ISUM from ASTSR (SSOT)
		quint64 isum = buildISUMFromASTSR(astsr);

		// Add other ISUM bits (IPL, interrupts, etc.)
		// TODO

		return isum;
	}

	

	/*
	 *	
		readIPR : switch (index) -> load from correct container -> writeInt(0, result)
		writeIPR : readInt(16) -> switch (index) -> store to correct container or trigger action
	 */
	// ============================================================================
	// readIPR — HW_MFPR: R0 <- IPR
	// ============================================================================
	AXP_HOT AXP_ALWAYS_INLINE void readIPR(quint16 iprIndex2, quint64& result) const noexcept
	{
		switch (iprIndex2)
		{
		case IPR_MFPR_ASN: result = m_iprGlobalMaster->h->asn;          break;
		case IPR_MFPR_ESP: result = m_iprGlobalMaster->h->esp;			break;
			break;
		case IPR_MFPR_IPL: result = m_iprGlobalMaster->h->getIPL();
			break;
		case IPR_FEN: 	result = m_iprGlobalMaster->h->fen;          break;
		case IPR_MFPR_MCES:     result = m_iprGlobalMaster->x->mces & 0x1F;  // only bits 4:0 defined
			break;
		case IPR_MFPR_PCBB:  result = m_iprGlobalMaster->x->pcbb;         break;
		case IPR_MFPR_PRBR:	 result = m_iprGlobalMaster->x->prbr;         break;
		case IPR_MFPR_PTBR:  result = m_iprGlobalMaster->h->ptbr;         break;
		case IPR_MFPR_SCBB:  result = m_iprGlobalMaster->x->scbb;         break;
		case IPR_MFPR_SISR:  result = m_iprGlobalMaster->h->sisr;		break;
		case IPR_MFPR_SSP:	 result = m_iprGlobalMaster->h->ssp;		break;
		case IPR_MFPR_SYSPTBR:	result = m_iprGlobalMaster->x->sysptbr;	break;
		case IPR_MFPR_TBCHK:	result = m_tlb->tbchkProbe(m_cpuId, m_iprGlobalMaster->readInt(16),
			m_iprGlobalMaster->h->asn);
		case IPR_MFPR_USP:		result = m_iprGlobalMaster->h->usp;          break;
		case IPR_MFPR_VIRBND:  result = m_iprGlobalMaster->x->virbnd;       break;
		case IPR_MFPR_VPTB: result = m_iprGlobalMaster->x->vptb;         break;
		case IPR_MFPR_WHAMI:	result = m_iprGlobalMaster->x->whami;	break;
		default:
			{
				// TODO we need to log that the IPR read passed a grain whose function code did not match to the list.
			}

		}

		// R0 <- result (architectural contract)
		m_iprGlobalMaster->writeInt(0, result);
	}

	// ============================================================================
	// writeIPR — HW_MTPR: IPR <- R16
	// ============================================================================
	AXP_HOT AXP_ALWAYS_INLINE void writeIPR( quint16 iprIndex, PipelineSlot& slot) noexcept
	{
		// Source is always R16
		const quint64 value = m_iprGlobalMaster->readInt(16);

		switch (iprIndex)
		{
		
		/*
		 *These cases are handled later in the writeIPR_fromSlot()
		 *case IPR_MTPR_ASTEN: {
				m_iprGlobalMaster->h->aster = static_cast<quint8>(value & 0x0F); 
				updateAstEligibility(slot.cpuId);
				slot.palResult.flushPendingTraps();
				break;
		}
		case IPR_MTPR_ASTSR:  
		{
			m_iprGlobalMaster->h->astsr = static_cast<quint8>(value & 0x0F); break;
			updateAstEligibility(slot.cpuId);
			slot.palResult.flushPendingTraps();
			break;
		}*/
		case IPR_MTPR_DATFX:	m_iprGlobalMaster->h->datfx = value; break;
		case IPR_MTPR_ESP:		m_iprGlobalMaster->h->esp = value; break;
		case IPR_MTPR_IPIR:		executeWRIPIR(slot, slot.palResult); break;
		case IPR_MTPR_IPL: {
			const quint8 oldIPL = m_iprGlobalMaster->h->getIPL();
			const quint8 newIPL = static_cast<quint8>(value & 0x1F);
			m_iprGlobalMaster->h->setIPL_Unsynced(newIPL);
			slot.palResult.iplChanged();
			if (newIPL < oldIPL) slot.palResult.flushPendingTraps();
			break;
		}
		case IPR_MTPR_MCES: {
			quint64 old = m_iprGlobalMaster->x->mces;
			// Bits 2:0 are write-1-to-clear (writing 1 CLEARS the bit)
			quint64 cleared = old & ~(value & 0x07);
			// Bits 4:3 are direct write (DPC, DSC enable/disable)
			cleared = (cleared & ~0x18) | (value & 0x18);
			m_iprGlobalMaster->x->mces = cleared;
			break;
		}
		case IPR_MTPR_PERFMON:	m_iprGlobalMaster->x->perfmon = value; break;
		case IPR_MTPR_PRBR:		m_iprGlobalMaster->x->prbr = value; break;
		case IPR_MTPR_SCBB:		m_iprGlobalMaster->x->scbb = value; break;
		case IPR_MTPR_SIRR: 
		{
			const quint8 level = static_cast<quint8>(value & 0xF);
			if (level >= 1 && level <= 15) {
				m_iprGlobalMaster->h->sisr |= static_cast<quint16>(1u << level);
				m_pending->raise(level, level);
				slot.palResult.flushPendingTraps();

			} break;
		}	
		case IPR_MTPR_SSP:		m_iprGlobalMaster->h->ssp = value; break;
		case IPR_MTPR_SYSPTBR:	m_iprGlobalMaster->x->sysptbr = value; break;
		case IPR_MTPR_TBIA: 	executeTBI(slot, slot.palResult); break;
		case IPR_MTPR_TBIAP:	executeMTPR_TBIAP(slot, slot.palResult); break;
		case IPR_MTPR_TBIS:		executeTBIS(slot, slot.palResult); break;
		case IPR_MTPR_TBISD:	executeTBISD(slot, slot.palResult); break;
		case IPR_MTPR_TBISI:	executeTBISI(slot, slot.palResult); break;
		case IPR_MTPR_USP:		m_iprGlobalMaster->h->usp = value; break;
		case IPR_MTPR_VIRBND:   m_iprGlobalMaster->x->virbnd = value; break;
		case IPR_MTPR_VPTB:		m_iprGlobalMaster->x->vptb = value; break;
		case IPR_FEN:  m_iprGlobalMaster->h->fen = static_cast<quint8>(value & 0x1); break;
		default:
			{
				// TODO implement log trace - catch any IPRs that are passed by not handled
			}
		}
	}

 AXP_HOT AXP_ALWAYS_INLINE	void writeIPR_fromSlot(HW_IPR iprIndex, quint64 value, PipelineSlot& slot)  noexcept {

	 switch (iprIndex)
	 {
	 case IPR_MTPR_ASTEN:
		 executeMTPR_ASTEN(slot, slot.palResult);
		 return;
	 case IPR_MTPR_ASTSR:
		 executeMTPR_ASTSR(slot, slot.palResult);
		 return;
	
	 default:
		 writeIPR(iprIndex, slot);
	 }
	}
#pragma endregion PalBox Helpers

	// Version 1: Takes cpuId, will lookup dispatcher internally
	AXP_HOT AXP_ALWAYS_INLINE  void reportException(const PendingEvent& ev) const noexcept
	{
		// Get dispatcher and report
		m_faultDispatcher->raiseFault(ev);
	}

	// ------------------------------------------------------------------------
	// Mandatory Single-Point PAL-mode mutation
	// ------------------------------------------------------------------------

	AXP_HOT AXP_ALWAYS_INLINE  bool isInPalMode() const noexcept
	{
		// Cache is allowed for speed, but must be synchronized by this service.
		return m_cachedInPalMode;
	}

	// ------------------------------------------------------------------------
	// Mandatory Single-Point PAL-mode mutation
	// ------------------------------------------------------------------------

	AXP_HOT AXP_ALWAYS_INLINE void setPalMode(bool enable, bool reset = false) noexcept
	{
		// Update IPRHot (architectural state)
		quint64 pc = m_iprGlobalMaster->h->pc;
		if (reset)
			m_iprGlobalMaster->setPC(0x0000000000008001); // EV6 starts at 0x0000_0000_0000_8001 (PAL mode bit set)
		else
			m_iprGlobalMaster->setPC((pc & 0x1));

		// Update local cache
		m_cachedInPalMode = enable;
		m_iprGlobalMaster->setPalMode(true);
	}
	// ------------------------------------------------------------------------
  // PC bit[0] policy (architectural PAL-mode tagging convention)
  // - You already synchronize PAL mode with PC[0] in other code paths.
  // - Move that authority here: callers request "PAL PC" or "User PC".
  // ------------------------------------------------------------------------
	// ReSharper disable once CppMemberFunctionMayBeStatic
	AXP_HOT AXP_ALWAYS_INLINE  quint64 canonicalizePalPC(quint64 pc) noexcept {
		// Convention used in your code base: PC[0]=1 indicates PAL context.
		return (pc | 0x1ULL);
	}

	AXP_HOT AXP_ALWAYS_INLINE   quint64 canonicalizeUserPC(quint64 pc) const noexcept
	{
		return (pc & ~0x1ULL);
	}

	// IPL Orchestration - Disable IPL (Hybrid) when in Pal Mode
	// In PalService class:

	/**
	 * @brief Check if interrupt should be blocked due to PAL mode
	 *
	 * @param interruptIPL IPL of pending interrupt
	 * @param isCritical Is this a critical interrupt (MCHK)?
	 * @return true if interrupt should be blocked
	 */
	AXP_HOT AXP_ALWAYS_INLINE  bool shouldBlockInterrupt(IPLType interruptIPL, bool isCritical) const noexcept
	{
		// In PAL mode, block all non-critical interrupts
		return true;
	}

	// ------------------------------------------------------------------------
	// Exit PAL environment (typically via HW_REI / PAL return path)
	// ------------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void exitPAL() noexcept
	{
		// ASA: hardware mechanism transitions PAL env back to non-PAL env:
		// loads PC, enables interrupts, enables mapping, disables PAL privileges.
		// In this project: globalIPRHot64().setInPalMode(false) also demotes mode to user
		// (per your existing comment).
		setPalMode(false);
		// NOTE: IPL is restored by HW_REI from saved PS
		// Interrupts can now be delivered based on restored IPL
	}

	// ------------------------------------------------------------------------
	// Hooks for PAL replacement (OS vs firmware PAL, SWPPAL, REBOOT)
	// NOTE: Keep these in PAL service so transitions are centralized.
	// ------------------------------------------------------------------------
	AXP_HOT AXP_ALWAYS_INLINE void swapPalBase(PipelineSlot& slot, quint64 newPalBase) noexcept
	{
		// ASA: PALcode replacement is required; PAL_BASE participates in vectoring.
		m_iprGlobalMaster->x->pal_base = newPalBase;
	}



#pragma region CALL_PAL services

	AXP_HOT AXP_ALWAYS_INLINE void execute(PalCallPalFunction fn, PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint16 funcCode = static_cast<quint16>(getFunctionCode(slot.di));

		// Sanity check
		if (funcCode > static_cast<quint16>(PalCallPalFunction::MAX_PAL_FUNCTION)) {
			ERROR_LOG(QString("CPU %1: PAL function 0x%2 out of range!")
				.arg(slot.cpuId)
				.arg(funcCode, 4, 16, QChar('0')));
			palUnimplemented(slot, result);
			return;
		}

		// Default result behavior: return to caller, no flush
		result = PalResult{};
		result.doesReturn = true;

		switch (fn)
		{
		case PalCallPalFunction::MFPR_ASN:  executeMFPR_ASN(slot, result); break;//PAL

		case PalCallPalFunction::MFPR_ASTSR:   executeMFPR_ASTSR(slot, result); break;//PAL
		case PalCallPalFunction::READ_UNQ:  executeREAD_UNQ(slot, result); break;//PAL
		case PalCallPalFunction::WRITE_UNQ: executeWRITE_UNQ(slot, result); break;//PAL
		case PalCallPalFunction::GENTRAP:   executeGENTRAP(slot, result); break;//PAL
		case PalCallPalFunction::RD_PS:     executeRD_PS(slot, result); break;//PAL
		case PalCallPalFunction::WR_PS_SW:     executeWR_PS(slot, result); break;
		case PalCallPalFunction::RSCC:      executeRSCC(slot, result); break;
		case PalCallPalFunction::PROBER:    executePROBER(slot, result); break;//PAL
		case PalCallPalFunction::PROBEW:    executePROBEW(slot, result); break;//PAL
		case PalCallPalFunction::HALT:      executeHALT(slot, result); break;//PAL
		case PalCallPalFunction::CFLUSH:    executeCFLUSH(slot, result); break; // PAL
		case PalCallPalFunction::DRAINA:    executeDRAINA(slot, result); break;//PAL
		case PalCallPalFunction::CSERVE:    executeCSERVE(slot, result); break;//PAL
		case PalCallPalFunction::SWPPAL:    executeSWPPAL(slot, result); break;
		case PalCallPalFunction::IMB:       executeIMB(slot, result); break;//PAL
		case PalCallPalFunction::BPT:	executeBPT(slot, result); break; //PAL
		case PalCallPalFunction::BUGCHECK:	executeBUGCHK(slot, result); break;// PAL
		case PalCallPalFunction::MFPR_FEN:	executeMFPR_FEN(slot, result); break;
		case PalCallPalFunction::MTPR_FEN:	executeMTPR_FEN(slot, result); break;
		case PalCallPalFunction::MTPR_IPIR: executeMTPR_IPIR(slot, result); break;
		case PalCallPalFunction::MFPR_IPL:	executeMFPR_IPL(slot, result); break;
		case PalCallPalFunction::MTPR_IPL:	executeMTPR_IPL(slot, result); break;
		case PalCallPalFunction::MFPR_MCES:	executeMFPR_MCES(slot, result); break;
		case PalCallPalFunction::MTPR_MCES:	executeMTPR_MCES(slot, result); break;
		case PalCallPalFunction::MFPR_PCBB:	executeMFPR_PCBB(slot, result); break;
		case PalCallPalFunction::MFPR_PRBR:	executeMFPR_PRBR(slot, result); break; //Processor Base Register - Base address for per-CPU data structures
		case PalCallPalFunction::MTPR_PRBR:	executeMTPR_PRBR(slot, result); break; //Processor Base Register - Base address for per-CPU data structures
		case PalCallPalFunction::MFPR_PTBR:	executeMFPR_PTBR(slot, result); break;
		case PalCallPalFunction::MFPR_SCBB:	executeMFPR_SCBB(slot, result); break;
		case PalCallPalFunction::MTPR_SCBB:	executeMTPR_SCBB(slot, result); break;
		case PalCallPalFunction::MFPR_SIRR:	executeMFPR_SIRR(slot, result); break;
		case PalCallPalFunction::MFPR_SISR:	executeMFPR_SISR(slot, result); break;
		case PalCallPalFunction::MFPR_TBCHK:	executeMFPR_TBCHK(slot, result); break;
		case PalCallPalFunction::MTPR_TBIA:		executeMTPR_TBIA(slot, result); break;
		case PalCallPalFunction::MTPR_TBIAP:	executeMTPR_TBIAP(slot, result); break;
		case PalCallPalFunction::MTPR_TBIS:	executeMTPR_TBIS(slot, result); break;
		case PalCallPalFunction::MFPR_ESP:	executeMFPR_ESP(slot, result); break;
		case PalCallPalFunction::MTPR_ESP:	executeMTPR_ESP(slot, result); break;
		case PalCallPalFunction::MFPR_SSP:	executeMFPR_SSP(slot, result); break;
		case PalCallPalFunction::MTPR_SSP:	executeMTPR_SSP(slot, result); break;
		case PalCallPalFunction::MFPR_USP:	executeMFPR_USP(slot, result); break;
		case PalCallPalFunction::MTPR_USP:	executeMTPR_USP(slot, result); break;
		case PalCallPalFunction::MTPR_TBISD:	executeMTPR_TBISD(slot, result); break;
		case PalCallPalFunction::MTPR_TBISI:	executeMTPR_TBISI(slot, result); break;
		case PalCallPalFunction::MFPR_ASTEN:	executeMFPR_ASTEN(slot, result); break;
		case PalCallPalFunction::MFPR_VPTB:	executeMFPR_VPTB(slot, result); break;
		case PalCallPalFunction::MTPR_VPTB:	executeMTPR_VPTB(slot, result); break;
		case PalCallPalFunction::MTPR_PERFMON:	executeMTPR_PERFMON(slot, result); break;
		case PalCallPalFunction::MTPR_DATFX:	executeMTPR_DATFX(slot, result); break;
		case PalCallPalFunction::MFPR_WHAMI:	executeMFPR_WHAMI(slot, result); break;
		case PalCallPalFunction::SWPCTX:    executeSWPCTX(slot, result); break; // PAL
		case PalCallPalFunction::CHME:	executeCHME(slot, result); break; // PAL
		case PalCallPalFunction::CHMK:	executeCHMK(slot, result); break; //PAL
		case PalCallPalFunction::CHMS:	executeCHMS(slot, result); break; //PAL
		case PalCallPalFunction::CHMU:	executeCHMU(slot, result); break;// PAL
		case PalCallPalFunction::AMOVRM:    executeAMOVRM(slot, result); break; //PAL
		case PalCallPalFunction::AMOVRR:    executeAMOVRR(slot, result); break; // PAL
		case PalCallPalFunction::INSQHIL:	executeINSQHIL(slot, result); break; //PAL
		case PalCallPalFunction::INSQTIL:	executeINSQTIL(slot, result); break;//PAL
		case PalCallPalFunction::INSQHILR:	executeINSQHILR(slot, result); break; //PAL
		case PalCallPalFunction::INSQTILR:	executeINSQTILR(slot, result); break;//PAL
		case PalCallPalFunction::INSQHIQR:	executeINSQHIQR(slot, result); break; //PAL
		case PalCallPalFunction::INSQTIQR:	executeINSQTIQR(slot, result); break;//PAL
		case PalCallPalFunction::INSQHIQ:	executeINSQHIQ(slot, result); break;//PAL
		case PalCallPalFunction::INSQTIQ:	executeINSQTIQ(slot, result); break;//PAL
		case PalCallPalFunction::INSQUEL:	executeINSQUEL(slot, result); break;//PAL
		case PalCallPalFunction::INSQUEQ:	executeINSQUEQ(slot, result); break;//PAL
		case PalCallPalFunction::INSQUEL_D:	executeINSQUEL_D(slot, result); break;//PAL
		case PalCallPalFunction::INSQUEQ_D:	executeINSQUEQ_D(slot, result); break;//PAL
		case PalCallPalFunction::REMQHIL:    executeREMQHIL(slot, result); break;
		case PalCallPalFunction::REMQTIL:    executeREMQTIL(slot, result); break;
		case PalCallPalFunction::REMQHIQ:    executeREMQHIQ(slot, result); break;
		case PalCallPalFunction::REMQTIQ:    executeREMQTIQ(slot, result); break;
		case PalCallPalFunction::REMQUEL:    executeREMQUEL(slot, result); break;
		case PalCallPalFunction::REMQUEQ:    executeREMQUEQ(slot, result); break;
		case PalCallPalFunction::REMQUEL_D:    executeREMQUEL_D(slot, result); break;
		case PalCallPalFunction::REMQUEQ_D:    executeREMQUEQ_D(slot, result); break;
		case PalCallPalFunction::REMQHILR:    executeREMQHILR(slot, result); break;
		case PalCallPalFunction::REMQTILR:    executeREMQTILR(slot, result); break;
		case PalCallPalFunction::REMHIQR:    executeREMHIQR(slot, result); break;
		case PalCallPalFunction::REMQTIQR:    executeREMQTIQR(slot, result); break;

		case PalCallPalFunction::CLRFEN:   executeCLRFEN(slot, result); break; //PAL
		case PalCallPalFunction::SWASTEN:      executeSWASTEN(slot, result); break;//PAL
		case PalCallPalFunction::WTINT:      executeWTINT(slot, result); break;//PAL
		case PalCallPalFunction::LDQP:      executeLDQP(slot, result); break;//PAL
		case PalCallPalFunction::STQP:    executeSTQP(slot, result); break; // PAL

		default:
			// Proper exception handling - no hard coded vectors!
			ERROR_LOG(QString("CPU %1: Unknown PAL function 0x%2")
				.arg(slot.cpuId)
				.arg(static_cast<quint16>(fn), 4, 16, QChar('0')));
			palUnimplemented(slot, result);
			break;
		}
	}


	// ============================================================================
	// MFPR_ASN  (Move From Processor Register - Address Space Number)
	// ----------------------------------------------------------------------------
	// PAL-privileged instruction.
	//
	// Reads the currently active Address Space Number (ASN) from the
	// execution context and returns it to the caller.
	//
	// Architectural notes:
	// - ASN identifies the current address space for virtual-to-physical
	//   address translation.
	// - ASN participates in TLB tagging and lookup, but MFPR_ASN itself
	//   does not affect TLB state.
	// - MFPR_ASN has no architectural side effects.
	// - No TLB invalidation, epoch update, or translation changes occur.
	// - Execution mode, PC, interrupt state, and AST state are unchanged.
	//
	// References:
	// - Alpha Architecture Reference Manual (EV6), IPR: ASN
	// - PALcode address space management conventions
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_ASN(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->asn);
	}

	AXP_HOT AXP_ALWAYS_INLINE void serviceRD_PS(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->ps);
	}
	AXP_HOT AXP_ALWAYS_INLINE void serviceWR_PS(PipelineSlot& slot, quint64 ps) noexcept
	{
		// PAL semantics: read processor status
		m_iprGlobalMaster->h->ps = ps;
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_FEN(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->fen);
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_FEN(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_iprGlobalMaster->h->fen = readIntReg(slot, 16);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_IPIR(PipelineSlot& slot, PalResult& result) noexcept
	{
		// -------------------------------------------------------------------------
		// Read target CPU mask from R16
		// -------------------------------------------------------------------------
		const quint64 targetCpuMask = readIntReg(slot, 16);
		// -------------------------------------------------------------------------
		// Encode IPI data - IPIR sends a general-purpose interrupt
		// -------------------------------------------------------------------------
		quint64 ipiData = encodeIPIData(IPICommand::CUSTOM, 0);

		DEBUG_LOG(QString("CPU %1: MTPR IPIR - target mask=0x%2")
			.arg(slot.cpuId)
			.arg(targetCpuMask, 16, 16, QChar('0')));

		// -------------------------------------------------------------------------
		// Send IPI to each CPU bit set in mask
		// -------------------------------------------------------------------------
		for (int targetCpu = 0; std::cmp_less(targetCpu, m_cpuCount); ++targetCpu) {
			if (targetCpuMask & (1ULL << targetCpu)) {
				if (std::cmp_not_equal(targetCpu, m_cpuId)) {
					m_ipiManager->postIPI(static_cast<CPUIdType>(targetCpu), ipiData);
				}
			}
		}

		result.hasReturnValue = false;
		result.doesReturn = true;
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_IPL(PipelineSlot& slot, PalResult& result) noexcept
	{
		quint64 currentIPL = m_iprGlobalMaster->h->getIPL();
		result = PalResult::Return(PalReturnReg::R0, currentIPL);
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_IPL(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Get new IPL from R16
		const quint64 newIPL = readIntReg(slot, 16);

		// Set IPL in IRQ controller (masks to 5 bits: 0-31)

		m_iprGlobalMaster->h->setIPL_Unsynced( static_cast<quint8>(newIPL & 0x1F));

		result.hasReturnValue = false;
		result.doesReturn = true;
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_MCES(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->x->mces);
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_MCES(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Get new MCES value from R16
		const quint64 newMces = readIntReg(slot, 16);

		// MCES write behavior (EV6 specific):
		// - Writing 1 to a bit CLEARS it (write-one-to-clear semantics)
		// - Used to clear error status bits after handling
		m_iprGlobalMaster->x->mces &= ~newMces;  // Clear bits where newMces has 1s

		result.hasReturnValue = false;
		result.doesReturn = true;
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_PCBB(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->x->pcbb);
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_PRBR(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->x->prbr);
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_PRBR(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Get new PRBR value from R16
		m_iprGlobalMaster->x->prbr = readIntReg(slot, 16);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_PTBR(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->ptbr);
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_PTBR(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Get new PTBR value from R16
		m_iprGlobalMaster->h->ptbr = readIntReg(slot, 16);

		// CRITICAL: PTBR change invalidates ALL TLB entries
		// (new page tables = all old translations invalid)

		m_tlb->invalidateAllTLBs(slot.cpuId);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_SCBB(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->x->scbb);
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_SCBB(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Get/Update new SCBB value from R16
		m_iprGlobalMaster->x->scbb = readIntReg(slot, 16);

		// SCBB change doesn't require TLB flush
		// (it's just a pointer to the exception vector table)

		result.hasReturnValue = false;
		result.doesReturn = true;
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_SIRR(PipelineSlot& slot, PalResult& result) noexcept
	{
		Q_UNUSED(slot);

		// Read from the active CPU HWPCB (your pointer chain).
		// Ensure `m_iprGlobalMaster` and `h` are valid in your calling contract.
		const quint16 sisr = m_iprGlobalMaster->h->sisr;
		// Return value in R0.
		result = PalResult::Return(PalReturnReg::R0, packSISR_toMFPR(sisr));
	}
	

	AXP_HOT AXP_ALWAYS_INLINE void executeSSIR(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint16 setMask = static_cast<quint16>(readIntReg(slot, 16) & 0xFFFEu);
		m_iprGlobalMaster->h->sisr |= setMask;

		// Raise corresponding pending sources
		for (quint8 lvl = 1; lvl <= 15; ++lvl) {
			if (setMask & (1u << lvl))
				m_pending->raise(static_cast<IrqSourceId>(lvl), lvl);
		}

		result.hasReturnValue = false;
		result.doesReturn = true;
		result.flushPendingTraps();
	}




		// -------------------------------------------------------------------------- -
		// MFPR ASTSR (read-only)
		// ---------------------------------------------------------------------------
		AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_ASTSR(PipelineSlot & slot, PalResult & result) const noexcept
		{
			Q_UNUSED(slot)

			// ASTSR is a 4-bit mask (low nibble). Return it zero-extended.
			const quint8 astsr4 = static_cast<quint8>(m_iprGlobalMaster->h->astsr & 0x0F);

			// MFPR returns value in the configured return register (your convention uses R0).
			result = PalResult::Return(PalReturnReg::R0, static_cast<quint64>(astsr4));
		}

	

	AXP_HOT AXP_ALWAYS_INLINE  void executeMFPR_VPTB(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->x->vptb);
	}

	AXP_HOT AXP_ALWAYS_INLINE  void executeMTPR_VPTB(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Get new VPTB value from R16
		m_iprGlobalMaster->x->vptb = readIntReg(slot, 16);

		// NOTE:
		// VPTB change does NOT require TLB flush (unlike PTBR)
		// VPTB is used by PALcode for software page table walks
		// Hardware TLB uses PTBR (physical page table base)

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	AXP_HOT AXP_ALWAYS_INLINE  void executeMTPR_PERFMON(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_iprGlobalMaster->x->perfmon = readIntReg(slot, slot.di.ra);
	
		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	/*
		MTPR_DATFX is a pure state mutation instruction with no immediate architectural side effects.
		Consumed by: Data translation / fault logic (DTB miss, access violation, etc.) reads:
	*/
	AXP_HOT AXP_ALWAYS_INLINE  void executeMTPR_DATFX(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_iprGlobalMaster->h->datfx = slot.readIntReg(slot.di.ra);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	AXP_HOT AXP_ALWAYS_INLINE  void executeMFPR_DATFX(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->datfx);
	}

	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_WHAMI(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, static_cast<quint64>(slot.cpuId));
	}

	//TODO - incomplete - verify
	AXP_HOT AXP_ALWAYS_INLINE  void executeSWPPAL(PipelineSlot& slot, PalResult& result) noexcept
	{
		// =========================================================================
		// SWPPAL - Swap PALcode
		// =========================================================================
		// Architectural semantics:
		// - Switches from one PALcode image to another
		// - Used for transitions: Firmware PAL <-> OS PAL
		// - Does NOT return to caller
		// - New PAL starts at PAL_BASE + offset from PAL variant table
		//
		// Arguments (in registers):
		// - R16: New PAL variant selector
		//   - 0 = Firmware/Console PAL (SRM)
		//   - 1 = OpenVMS PAL
		//   - 2 = Unix/Tru64 PAL
		//   - 3 = Windows NT PAL
		// - R17-R20: Optional arguments passed to new PAL
		// =========================================================================

		const quint64 palVariant = readIntReg(slot, 16);

		// Validate PAL variant
		if (palVariant > 3) {
			// Invalid PAL variant - raise exception
			PendingEvent ev{};
			ev.kind = PendingEventKind::Exception;
			ev.exceptionClass = ExceptionClass_EV6::OpcDec;  // Or UNIMPL
			m_faultDispatcher->raiseFault(ev);
			result.doesReturn = false;
			return;
		}

		// Save current state (SWPPAL may need to return status)
		// R0 = 0 on success, non-zero on failure
		// Is performed in AlphaCPU

		// Enter PAL mode (in case not already)
		setPalMode(true);
		m_iprGlobalMaster->h->setCM(0);


		// Compute new PAL entry point
		// Typically: PAL_BASE + variant_offset
		const quint64 palBase = m_iprGlobalMaster->x->pal_base;
		const quint64 variantOffset = palVariant * 0x1000;  // 4KB spacing typical
		const quint64 newPalEntry = palBase + variantOffset;
		// Jump to new PAL (PC[0] = 1 for PAL mode)
		m_iprGlobalMaster->h->forcePalPC(canonicalizePalPC(newPalEntry));

		// Set R0 = 0 (success) for new PAL
		writeIntReg(slot, 0, 0);

		// SWPPAL does NOT return to caller
		result.doesReturn = false;
		slot.needsWriteback = false;

		TRACE_LOG(QString("CPU %1: SWPPAL to variant %2, entry = 0x%3")
			.arg(slot.cpuId)
			.arg(palVariant)
			.arg(newPalEntry, 16, 16, QChar('0')));
	}

	// ============================================================================
	// CHMx - Change Mode Instructions
	// ============================================================================
	// These instructions request a mode change:
	// - CHMK: Change to Kernel mode (most privileged)
	// - CHME: Change to Executive mode
	// - CHMS: Change to Supervisor mode
	// - CHMU: Change to User mode (least privileged)
	//
	// Architectural behavior:
	// - Can only change to SAME or LESS privileged mode
	// - Saves PC/PS on new mode's stack
	// - Vectors through System Control Block (SCB)
	// - Does NOT return directly (like syscall)
	// ============================================================================


	AXP_HOT AXP_ALWAYS_INLINE   void executeCHMK(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 savedPC = slot.di.pc + 4;
		const quint64 savedPS = m_iprGlobalMaster->h->ps; 

		// Switch to kernel mode
		m_iprGlobalMaster->h->setCM(0);

		quint64 ksp = m_iprGlobalMaster->h->ksp; 

		// Push exception frame (uses new helpers!)
		if (!m_ev6Translation->pushStack( ksp, savedPS, slot.di.pc, PrivilegeLevel::KERNEL)) {
			result.doesReturn = false;
			return;  // Exception queued
		}

		if (!m_ev6Translation->pushStack( ksp, savedPC, slot.di.pc, PrivilegeLevel::KERNEL)) {
			result.doesReturn = false;
			return;  // Exception queued
		}
		m_iprGlobalMaster->h->ksp = ksp;

		result.doesReturn = false;  // vectors through SCB, doesn't return directly
	}


	AXP_HOT AXP_ALWAYS_INLINE void executeCHME(PipelineSlot& slot, PalResult& result) noexcept {
		const quint64 savedPC = slot.di.pc + 4;
		const quint64 savedPS = m_iprGlobalMaster->h->getPS();

		// Switch to kernel mode
		m_iprGlobalMaster->h->setCM(1);
		quint64 esp = m_iprGlobalMaster->h->esp;

		// Push exception frame (uses new helpers!)
		if (!m_ev6Translation->pushStack( esp, savedPS, slot.di.pc,  PrivilegeLevel::EXECUTIVE)) {
			result.doesReturn = false;
			return;  // Exception queued
		}

		if (!m_ev6Translation->pushStack( esp, savedPC, slot.di.pc,  PrivilegeLevel::EXECUTIVE)) {
			result.doesReturn = false;
			return;  // Exception queued
		}
		m_iprGlobalMaster->h->esp = esp;
		result.doesReturn = false;  // vectors through SCB, doesn't return directly
	}


	AXP_HOT AXP_ALWAYS_INLINE  void executeCHMS(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 savedPC = slot.di.pc + 4;
		const quint64 savedPS = m_iprGlobalMaster->h->ps;
		
		m_iprGlobalMaster->h->setCM(2);

		quint64 ssp = m_iprGlobalMaster->h->ssp;

		// Push exception frame (uses new helpers!)
		if (!m_ev6Translation->pushStack( ssp, savedPS, slot.di.pc, PrivilegeLevel::SUPERVISOR)) {
			result.doesReturn = false;
			return;  // Exception queued
		}

		if (!m_ev6Translation->pushStack( ssp, savedPC, slot.di.pc, PrivilegeLevel::SUPERVISOR)) {
			result.doesReturn = false;
			return;  // Exception queued
		}
		m_iprGlobalMaster->h->ssp = ssp;
	
		result.doesReturn = false;  // vectors through SCB, doesn't return directly
	}


	AXP_HOT AXP_ALWAYS_INLINE   void executeCHMU(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 savedPC = slot.di.pc + 4;
		const quint64 savedPS = m_iprGlobalMaster->h->ps; 

		// Switch to kernel mode
		m_iprGlobalMaster->h->setCM(3);
		quint64 usp = m_iprGlobalMaster->h->usp;

		// Push exception frame (uses new helpers!)
		if (!m_ev6Translation->pushStack( usp, savedPS, slot.di.pc,  PrivilegeLevel::USER)) {
			result.doesReturn = false;
			return;  // Exception queued
		}

		if (!m_ev6Translation->pushStack( usp, savedPC, slot.di.pc,  PrivilegeLevel::USER)) {
			result.doesReturn = false;
			return;  // Exception queued
		}

		m_iprGlobalMaster->h->usp = usp;
		result.doesReturn = false;  // vectors through SCB, doesn't return directly
	}

	// ============================================================================
	// SWPCTX - Swap Process Context
	// ============================================================================
	/**
	 * @brief Swap process context (full context switch)
	 *
	 * This is the heart of process/thread switching on Alpha.
	 * Performs a complete context switch from one process to another.
	 *
	 * Operation:
	 * 1. Save current process state to old HWPCB
	 * 2. Switch PCBB to point to new HWPCB
	 * 3. Load new process state from new HWPCB
	 * 4. Flush TLB for old ASN (if ASN changed)
	 * 5. Update IPL from new process state
	 *
	 * Arguments:
	 * - R16: Physical address of new HWPCB
	 *
	 * Returns:
	 * - R0: Physical address of old HWPCB (for OS to save)
	 *
	 * Side effects:
	 * - Full register state swapped
	 * - ASN changed (triggers TLB flush if different)
	 * - Stack pointers swapped (KSP, ESP, SSP, USP)
	 * - Page table base (PTBR) changed
	 * - IPL restored from new process
	 *
	 * Note: This is PAL-only and used by OS scheduler
	 * // TODO - executeSWPCTX
	 */

	 // ============================================================================
	 // executeSWPCTX - PAL Handler
	 // ============================================================================
	 // Drop-in replacement for the existing SWPCTX handler in PalService.
	 //
	 // This is the thin PAL-layer wrapper. All heavy lifting is in
	 // hwpcbSwapContext() (HWPCB_SwapContext.h).
	 //
	 // Preconditions checked here:
	 //   - R16 alignment (128-byte boundary) -> reserved operand exception
	 //   - Kernel mode (CM == 0)             -> privileged instruction exception
	 //
	 // Post-conditions set here:
	 //   - R0 = old PCBB PA
	 //   - R30 = new KSP (loaded from new HWPCB)
	 //   - PCBB IPR = new PCBB PA
	 //   - Memory barrier + pipeline flush requested
	 //
	 // ============================================================================

	AXP_ALWAYS_INLINE
	void executeSWPCTX(PipelineSlot& slot, PalResult& result) noexcept
	{
		// ================================================================
		// 1. READ R16 (new HWPCB physical address)
		// ================================================================
		const quint64 newPCBB_PA = readIntReg(slot, 16);
	
		// ================================================================
		// 2. ALIGNMENT CHECK: R16<6:0> must be zero
		// ================================================================
		if (newPCBB_PA & HWPCBLayout::ALIGNMENT_MASK) {
			TRACE_LOG(QString("CPU %1: SWPCTX alignment fault, R16=0x%2")
				.arg(slot.cpuId)
				.arg(newPCBB_PA, 16, 16, QChar('0')));

			PendingEvent ev{};
			ev.kind = PendingEventKind::Exception;
			ev.exceptionClass = ExceptionClass_EV6::ReservedOperand;
			m_faultDispatcher->raiseFault(ev);
			result.doesReturn = false;
			return;
		}

		// ================================================================
		// 3. PRIVILEGE CHECK: must be in kernel mode
		// ================================================================
		if (m_iprGlobalMaster->h->cm != 0) {
			TRACE_LOG(QString("CPU %1: SWPCTX privilege violation, CM=%2")
				.arg(slot.cpuId)
				.arg(m_iprGlobalMaster->h->cm));

			PendingEvent ev{};
			ev.kind = PendingEventKind::Exception;
			ev.exceptionClass = ExceptionClass_EV6::OpcDec;
			m_faultDispatcher->raiseFault(ev);
			result.doesReturn = false;
			return;
		}

		// ================================================================
		// 4. GET CURRENT STATE
		// ================================================================
		const quint64 oldPCBB_PA = m_iprGlobalMaster->x->pcbb;
		const quint64 currentR30 = readIntReg(slot, 30);
		const quint64 hwCycleCounter = m_iprGlobalMaster->h->pcc;  // hardware counter

		// ================================================================
		// 5. PERFORM CONTEXT SWITCH
		// ================================================================
		SwapContextResult swapResult = m_iprGlobalMaster->hwpcbSwapContext(
			oldPCBB_PA,	newPCBB_PA,	m_guestMemory, m_iprGlobalMaster->r->cc,
			m_iprGlobalMaster->readInt(30)
		);
		

		if (swapResult.success)
		{
			// Update PCBB
			m_iprGlobalMaster->x->pcbb = newPCBB_PA;

			// R30 = new stack pointer for current mode
			m_iprGlobalMaster->writeInt(30,
				m_iprGlobalMaster->h->loadSP(m_iprGlobalMaster->h->getCM()));

			// R0 = old PCBB
			m_iprGlobalMaster->writeInt(0, swapResult.oldPCBB);

			// Sync IPL from new context
			m_iprGlobalMaster->h->setIPL_Unsynced(m_iprGlobalMaster->h->ipl);

			// ============================================================
			// TLB INVALIDATION — mandatory after context switch
			// ============================================================
			// EV6 TLB entries are ASN-tagged, but:
			//   - PTBR change = completely different page tables
			//   - ASN change  = old non-ASM entries won't match (safe)
			//   - ASN same    = PTBR must also be same (OS contract)
			//
			// If PTBR changed, flush everything (new address space).
			// If only ASN changed, ASN tagging handles it — no flush.
			// Global (ASM) entries survive both cases.
			// ============================================================
			if (swapResult.ptbrChanged)
			{
				m_tlb->invalidateNonASM(m_cpuId);  // flush non-global entries
			}
		}


		if (!swapResult.success) {
			// Should not reach here -- alignment already checked above
			// but defensive coding in case hwpcbSwapContext adds checks
			PendingEvent ev{};
			ev.kind = PendingEventKind::Exception;
			ev.exceptionClass = ExceptionClass_EV6::ReservedOperand;
			m_faultDispatcher->raiseFault(ev);
			result.doesReturn = false;
			return;
		}

		// ================================================================
		// 6. UPDATE CPU STATE
		// ================================================================
		// PCBB IPR now points to new process
		m_iprGlobalMaster->x->pcbb = newPCBB_PA;

		// R30 = new kernel stack pointer
		writeIntReg(slot, 30, m_iprGlobalMaster->h->ksp);

		// Sync IPL from new process context
		m_iprGlobalMaster->h->setIPL_Unsynced( m_iprGlobalMaster->h->ipl);

		// R0 = old PCBB (return value for OS scheduler)
		writeIntReg(slot, 0, swapResult.oldPCBB);

		// ================================================================
		// 7. RETURN
		// ================================================================
		result.doesReturn = true;
		result.memoryBarrier();
		result.requestPipelineFlush(m_iprGlobalMaster->getPC());
		result.clearBranchPredictor();

		TRACE_LOG(QString("CPU %1: SWPCTX old=0x%2 new=0x%3 PTBR_chg=%4 ASN_chg=%5")
			.arg(slot.cpuId)
			.arg(oldPCBB_PA, 16, 16, QChar('0'))
			.arg(newPCBB_PA, 16, 16, QChar('0'))
			.arg(swapResult.ptbrChanged)
			.arg(swapResult.asnChanged));
	}

	// ============================================================================
	// INSQTIL - Insert into Queue at Tail, Interlocked (Longword)
	// ============================================================================
	AXP_HOT AXP_ALWAYS_INLINE  void executeINSQTIL(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 entryAddr = readIntReg(slot, 16);
		const quint64 headerAddr = readIntReg(slot, 17);
		bool isWrite = true;
		quint64 entryPA, headerPA;
		if (m_ev6Translation->translateVA_Data( entryAddr, slot.di.pc,  isWrite, entryPA) != TranslationResult::Success ||
			m_ev6Translation->translateVA_Data( headerAddr, slot.di.pc,  isWrite, headerPA) != TranslationResult::Success) {
			result.doesReturn = false;
			return;
		}



		// Read current tail pointer (blink, offset +4 from header)
		quint32 oldTail;
		quint64 oldTailPA;
		if (m_ev6Translation->translateVA_Data(oldTail, slot.di.pc, true, oldTailPA) != TranslationResult::Success) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}
		if (m_guestMemory->readPA(headerPA + 4, &oldTail, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			result.doesReturn = true;
			return;
		}

		// Link new entry
		quint32 zero = 0;
		if (m_guestMemory->writePA(entryPA, &zero, sizeof(quint32)) != MEM_STATUS::Ok) {  // flink = 0 (end of list)
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Update old tail to point to new entry
		quint32 newTailPtr = static_cast<quint32>(entryAddr & 0xFFFFFFFF);
		if (oldTail != 0) {
			if (m_guestMemory->writePA(oldTail, &newTailPtr, sizeof(quint32)) != MEM_STATUS::Ok) {
				result = PalResult::Return(PalReturnReg::R0, 1);
				result.doesReturn = true;
				return;
			}
		}

		// Update header tail pointer
		if (m_guestMemory->writePA(headerPA + 4, &newTailPtr, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			result.doesReturn = true;
			return;
		}

		result = PalResult::Return(PalReturnReg::R0, 0);
		result.doesReturn = true;
	}

	// ============================================================================
	// INSQHIQ - Insert into Queue at Head, Interlocked (Quadword)
	// ============================================================================
	AXP_HOT AXP_ALWAYS_INLINE void executeINSQHIQ(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 entryAddr = readIntReg(slot, 16);
		const quint64 headerAddr = readIntReg(slot, 17);

		quint64 entryPA, headerPA;
		if (m_ev6Translation->translateVA_Data( entryAddr, slot.di.pc,  true, entryPA) != TranslationResult::Success ||
			m_ev6Translation->translateVA_Data( headerAddr, slot.di.pc,  true, headerPA) != TranslationResult::Success) {
			result.doesReturn = false;
			return;
		}

		// Read current head pointer (quadword)
		quint64 oldHead;
		if (m_guestMemory->readPA(headerPA, &oldHead, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Write old head as flink of new entry
		if (m_guestMemory->writePA(entryPA, &oldHead, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Update header to point to new entry
		if (m_guestMemory->writePA(headerPA, &entryAddr, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		result = PalResult::Return(PalReturnReg::R0, 0);
		result.doesReturn = true;
	}

	// ============================================================================
	// INSQTIQ - Insert into Queue at Tail, Interlocked (Quadword)
	// ============================================================================
	AXP_HOT AXP_ALWAYS_INLINE void executeINSQTIQ(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 entryAddr = readIntReg(slot, 16);
		const quint64 headerAddr = readIntReg(slot, 17);

		quint64 entryPA, headerPA;
		if (m_ev6Translation->translateVA_Data( entryAddr, slot.di.pc,  true, entryPA) != TranslationResult::Success ||
			m_ev6Translation->translateVA_Data( headerAddr, slot.di.pc,  true, headerPA) != TranslationResult::Success) {
			result.doesReturn = false;
			return;
		}

	

		// Read current tail pointer (blink, offset +8 from header)
		quint64 oldTail;
		quint64 oldTailPA;
		if (m_ev6Translation->translateVA_Data(oldTail, slot.di.pc, true, oldTailPA) != TranslationResult::Success) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}
		if (m_guestMemory->readPA(headerPA + 8, &oldTail, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Link new entry (flink = 0, end of list)
		quint64 zero = 0;
		if (m_guestMemory->writePA(entryPA, &zero, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Update old tail to point to new entry
		if (oldTail != 0) {
			if (m_guestMemory->writePA(oldTail, &entryAddr, sizeof(quint64)) != MEM_STATUS::Ok) {
				result = PalResult::Return(PalReturnReg::R0, 1);
				return;
			}
		}

		// Update header tail pointer
		if (m_guestMemory->writePA(headerPA + 8, &entryAddr, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		result = PalResult::Return(PalReturnReg::R0, 0);
		}

	// ============================================================================
	// INSQUEL - Insert into Queue, Unconditional (Longword)
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE void executeINSQUEL(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Similar to INSQHIL but without interlock checking
		// Simplified version - insert at head
		executeINSQHIL(slot, result);
	}

	// ============================================================================
	// INSQUEQ - Insert into Queue, Unconditional (Quadword)
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE  void executeINSQUEQ(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Similar to INSQHIQ but without interlock checking
		executeINSQHIQ(slot, result);
	}

	// ============================================================================
	// INSQUEL_D - Insert into Queue, Unconditional, Deferred (Longword)
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE  void executeINSQUEL_D(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Deferred version - allows pipeline to continue
		// For now, same as INSQUEL
		executeINSQUEL(slot, result);
	}

	// ============================================================================
	// INSQUEQ_D - Insert into Queue, Unconditional, Deferred (Quadword)
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE  void executeINSQUEQ_D(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Deferred version - allows pipeline to continue
		// For now, same as INSQUEQ
		executeINSQUEQ(slot, result);
	}


	// ============================================================================
	// INSQHILR - Insert into Queue at Head, Interlocked, Restartable (Longword)
	// ============================================================================
	/**
	 * @brief Restartable version of INSQHIL
	 *
	 * Can be restarted after a page fault or other interruption.
	 * PALcode maintains state to resume the operation if interrupted.
	 *
	 * For simplified emulation, identical to INSQHIL.
	 * Real hardware would save restart state for fault recovery.
	 *
	 * Arguments:
	 * - R16: Address of new entry to insert
	 * - R17: Address of queue header
	 *
	 * Returns:
	 * - R0: Status (0 = success, 1 = failure)
	 */
	AXP_HOT AXP_ALWAYS_INLINE void executeINSQHILR(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Restartable version - for simplified emulation, same as INSQHIL
		// Real implementation would save restart state
		executeINSQHIL(slot, result);
	}

	// ============================================================================
	// INSQTILR - Insert into Queue at Tail, Interlocked, Restartable (Longword)
	// ============================================================================
	/**
	 * @brief Restartable version of INSQTIL
	 *
	 * Can be restarted after a page fault or other interruption.
	 *
	 * Arguments:
	 * - R16: Address of new entry to insert
	 * - R17: Address of queue header
	 *
	 * Returns:
	 * - R0: Status (0 = success, 1 = failure)
	 */
	AXP_HOT AXP_ALWAYS_INLINE  void executeINSQTILR(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Restartable version - for simplified emulation, same as INSQTIL
		executeINSQTIL(slot, result);
	}

	// ============================================================================
	// INSQHIQR - Insert into Queue at Head, Interlocked, Restartable (Quadword)
	// ============================================================================
	/**
	 * @brief Restartable version of INSQHIQ
	 *
	 * Can be restarted after a page fault or other interruption.
	 *
	 * Arguments:
	 * - R16: Address of new entry to insert
	 * - R17: Address of queue header
	 *
	 * Returns:
	 * - R0: Status (0 = success, 1 = failure)
	 */
	AXP_HOT AXP_ALWAYS_INLINE  void executeINSQHIQR(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Restartable version - for simplified emulation, same as INSQHIQ
		executeINSQHIQ(slot, result);
	}

	// ============================================================================
	// INSQTIQR - Insert into Queue at Tail, Interlocked, Restartable (Quadword)
	// ============================================================================
	/**
	 * @brief Restartable version of INSQTIQ
	 *
	 * Can be restarted after a page fault or other interruption.
	 *
	 * Arguments:
	 * - R16: Address of new entry to insert
	 * - R17: Address of queue header
	 *
	 * Returns:
	 * - R0: Status (0 = success, 1 = failure)
	 */
	AXP_HOT AXP_ALWAYS_INLINE  void executeINSQTIQR(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Restartable version - for simplified emulation, same as INSQTIQ
		executeINSQTIQ(slot, result);
	}

	// ============================================================================
	// REMQHIL - Remove from Queue at Head, Interlocked (Longword)
	// ============================================================================
	/**
	 * @brief Remove entry from head of interlocked queue (longword pointers)
	 *
	 * Atomic operation:
	 *   1. Read current queue head
	 *   2. Read next entry pointer from head entry
	 *   3. Update queue head to point to next entry
	 *   4. Return address of removed entry
	 *
	 * Arguments:
	 * - R16: Address of queue header
	 *
	 * Returns:
	 * - R0: Status
	 *       0 = success, R1 contains address of removed entry
	 *       1 = queue empty (no entry removed)
	 *      -1 = reserved operand fault
	 * - R1: Address of removed entry (if R0 = 0)
	 */

	AXP_HOT AXP_ALWAYS_INLINE  void executeREMQHIL(PipelineSlot& slot, PalResult& result) noexcept
	{


		const quint64 headerAddr = readIntReg(slot, 16);  // Queue header address

		// Translate address
		quint64 headerPA;
		if (m_ev6Translation->translateVA_Data(headerAddr, slot.di.pc, true, headerPA) != TranslationResult::Success) {
			result.doesReturn = false;
			return;
		}

		// Read current head pointer (longword at header)
		quint32 currentHead;
		if (m_guestMemory->readPA(headerPA, &currentHead, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Check if queue is empty
		if (currentHead == 0) {
			setR0(result, 1);  // Queue empty
			writeIntReg(slot, 1, 0);
			result.doesReturn = true;
			return;
		}

		// Translate entry address
		quint64 entryPA;
		if (m_ev6Translation->translateVA_Data( currentHead, slot.di.pc,  true, entryPA) != TranslationResult::Success) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Read next pointer from current head entry
		quint32 nextEntry;
		if (m_guestMemory->readPA(entryPA, &nextEntry, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Update header to point to next entry
		if (m_guestMemory->writePA(headerPA, &nextEntry, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Return removed entry address in R1
		writeIntReg(slot, 1, currentHead);
		setR0(result, 0);  // Success
		result.doesReturn = true;
	}

	// ============================================================================
	// REMQTIL - Remove from Queue at Tail, Interlocked (Longword)
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE void executeREMQTIL(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 headerAddr = readIntReg(slot, 16);

		quint64 headerPA;
		if (m_ev6Translation->translateVA_Data(headerAddr, slot.di.pc,  true, headerPA) != TranslationResult::Success) {
			result.doesReturn = false;
			return;
		}

	

		// Read current tail pointer (blink, offset +4)
		quint32 currentTail;
		if (m_guestMemory->readPA(headerPA + 4, &currentTail, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Check if queue is empty
		if (currentTail == 0) {
			setR0(result, 1);  // Queue empty
			writeIntReg(slot, 1, 0);
			return;
		}

		// Translate entry address
		quint64 entryPA;
		if (m_ev6Translation->translateVA_Data(currentTail, slot.di.pc, true, entryPA) != TranslationResult::Success) {
			result = PalResult::Return(PalReturnReg::R0, 1);

			return;
		}

		// Read previous pointer from current tail entry (blink at offset +4)
		quint32 prevEntry;
		if (m_guestMemory->readPA(entryPA + 4, &prevEntry, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);

			return;
		}

		// Update header tail to point to previous entry
		if (m_guestMemory->writePA(headerPA + 4, &prevEntry, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
	
			return;
		}

		// If there's a previous entry, update its flink to 0 (new tail)
		if (prevEntry != 0) {
			quint64 prevPA;
			if (m_ev6Translation->translateVA_Data( prevEntry, slot.di.pc,  true, prevPA) == TranslationResult::Success) {
				quint32 zero = 0;
		MEM_STATUS memStat =		m_guestMemory->writePA(prevPA, &zero, sizeof(quint32));
			}
		}

		// Return removed entry address in R1
		writeIntReg(slot, 1, currentTail);
		setR0(result, 0);  // Success
		result.doesReturn = true;
	}

	// ============================================================================
	// REMQHIQ - Remove from Queue at Head, Interlocked (Quadword)
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE void executeREMQHIQ(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 headerAddr = readIntReg(slot, 16);

		quint64 headerPA;
		if (m_ev6Translation->translateVA_Data( headerAddr, slot.di.pc,  true, headerPA) != TranslationResult::Success) {
			result.doesReturn = false;
			return;
		}



		// Read current head pointer (quadword)
		quint64 currentHead;
		if (m_guestMemory->readPA(headerPA, &currentHead, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Check if queue is empty
		if (currentHead == 0) {
			setR0(result, 1);  // Queue empty
			writeIntReg(slot, 1, 0);
			result.doesReturn = true;
			return;
		}

		// Translate entry address
		quint64 entryPA;
		if (m_ev6Translation->translateVA_Data( currentHead, slot.di.pc,  true, entryPA) != TranslationResult::Success) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Read next pointer from current head entry
		quint64 nextEntry;
		if (m_guestMemory->readPA(entryPA, &nextEntry, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Update header to point to next entry
		if (m_guestMemory->writePA(headerPA, &nextEntry, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Return removed entry address in R1
		writeIntReg(slot, 1, currentHead);
		setR0(result, 0);  // Success
		result.doesReturn = true;
	}

	// ============================================================================
	// REMQTIQ - Remove from Queue at Tail, Interlocked (Quadword)
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE void executeREMQTIQ(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 headerAddr = readIntReg(slot, 16);

		quint64 headerPA;
		if (m_ev6Translation->translateVA_Data( headerAddr, slot.di.pc,  true, headerPA) != TranslationResult::Success) {
			result.doesReturn = false;
			return;
		}

	

		// Read current tail pointer (blink, offset +8)
		quint64 currentTail;
		if (m_guestMemory->readPA(headerPA + 8, &currentTail, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Check if queue is empty
		if (currentTail == 0) {
			setR0(result, 1);  // Queue empty
			writeIntReg(slot, 1, 0);
			result.doesReturn = true;
			return;
		}

		// Translate entry address
		quint64 entryPA;
		if (m_ev6Translation->translateVA_Data(currentTail, slot.di.pc,  true, entryPA) != TranslationResult::Success) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Read previous pointer from current tail entry (blink at offset +8)
		quint64 prevEntry;
		if (m_guestMemory->readPA(entryPA + 8, &prevEntry, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Update header tail to point to previous entry
		if (m_guestMemory->writePA(headerPA + 8, &prevEntry, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// If there's a previous entry, update its flink to 0 (new tail)
		if (prevEntry != 0) {
			quint64 prevPA;
			if (m_ev6Translation->translateVA_Data( prevEntry, slot.di.pc,  true, prevPA) == TranslationResult::Success) {
				quint64 zero = 0;
			MEM_STATUS memStat0 =	m_guestMemory->writePA(prevPA, &zero, sizeof(quint64));
			}
		}

		// Return removed entry address in R1
		writeIntReg(slot, 1, currentTail);
		setR0(result, 0);  // Success
		result.doesReturn = true;
	}

	// ============================================================================
	// REMQxILR / REMQxIQR - Restart versions
	// ============================================================================
	// These are "restart" versions that can resume after page faults
	// For simplified implementation, we treat them the same as non-restart versions


	AXP_HOT AXP_ALWAYS_INLINE void executeREMQHILR(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeREMQHIL(slot, result);
	}


	AXP_HOT AXP_ALWAYS_INLINE void executeREMQTILR(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeREMQTIL(slot, result);
	}


	AXP_HOT AXP_ALWAYS_INLINE void executeREMHIQR(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeREMQHIQ(slot, result);
	}


	AXP_HOT AXP_ALWAYS_INLINE void executeREMQTIQR(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeREMQTIQ(slot, result);
	}

	// ============================================================================
	// REMQUEL - Remove from Queue, Unconditional (Longword)
	// ============================================================================
	/**
	 * @brief Remove entry from queue, unconditional (longword pointers)
	 *
	 * Unlike REMQHIL/REMQTIL, this removes an arbitrary entry from anywhere
	 * in the queue by unlinking it from its neighbors.
	 *
	 * Arguments:
	 * - R16: Address of entry to remove
	 *
	 * Returns:
	 * - R0: Status
	 *       0 = success
	 *       1 = failure (entry not in queue or error)
	 * - R1: Address of removed entry (same as R16 if success)
	 *
	 * Queue structure (longword):
	 *   Entry: [flink (32-bit)] [blink (32-bit)] [data...]
	 *
	 * Operation:
	 *   entry->flink->blink = entry->blink
	 *   entry->blink->flink = entry->flink
	 */
	AXP_HOT AXP_ALWAYS_INLINE void executeREMQUEL(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 entryAddr = readIntReg(slot, 16);

		// Translate entry address
		quint64 entryPA;
		if (m_ev6Translation->translateVA_Data( entryAddr, slot.di.pc,  true, entryPA) != TranslationResult::Success) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}



		// Read entry's flink (offset +0)
		quint32 flink;
		if (m_guestMemory->readPA(entryPA, &flink, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Read entry's blink (offset +4)
		quint32 blink;
		if (m_guestMemory->readPA(entryPA + 4, &blink, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Check if entry is in a queue (both links non-zero)
		if (flink == 0 && blink == 0) {
			setR0(result, 1);  // Not in queue
			writeIntReg(slot, 1, 0);
			result.doesReturn = true;
			return;
		}

		// Translate flink and blink addresses
		quint64 flinkPA, blinkPA;

		if (flink != 0) {
			if (m_ev6Translation->translateVA_Data(flink, slot.di.pc,  true, flinkPA) != TranslationResult::Success) {
				result = PalResult::Return(PalReturnReg::R0, 1);
				return;
			}

			// flink->blink = entry->blink
			if (m_guestMemory->writePA(flinkPA + 4, &blink, sizeof(quint32)) != MEM_STATUS::Ok) {
				result = PalResult::Return(PalReturnReg::R0, 1);
				return;
			}
		}

		if (blink != 0) {
			if (m_ev6Translation->translateVA_Data( blink, slot.di.pc,  true, blinkPA) != TranslationResult::Success) {
				result = PalResult::Return(PalReturnReg::R0, 1);
				return;
			}

			// blink->flink = entry->flink
			if (m_guestMemory->writePA(blinkPA, &flink, sizeof(quint32)) != MEM_STATUS::Ok) {
				result = PalResult::Return(PalReturnReg::R0, 1);
				return;
			}
		}

		// Clear removed entry's links
		quint32 zero = 0;
		MEM_STATUS stat0 =	m_guestMemory->writePA(entryPA, &zero, sizeof(quint32));      // flink = 0
		MEM_STATUS stat1 =  m_guestMemory->writePA(entryPA + 4, &zero, sizeof(quint32));  // blink = 0

		// Return success
		writeIntReg(slot, 1, entryAddr);  // R1 = removed entry address
		setR0(result, 0);  // Success
		result.doesReturn = true;
	}

	// ============================================================================
	// REMQUEQ - Remove from Queue, Unconditional (Quadword)
	// ============================================================================
	/**
	 * @brief Remove entry from queue, unconditional (quadword pointers)
	 *
	 * Quadword version of REMQUEL.
	 *
	 * Arguments:
	 * - R16: Address of entry to remove
	 *
	 * Returns:
	 * - R0: Status (0 = success, 1 = failure)
	 * - R1: Address of removed entry
	 */
	AXP_HOT AXP_ALWAYS_INLINE void executeREMQUEQ(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 entryAddr = readIntReg(slot, 16);

		// Translate entry address
		quint64 entryPA;
		if (m_ev6Translation->translateVA_Data( entryAddr, slot.di.pc,  true, entryPA) != TranslationResult::Success) {
			setR0(result, 1);
			result.doesReturn = true;
			return;
		}

	

		// Read entry's flink (offset +0)
		quint64 flink;
		if (m_guestMemory->readPA(entryPA, &flink, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Read entry's blink (offset +8)
		quint64 blink;
		if (m_guestMemory->readPA(entryPA + 8, &blink, sizeof(quint64)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Check if entry is in a queue
		if (flink == 0 && blink == 0) {
			setR0(result, 1);  // Not in queue
			writeIntReg(slot, 1, 0);
			result.doesReturn = true;
			return;
		}

		// Translate flink and blink addresses
		quint64 flinkPA, blinkPA;

		if (flink != 0) {
			if (m_ev6Translation->translateVA_Data( flink, slot.di.pc,  true, flinkPA) != TranslationResult::Success) {
				result = PalResult::Return(PalReturnReg::R0, 1);
				return;
			}

			// flink->blink = entry->blink
			if (m_guestMemory->writePA(flinkPA + 8, &blink, sizeof(quint64)) != MEM_STATUS::Ok) {
				result = PalResult::Return(PalReturnReg::R0, 1);
				return;
			}
		}

		if (blink != 0) {
			if (m_ev6Translation->translateVA_Data( blink, slot.di.pc, true, blinkPA) != TranslationResult::Success) {
				result = PalResult::Return(PalReturnReg::R0, 1);
				return;
			}

			// blink->flink = entry->flink
			if (m_guestMemory->writePA(blinkPA, &flink, sizeof(quint64)) != MEM_STATUS::Ok) {
				result = PalResult::Return(PalReturnReg::R0, 1);
				return;
			}
		}

		// Clear removed entry's links
		quint64 zero = 0;
		MEM_STATUS stat0 = m_guestMemory->writePA(entryPA, &zero, sizeof(quint64));      // flink = 0
		MEM_STATUS stat1 = m_guestMemory->writePA(entryPA + 8, &zero, sizeof(quint64));  // blink = 0

		// Return success
		writeIntReg(slot, 1, entryAddr);  // R1 = removed entry address
		setR0(result, 0);  // Success
		result.doesReturn = true;
	}

	// ============================================================================
	// REMQUEL_D - Remove from Queue, Unconditional, Deferred (Longword)
	// ============================================================================
	/**
	 * @brief Deferred version of REMQUEL
	 *
	 * Allows more pipeline parallelism by deferring completion.
	 * For simplified emulation, same as REMQUEL.
	 */
	AXP_HOT AXP_ALWAYS_INLINE void executeREMQUEL_D(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeREMQUEL(slot, result);
	}

	// ============================================================================
	// REMQUEQ_D - Remove from Queue, Unconditional, Deferred (Quadword)
	// ============================================================================
	/**
	 * @brief Deferred version of REMQUEQ
	 *
	 * Allows more pipeline parallelism by deferring completion.
	 * For simplified emulation, same as REMQUEQ.
	 */
	AXP_HOT AXP_ALWAYS_INLINE void executeREMQUEQ_D(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeREMQUEQ(slot, result);
	}

	// ==================================================

	// ============================================================================
	// CLRFEN - Clear Floating-point Enable
	// ============================================================================
	/**
	 * @brief Clear the FEN (Floating-point Enable) bit
	 *
	 * Disables floating-point instructions. Subsequent FP operations will trap
	 * to the operating system for emulation or enabling.
	 *
	 * Arguments: None
	 * Returns: None (R0-R31 unchanged)
	 */

	AXP_HOT AXP_ALWAYS_INLINE void executeCLRFEN(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Clear FEN bit in HWPCB
		m_iprGlobalMaster->h->fen = 0;
		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// ============================================================================
	// SWASTEN - Swap AST Enable
	// ============================================================================
	/**
	 * @brief Swap AST Enable register with value in R16
	 *
	 * Atomically reads current ASTEN and writes new value from R16.
	 * Used for managing AST (Asynchronous System Trap) delivery masks.
	 *
	 * Arguments:
	 * - R16: New ASTEN value (4-bit mask for modes 0-3)
	 *
	 * Returns:
	 * - R0: Old ASTEN value
	 */

	AXP_HOT AXP_ALWAYS_INLINE void executeSWASTEN(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Read new ASTEN value from R16
		const quint32 newAsten = static_cast<quint32>(readIntReg(slot, 16) & 0xF);

		// Atomically read old value and write new value
		const quint32 oldAsten = m_iprGlobalMaster->h->aster;
		m_iprGlobalMaster->h->aster = static_cast<quint8>(newAsten);

		result = PalResult::Return(PalReturnReg::R0, static_cast<quint64>(oldAsten));
	}

	// ============================================================================
	// WTINT - Wait for Interrupt
	// ============================================================================
	/**
	 * @brief Wait for interrupt (enter low-power state)
	 *
	 * Puts the CPU into a low-power wait state until an interrupt occurs.
	 * This is the idle loop primitive used by the OS when no work is available.
	 *
	 * Architectural behavior:
	 * - CPU halts execution until interrupt
	 * - Interrupts are enabled (IPL must allow)
	 * - Returns when interrupt delivered
	 *
	 * Arguments: None
	 * Returns: None (returns when interrupt occurs)
	 */

	AXP_HOT AXP_ALWAYS_INLINE void executeWTINT(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Mark CPU as waiting for interrupt
		// Your emulator should:
		// 1. Set a "halted" or "idle" flag on this CPU
		// 2. Wake up when an interrupt arrives
		// 3. Deliver the interrupt normally

		// For now, simple implementation:
		// Just return immediately (busy-wait equivalent)
		// TODO: Implement actual CPU halt/wake mechanism

		TRACE_LOG(QString("CPU %1: WTINT - waiting for interrupt").arg(slot.cpuId));

		result.hasReturnValue = false;
		result.doesReturn = true;

		// Optional: Could set a flag for the emulator's main loop
		// slot.apc()->setWaitingForInterrupt(true);
	}

	// ============================================================================
	// LDQP - Load Quadword Physical
	// ============================================================================
	/**
	 * @brief Load quadword from physical address (bypass TLB)
	 *
	 * PAL physical mode load - no address translation.
	 * Used for accessing physical memory when VM is off or during PAL setup.
	 *
	 * Arguments:
	 * - R16: Physical address (must be quadword-aligned)
	 *
	 * Returns:
	 * - R0: Data read from memory
	 *
	 * Note: PAL-only, uses physical addressing
	 */

	AXP_HOT AXP_ALWAYS_INLINE void executeLDQP(PipelineSlot& slot, PalResult& result) noexcept
	{
		
		// Read physical address from R16
		const quint64 pa = readIntReg(slot, 16);

		// Alignment check
		if ((pa & 0x7ULL) != 0) {
			PendingEvent ev = makeUnalignedEvent(
				slot.cpuId, pa, false);
			m_faultDispatcher->raiseFault(ev);
			result.doesReturn = false;
			return;
		}

		// Read from physical memory (bypass TLB)
	
		quint64 value;

		if (m_guestMemory->readPA(pa, &value, sizeof(quint64)) != MEM_STATUS::Ok) {
			// Memory error - raise machine check
			PendingEvent ev = makeMachineCheckEvent(
				slot.cpuId,
				MachineCheckReason::IO_BUS_ERROR,
				pa
			);
			m_faultDispatcher->raiseFault(ev);
			result.doesReturn = false;
			return;
		}

		// Return value in R0
		result = PalResult::Return(PalReturnReg::R0, value);
	}

	// ============================================================================
	// STQP - Store Quadword Physical
	// ============================================================================
	/**
	 * @brief Store quadword to physical address (bypass TLB)
	 *
	 * PAL physical mode store - no address translation.
	 * Used for accessing physical memory when VM is off or during PAL setup.
	 *
	 * Arguments:
	 * - R16: Physical address (must be quadword-aligned)
	 * - R17: Data to write
	 *
	 * Returns: None
	 *
	 * Note: PAL-only, uses physical addressing
	 */
	AXP_HOT AXP_ALWAYS_INLINE void executeSTQP(PipelineSlot& slot, PalResult& result) noexcept
	{
		
		// Read physical address from R16
		const quint64 pa = readIntReg(slot, 16);

		// Read data from R17
		const quint64 value = readIntReg(slot, 17);

		// Alignment check
		if ((pa & 0x7ULL) != 0) {

			PendingEvent ev = makeUnalignedEvent(
				slot.cpuId, pa, true);
			m_faultDispatcher->raiseFault(ev);
			result.doesReturn = false;
			return;
		}

		// Write to physical memory (bypass TLB)
		if (m_guestMemory->writePA(pa, &value, sizeof(quint64)) != MEM_STATUS::Ok) {
			// Memory error - raise machine check

			PendingEvent ev = makeMachineCheckEvent(
				slot.cpuId,
				MachineCheckReason::IO_BUS_ERROR,
				pa
			);
			m_faultDispatcher->setPendingEvent(ev);
			result.doesReturn = false;
			return;
		}

		// Invalidate LL/SC reservations for this address
		m_reservationManager->breakReservation(pa);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	



	// ============================================================================
	// MFPR_ESP  (Move From Processor Register - Executive Stack Pointer)
	// ----------------------------------------------------------------------------
	// PAL-privileged instruction.
	//
	// Reads the current Executive Stack Pointer (ESP) value from the
	// active execution context and returns it to the caller.
	//
	// Architectural notes:
	// - ESP is part of the executive-mode context maintained in the HWPCB.
	// - MFPR_ESP has no side effects.
	// - No validation, translation, or memory access is performed.
	// - No execution mode changes occur.
	// - Stack pointer faults (if any) occur only when the value is later used.
	//
	// References:
	// - Alpha Architecture Reference Manual (EV6), IPR: ESP
	// - PALcode context save/restore conventions
	// ============================================================================
	AXP_HOT AXP_ALWAYS_INLINE  void executeMFPR_ESP(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->esp);
	}

	// ============================================================================
	// MFPR_SSP  (Move From Processor Register - Supervisor Stack Pointer)
	// ----------------------------------------------------------------------------
	// PAL-privileged instruction.
	//
	// Reads the current Supervisor Stack Pointer (SSP) value from the
	// active execution context and returns it to the caller.
	//
	// Architectural notes:
	// - SSP is part of the supervisor-mode context maintained in the HWPCB.
	// - MFPR_SSP has no architectural side effects.
	// - No validation, translation, or memory access is performed.
	// - No execution mode changes occur.
	// - Stack pointer faults (if any) occur only when the value is later used
	//   by supervisor-mode execution.
	//
	// References:
	// - Alpha Architecture Reference Manual (EV6), IPR: SSP
	// - PALcode context save/restore conventions
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_SSP(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->ssp);
	}

	// ============================================================================
	// MFPR_USP  (Move From Processor Register - User Stack Pointer)
	// ----------------------------------------------------------------------------
	// PAL-privileged instruction.
	//
	// Reads the current User Stack Pointer (USP) value from the
	// active execution context and returns it to the caller.
	//
	// Architectural notes:
	// - USP is part of the user-mode context maintained in the HWPCB.
	// - MFPR_USP has no architectural side effects.
	// - No validation, translation, or memory access is performed.
	// - No execution mode changes occur.
	// - Stack pointer faults (if any) occur only when the value is later used
	//   by user-mode execution after returning from PAL.
	//
	// References:
	// - Alpha Architecture Reference Manual (EV6), IPR: USP
	// - PALcode context save/restore conventions
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_USP(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->usp);
	}


	// Stack Pointer
	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_ESP(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_iprGlobalMaster->h->esp  = readIntReg(slot, slot.di.ra);
	
		// NOTE:
		// ESP is updated as part of the executive-mode context. No validation or memory
		// access occurs at write time; faults occur only when the stack pointer is later
		// used by executive-mode execution.

		result.hasReturnValue = false;
		result.doesReturn = true;
	}


	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_SSP(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_iprGlobalMaster->h->ssp = readIntReg(slot, slot.di.ra);
	
		// NOTE:
		// SSP is updated as part of the supervisor-mode context. No validation or
		// memory access occurs at write time; faults (if any) occur when the stack
		// pointer is later used by supervisor-mode execution.

		result.hasReturnValue = false;
		result.doesReturn = true;
	}


	AXP_HOT AXP_ALWAYS_INLINE  void executeMTPR_USP(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 newUSP = readIntReg(slot, slot.di.ra);
		m_iprGlobalMaster->h->usp = newUSP;
		// NOTE:
		// USP is updated as part of the user context; no validation or memory access
		// occurs until user-mode execution resumes.

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// =========================================================================
	// MTPR_TBIS - TLB Invalidate Single (Instruction + Data)
	//
	// Alpha Architecture (EV6 / 21264):
	// - PAL-only instruction
	// - Performs an immediate, local invalidation of a single translation
	//   in BOTH the Instruction TLB (ITB) and Data TLB (DTB)
	// - Operates on:
	//     * the current CPU only
	//     * the current ASN
	//     * the virtual address supplied in Ra
	//
	// TBIS is functionally equivalent to executing:
	//     TBISI (invalidate ITB entry)
	//     TBISD (invalidate DTB entry)
	// for the same virtual address and ASN.
	//
	// Architectural ordering:
	// - The invalidate takes effect immediately
	// - Subsequent instruction fetches and data accesses must observe
	//   the updated TLB state
	// - No interrupt, AST, or exception delivery is triggered by this instruction
	//
	// SMP note:
	// - TBIS affects ONLY the local CPU
	// - PALcode must explicitly perform interprocessor coordination
	//   (e.g., IPIs + remote TBIS*) for global shootdowns
	// =========================================================================
	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_TBIS(
		PipelineSlot& slot,
		PalResult& result) noexcept
	{
		
		// -------------------------------------------------------------------------
		// Read the virtual address to invalidate
		// -------------------------------------------------------------------------
		const quint64 va = readIntReg(slot, slot.di.ra);
		// -------------------------------------------------------------------------
		// STEP 1: Local invalidation
		// -------------------------------------------------------------------------
		m_tlb->invalidateTLBEntry(slot.cpuId, Realm::I, va, m_iprGlobalMaster->h->asn);
		m_tlb->invalidateTLBEntry(slot.cpuId, Realm::D, va, m_iprGlobalMaster->h->asn);

		// -------------------------------------------------------------------------
		// STEP 2: Broadcast to all other CPUs with VA encoded in IPI
		// -------------------------------------------------------------------------
		// Encode VA into IPI data (48 bits available)
		quint64 ipiData = encodeIPIWithVA(IPICommand::TLB_INVALIDATE_VA_ITB, va);

		// Broadcast to all other CPUs
		broadcastTLBShootdown(slot.cpuId, ipiData);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	/**
	 * @brief Broadcast TLB shootdown IPI to all CPUs
	 *
	 * @param sourceCpu CPU initiating shootdown
	 * @param ipiData Encoded IPI data (contains command + VA/ASN)
	 */
	AXP_HOT AXP_ALWAYS_INLINE void broadcastTLBShootdown(CPUIdType sourceCpu, quint64 ipiData) const noexcept
	{
		// Send to all CPUs except source
		for (CPUIdType targetCpu = 0; targetCpu < m_cpuCount; ++targetCpu) {
			if (targetCpu != sourceCpu) {
				if (m_ipiManager->postIPI(targetCpu, ipiData)) {
					DEBUG_LOG(QString("CPU %1: Sent TLB shootdown IPI to CPU %2 (data=0x%3)")
						.arg(sourceCpu)
						.arg(targetCpu)
						.arg(ipiData, 16, 16, QChar('0')));
				}
				else {
					ERROR_LOG(QString("CPU %1: Failed to send TLB shootdown IPI to CPU %2")
						.arg(sourceCpu)
						.arg(targetCpu));
				}
			}
		}
	}


	// TLB Invalidation 
	// ============================================================================
	// MTPR_TBIA  (TLB Invalidate All)
	// ----------------------------------------------------------------------------
	// Architectural intent (EV6):
	//   - PAL-only maintenance operation.
	//   - Invalidates all local translations for both ITB and DTB.
	//   - In real hardware this is local-only; SMP shootdown is PAL policy.
	// Emulator functional compromise:
	//   - Perform local invalidation immediately (epoch bump / fast path).
	//   - Schedule IPIs so other CPUs bump their local epochs at recognition points.
	//   - Remote CPUs are NOT invalidated early (avoids "fault too early").
	// ============================================================================

	// ReSharper disable once CppMemberFunctionMayBeStatic
	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_TBIA(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Invalidate ALL TLB entries for this CPU
		// (Both ITB and DTB, all size classes, all ASNs)

		m_tlb->invalidateAllTLBs(slot.cpuId);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}
	// When CPU 0 needs to invalidate TLB on all CPUs:

	// TBIAP = TLB Invalidate All (Process / ASN scoped)
	// Alpha hardware never performs implicit SMP TLB shootdowns. 
	// Remote CPUs must be explicitly invalidated via PAL - orchestrated IPIs.
	// Alpha MTPR TLB invalidation instructions operate only on the local CPU; 
	// SMP TLB shootdown is a PAL-level policy implemented via IPIs.

	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_TBIAP(
		PipelineSlot& slot,
		PalResult& result) noexcept
	{
		
		// -------------------------------------------------------------------------
		// Read ASN to invalidate (from R16 per Alpha convention)
		// -------------------------------------------------------------------------
		const ASNType asn = readIntReg(slot, 16) & 0xFF;  //   Correct method call

		// -------------------------------------------------------------------------
		// STEP 1: Invalidate LOCAL TLB (this CPU)
		// -------------------------------------------------------------------------
		m_tlb->invalidateTLBsByASN(slot.cpuId, asn);

		// -------------------------------------------------------------------------
		// STEP 2: Broadcast to all other CPUs
		// -------------------------------------------------------------------------
		// Encode ASN into IPI data (fits in lower 8 bits)
		quint64 ipiData = encodeIPIData(IPICommand::TLB_INVALIDATE_ASN, static_cast<quint16>(asn));

		broadcastTLBShootdown(slot.cpuId, ipiData);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}



	// =========================================================================
	// MTPR_TBISD - TLB Invalidate Single Data (DTB only)
	//
	// Alpha Architecture (EV6 / 21264):
	// - PAL-only instruction
	// - Performs an immediate, local invalidation of a single DTB entry
	// - Operates on:
	//     * the current CPU only
	//     * the current ASN
	//     * the virtual address supplied in Ra
	// - This is a synchronous TLB maintenance operation
	//
	// Architectural ordering:
	// - The invalidate takes effect immediately
	// - Subsequent data memory accesses must observe the updated DTB state
	// - No interrupt, AST, or exception delivery is triggered by this instruction
	//
	// SMP note:
	// - TBISD does NOT perform a broadcast TLB shootdown
	// - PALcode must explicitly issue IPIs and remote TBIS* operations
	//   to invalidate DTB entries on other CPUs
	// =========================================================================
	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_TBISD(PipelineSlot& slot, PalResult& result) noexcept
	{
		
		// -------------------------------------------------------------------------
		// Read the virtual address to invalidate
		//
		// The architecture supplies a virtual address (VA), not a VPN.
		// Normalization to VPN, page-size class handling, and matching against
		// global (ASN-independent) translations are responsibilities of the
		// TLB/SPAM layer, not this instruction grain.
		// -------------------------------------------------------------------------
		const quint64 va = readIntReg(slot, slot.di.ra);

		// -------------------------------------------------------------------------
		// Determine the active Address Space Number (ASN)
		//
		// TBISD invalidates DTB entries that match:
		//   - the supplied virtual address
		//   - the current ASN
		//   - and any global translations for that VA
		//
		// ASN versus global-entry semantics are handled inside the SPAM/TLB layer.
		// -------------------------------------------------------------------------
		const ASNType asn = m_iprGlobalMaster->h->asn;

		// -------------------------------------------------------------------------
		// Perform the DTB single-entry invalidate
		//
		// This is an immediate, local TLB shootdown affecting the Data TLB only.
		// The effect is synchronous: once this call returns, the DTB no longer
		// contains the invalidated translation for this CPU.
		//
		// This operation is intentionally performed inside the HW_MTPR instruction
		// and is NOT deferred to a later pipeline recognition point.
		// -------------------------------------------------------------------------

		m_tlb->tbisdInvalidate(slot.cpuId, va, asn);

		// -------------------------------------------------------------------------
		// MTPR instructions have no architectural writeback
		// -------------------------------------------------------------------------
		result.hasReturnValue = false;
		result.doesReturn = true;
	}



	// =========================================================================
	// MTPR_TBISI - TLB Invalidate Single Instruction (ITB only)
	//
	// Alpha Architecture (EV6 / 21264):
	// - PAL-only instruction
	// - Performs an immediate, local invalidation of a single ITB entry
	// - Operates on:
	//     * the current CPU only
	//     * the current ASN
	//     * the virtual address supplied in Ra
	// - This is a synchronous maintenance operation, NOT a deferred event
	//
	// Architectural ordering:
	// - The invalidate takes effect immediately
	// - Subsequent instruction fetches must observe the updated ITB state
	// - No interrupt, AST, or fault delivery is triggered by this instruction
	//
	// SMP note:
	// - TBISI does NOT perform a broadcast shootdown
	// - PALcode must explicitly issue IPIs and remote TBIS* operations
	// =========================================================================
	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_TBISI(PipelineSlot& slot, PalResult& result) noexcept
	{

		
		// -------------------------------------------------------------------------
		// Read the virtual address to invalidate
		//
		// The architecture supplies a VA, not a VPN. Normalization to VPN,
		// page-size class handling, and global-entry handling are responsibilities
		// of the TLB/SPAM layer, not the instruction grain.
		// -------------------------------------------------------------------------
		const quint64 va = readIntReg(slot, slot.di.ra);

		// -------------------------------------------------------------------------
		// Determine the active Address Space Number (ASN)
		//
		// TBISI invalidates entries matching:
		//   - the supplied VA
		//   - the current ASN
		//   - and any global (ASN-independent) translations
		//
		// The ASN vs global logic is handled inside the TLB/SPAM implementation.
		// -------------------------------------------------------------------------
		const ASNType asn = m_iprGlobalMaster->h->asn;

		// -------------------------------------------------------------------------
		// Perform the ITB single-entry invalidate
		//
		// This is an immediate, local TLB shootdown for the instruction TLB only.
		// The effect is synchronous: after this call returns, the ITB no longer
		// contains the invalidated translation.
		//
		// This operation is intentionally performed inside the HW_MTPR instruction
		// and is NOT deferred to a later pipeline recognition point.
		// -------------------------------------------------------------------------

		m_tlb->tbisiInvalidate(slot.cpuId, va, asn);

		// -------------------------------------------------------------------------
		// MTPR instructions have no architectural writeback
		// -------------------------------------------------------------------------
		result.hasReturnValue = false;
		result.doesReturn = true;
	}
#pragma endregion CALL_PAL services


	// ============================================================================
	// AMOVRR - Atomic Move Register to Register
	// ============================================================================
	/**
	 * @brief Atomic Move Register to Register
	 *
	 * Architectural behavior:
	 * - Atomically reads R17 and writes to R16
	 * - Used for atomic register swaps in critical sections
	 * - PAL-only instruction
	 * - Typically used for lock acquisition/release
	 *
	 * Arguments:
	 * - R16: Destination register number (0-31)
	 * - R17: Source value
	 *
	 * Returns:
	 * - R0: Old value that was in destination register
	 */
	AXP_HOT AXP_ALWAYS_INLINE void executeAMOVRR(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Read source value from R17
		const quint64 sourceValue = readIntReg(slot, 17);

		// Read destination register number from R16
		quint8 destReg = static_cast<quint8>(readIntReg(slot, 16) & 0x1F);

		// Atomically: read old value, write new value
		const quint64 oldValue = readIntReg(slot, destReg);
		writeIntReg(slot, destReg, sourceValue);

		result = PalResult::Return(PalReturnReg::R0, oldValue);
	}



	AXP_HOT AXP_ALWAYS_INLINE void executeAMOVRM(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Read memory address from R16
		const quint64 va = readIntReg(slot, 16);

		// Read new value from R17
		const quint64 newValue = readIntReg(slot, 17);

		// Translate virtual address (with alignment check)
		quint64 pa;
		
		TranslationResult transResult = m_ev6Translation->translateVA_STQ( va, m_iprGlobalMaster->h->pc, pa);

		if (transResult != TranslationResult::Success) {
			// Exception already queued by translateVA_STQ
			result.doesReturn = false;
			return;
		}

		// Perform atomic exchange via global GuestMemory
		quint64 oldValue;
		if (!m_ev6Translation->atomicExchangePA_Quad( pa, newValue, oldValue)) {
			// Memory error - raise machine check

			PendingEvent ev = makeMachineCheckEvent(
				slot.cpuId,
				MachineCheckReason::IO_BUS_ERROR,
				va
			);
			m_faultDispatcher->raiseFault(ev);
			result.doesReturn = false;
			return;
		}

		// Invalidate LL/SC reservations for this address
		m_reservationManager->breakReservation(pa);
		
		result = PalResult::Return(PalReturnReg::R0, oldValue);
	}


	 // ============================================================================
 // INSQHIL - Insert into Queue at Head, Interlocked (Longword)
 // ============================================================================
 /**
  * @brief Insert entry at head of interlocked queue (longword pointers)
  *
  * Atomic operation:
  *   1. Read current queue head
  *   2. Link new entry to point to old head
  *   3. Update queue head to point to new entry
  *
  * Arguments:
  * - R16: Address of new entry to insert
  * - R17: Address of queue header (contains head pointer)
  *
  * Returns:
  * - R0: 0 = success
  *       1 = failure (queue busy/locked)
  *      -1 = reserved operand fault
  *
  * Queue structure (longword pointers):
  *   Header: [flink (32-bit)] [blink (32-bit)]
  *   Entry:  [flink (32-bit)] [blink (32-bit)] [data...]
  */
	AXP_HOT AXP_ALWAYS_INLINE void executeINSQHIL(PipelineSlot& slot, PalResult& result) noexcept
	{
		
		const quint64 entryAddr = readIntReg(slot, 16);  // New entry address
		const quint64 headerAddr = readIntReg(slot, 17); // Queue header address

		bool isWrite = true;
		// Translate addresses
		quint64 entryPA, headerPA;
		if (m_ev6Translation->translateVA_Data( entryAddr, slot.di.pc,  isWrite, entryPA) != TranslationResult::Success ||
			m_ev6Translation->translateVA_Data( headerAddr, slot.di.pc,  isWrite, headerPA) != TranslationResult::Success) {
			result.doesReturn = false;
			return;
		}


		// Read current head pointer (longword at header)
		quint32 oldHead;
		if (m_guestMemory->readPA(headerPA, &oldHead, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Write old head as flink of new entry
		if (m_guestMemory->writePA(entryPA, &oldHead, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		// Update header to point to new entry (atomic)
		quint32 newHead = static_cast<quint32>(entryAddr & 0xFFFFFFFF);
		if (m_guestMemory->writePA(headerPA, &newHead, sizeof(quint32)) != MEM_STATUS::Ok) {
			result = PalResult::Return(PalReturnReg::R0, 1);
			return;
		}

		result = PalResult::Return(PalReturnReg::R0,0);
	}




// ============================================================================
// WRVPTPTR_OSF - Write Virtual Page Table Pointer (OSF/1)
// ============================================================================
/**
 * @brief Write Virtual Page Table Pointer
 *
 * Sets the virtual address pointer used for software page table walks.
 * OSF/1 uses this for hierarchical page table traversal.
 *
 * Arguments:
 * - R16: Virtual address of page table pointer
 *
 * Returns: None
 */
	AXP_HOT AXP_ALWAYS_INLINE void executeWRVPTPTR_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_iprGlobalMaster->o->vptptr = readIntReg(slot, 16);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

#pragma region CALL_PAL OSF/Tru64 Services
	// ============================================================================
	// OSF/1 (Tru64 Unix) Specific PAL Functions
	// ============================================================================

	// ============================================================================
	// SWPCTX_OSF - Swap Process Context (OSF/1)
	// ============================================================================
	/**
	 * @brief Swap process context - OSF/1 variant
	 *
	 * Same as generic SWPCTX but with OSF/1 specific semantics.
	 * May have slightly different HWPCB layout or save/restore behavior.
	 *
	 * Arguments:
	 * - R16: Physical address of new HWPCB
	 *
	 * Returns:
	 * - R0: Physical address of old HWPCB
	 */
	AXP_HOT AXP_ALWAYS_INLINE  void executeSWPCTX_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		// OSF/1 SWPCTX is identical to generic SWPCTX
		executeSWPCTX(slot, result);
	}

	// ============================================================================
	// WRVAL_OSF - Write Value (OSF/1)
	// ============================================================================
	/**
	 * @brief Write unique process/thread value
	 *
	 * Stores a unique identifier for the current process/thread.
	 * Used by OSF/1 for thread-local storage and process identification.
	 *
	 * Arguments:
	 * - R16: Unique value to write
	 *
	 * Returns: None
	 */
	AXP_HOT AXP_ALWAYS_INLINE  void executeWRVAL_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_iprGlobalMaster->h->unq = readIntReg(slot, 16);
		DEBUG_LOG(QString("CPU %1: WRVAL_OSF = 0x%2")
			.arg(slot.cpuId)
			.arg(m_iprGlobalMaster->h->unq, 16, 16, QChar('0')));

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// ============================================================================
	// RDVAL_OSF - Read Value (OSF/1)
	// ============================================================================
	/**
	 * @brief Read unique process/thread value
	 *
	 * Returns the unique identifier for the current process/thread.
	 *
	 * Arguments: None
	 *
	 * Returns:
	 * - R0: Unique value
	 */
	AXP_HOT AXP_ALWAYS_INLINE  void executeRDVAL_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->unq);
	}

	// ============================================================================
	// TBI_OSF - TB Invalidate (OSF/1)
	// ============================================================================
	/**
	 * @brief TLB invalidate - OSF/1 variant
	 *
	 * Invalidates TLB entries. The specific behavior depends on R16:
	 * - R16 = -2: Invalidate all (TBIA)
	 * - R16 = -1: Invalidate all for current ASN (TBIAP)
	 * - R16 = VA: Invalidate specific VA (TBIS)
	 *
	 * Arguments:
	 * - R16: Invalidation type/address
	 * - R17: Optional ASN (for specific invalidations)
	 *
	 * Returns: None
	 */
	AXP_HOT AXP_ALWAYS_INLINE void executeTBI_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		const qint64 type = static_cast<qint64>(readIntReg(slot, 16));

		if (type == -2) {
			// Invalidate all TLBs
			m_tlb->invalidateAllTLBs(slot.cpuId);
		}
		else if (type == -1) {
			// Invalidate all for current ASN
					m_tlb->invalidateTLBsByASN(slot.cpuId, m_iprGlobalMaster->h->asn);
		}
		else {
			// Invalidate specific VA
			const quint64 va = static_cast<quint64>(type);
			m_tlb->tbisInvalidate(slot.cpuId, va, m_iprGlobalMaster->h->asn);
		}

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// ============================================================================
	// WRENT_OSF - Write System Entry (OSF/1)
	// ============================================================================
	/**
	 * @brief Write system entry point
	 *
	 * Registers exception/interrupt handler entry points with PALcode.
	 * OSF/1 uses this to set up the exception vector table.
	 *
	 * Arguments:
	 * - R16: Entry point address
	 * - R17: Entry type (0=interrupt, 1=arith exception, 2=memory mgmt, etc.)
	 *
	 * Returns: None
	 */

	AXP_HOT AXP_ALWAYS_INLINE void executeWRENT_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 entryPoint = readIntReg(slot, 16);
		const quint64 entryType = readIntReg(slot, 17);

		// Store entry point in appropriate vector slot


		switch (entryType) {
		case 0:  // Interrupt entry
			m_iprGlobalMaster->o->ent_int = entryPoint;
			break;
		case 1:  // Arithmetic exception entry
			m_iprGlobalMaster->o->ent_arith = entryPoint;
			break;
		case 2:  // Memory management entry
			m_iprGlobalMaster->o->ent_mm = entryPoint;
			break;
		case 3:  // Fault entry
			m_iprGlobalMaster->o->ent_fault = entryPoint;
			break;
		case 4:  // Unaligned access entry
			m_iprGlobalMaster->o->ent_una = entryPoint;
			break;
		case 5:  // System call entry
			m_iprGlobalMaster->o->ent_sys = entryPoint;
			break;
		default:
			WARN_LOG(QString("CPU %1: WRENT_OSF unknown type %2")
				.arg(slot.cpuId).arg(entryType));
			break;
		}
		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// ============================================================================
	// SWPIPL_OSF - Swap IPL (OSF/1)
	// ============================================================================
	/**
	 * @brief Swap Interrupt Priority Level
	 *
	 * Atomically reads old IPL and writes new IPL.
	 * This is the primary IPL manipulation function for OSF/1.
	 *
	 * Arguments:
	 * - R16: New IPL value (0-31)
	 *
	 * Returns:
	 * - R0: Old IPL value
	 */
	AXP_HOT AXP_ALWAYS_INLINE void executeSWPIPL_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint8 newIPL = static_cast<quint8>(readIntReg(slot, 16) & 0x1F);

		// Get old IPL
	
		const quint8 oldIPL = m_iprGlobalMaster->h->getIPL();

		// Set new IPL
		m_iprGlobalMaster->h->setIPL_Unsynced( newIPL);

		result = PalResult::Return(PalReturnReg::R0, oldIPL);
		result.iplChanged();
		if (newIPL < oldIPL) result.flushPendingTraps();
	}

	// ============================================================================
	// RDPS_OSF - Read Processor Status (OSF/1)
	// ============================================================================
	/**
	 * @brief Read processor status register
	 *
	 * Returns the current PS (Processor Status) register.
	 *
	 * Arguments: None
	 *
	 * Returns:
	 * - R0: Current PS value
	 */
	AXP_HOT AXP_ALWAYS_INLINE void executeRDPS_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->ps);
	}

	// ============================================================================
	// WRKGP_OSF - Write Kernel Global Pointer (OSF/1)
	// ============================================================================
	/**
	 * @brief Write kernel global pointer
	 *
	 * Sets the kernel global pointer (R29/GP) value.
	 * OSF/1 uses this for kernel data access.
	 *
	 * Arguments:
	 * - R16: New kernel GP value
	 *
	 * Returns: None
	 */

	AXP_HOT AXP_ALWAYS_INLINE void executeWRKGP_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_iprGlobalMaster->o->wrkgp = readIntReg(slot, 16);
		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// ============================================================================
	// WRUSP_OSF - Write User Stack Pointer (OSF/1)
	// ============================================================================
	/**
	 * @brief Write user stack pointer
	 *
	 * Sets the user mode stack pointer.
	 *
	 * Arguments:
	 * - R16: New USP value
	 *
	 * Returns: None
	 */

	AXP_HOT AXP_ALWAYS_INLINE  void executeWRUSP_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_iprGlobalMaster->h->usp =  readIntReg(slot, 16);
		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// ============================================================================
	// WRPERFMON_OSF - Write Performance Monitor (OSF/1)
	// ============================================================================
	/**
	 * @brief Write performance monitor control
	 *
	 * OSF/1 variant of MTPR_PERFMON.
	 *
	 * Arguments:
	 * - R16: Performance monitor control value
	 *
	 * Returns: None
	 */

	AXP_HOT AXP_ALWAYS_INLINE  void executeWRPERFMON_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_iprGlobalMaster->x->perfmon = readIntReg(slot, 16);
		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// ============================================================================
	// RDUSP_OSF - Read User Stack Pointer (OSF/1)
	// ============================================================================
	/**
	 * @brief Read user stack pointer
	 *
	 * Returns the current user mode stack pointer.
	 *
	 * Arguments: None
	 *
	 * Returns:
	 * - R0: Current USP value
	 */

	AXP_HOT AXP_ALWAYS_INLINE void executeRDUSP_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->usp);
	}

	// ============================================================================
	// WHAMI_OSF - Who Am I (OSF/1)
	// ============================================================================
	/**
	 * @brief Return CPU ID
	 *
	 * OSF/1 variant of MFPR_WHAMI.
	 *
	 * Arguments: None
	 *
	 * Returns:
	 * - R0: CPU ID (0 to MAX_CPUS-1)
	 */

	AXP_HOT AXP_ALWAYS_INLINE void executeWHAMI_OSF(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, static_cast<quint64>(slot.cpuId));
	}



#pragma endregion CALL_PAL OSF/Tru64 Services



	AXP_HOT AXP_ALWAYS_INLINE void initialize()
	{
		// Register all PAL handlers
		registerCSERVEConsoleHandlers();
	}

	// ============================================================================
	// REGISTRATION HELPER
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE void writePalArgs(PipelineSlot& slot, const PalArgumentPack& args) noexcept
	{
		// Pal_AlphaProcessorContext currently does this directly.
		// Centralize it here so PAL ABI changes do not ripple into CPU/MBox/APC.
		writeIntReg(slot, 16, args.a0);
		writeIntReg(slot, 17, args.a1);
		writeIntReg(slot, 18, args.a2);
		writeIntReg(slot, 19, args.a3);
		writeIntReg(slot, 20, args.a4);
		writeIntReg(slot, 21, args.a5);
	}

	AXP_HOT AXP_ALWAYS_INLINE void writeIntReg(PipelineSlot& slot, quint8 index, quint64 argValue) const noexcept
	{
		m_iprGlobalMaster->i->write(index, argValue);
	}

	AXP_HOT AXP_ALWAYS_INLINE  void writeIntReg(quint8 index, quint64 argValue) const noexcept
	{
		m_iprGlobalMaster->i->write(index, argValue);
	}

	AXP_HOT AXP_ALWAYS_INLINE quint64 readIntReg(PipelineSlot& slot, quint8 index) const noexcept
	{
		return m_iprGlobalMaster->i->read(index);
	}
	
	AXP_HOT AXP_ALWAYS_INLINE void executeHALT(PipelineSlot& slot, PalResult& result) const noexcept
	{
		// HALT is non-returning (until external restart) but we need to complete sideeffects.
		result.doesReturn = true;

		// Capture context for console/debug UI if you want
		const quint64 haltPC = m_iprGlobalMaster->h->pc;
		const quint64 haltPS = m_iprGlobalMaster->h->ps;

		// Record halt code somewhere canonical (cold IPR or HWPCB field)
		// Keep policy in one place: either ctx has helpers or PalBox commits this.
		m_iprGlobalMaster->r->haltCPU(0); // 0 = normal HALT

   		// PC typically does not advance for HALT (restart re-executes unless console changes PC)
		result.pcModified = true;
		result.newPC = haltPC;

		// If you model PALmode transitions explicitly:
		// result.psModified or setPalMode flags could go here, but keep it consistent.
		result.drainWriteBuffers().flushPendingTraps().notifyHalt().requestPipelineFlush(haltPC);
	}

	// ============================================================================
	// CSERVE ABI Specification
	// ============================================================================
	// R16[7:0]  = Function code (0x01=GETC, 0x02=PUTC, 0x09=PUTS, 0x0C=GETS)
	// R16[63:8] = Reserved (may contain flags in future)
	// R17       = Argument 1 (data/address/flags depending on function)
	// R18       = Argument 2
	// R19       = Argument 3
	// R0        = Return value (function-specific)
	//
	// Function-specific mappings:
	// GETC (0x01): R17=timeout_ms, R0=character (-1 if timeout)
	// PUTC (0x02): R17=character, R0=status (0=success)
	// PUTS (0x09): R17=buffer_va, R18=length, R0=bytes_written
	// GETS (0x0C): R17=buffer_va, R18=maxlen, R19=echo_flag, R0=bytes_read
	// ============================================================================

	// ============================================================================
	// CSERVE ABI Specification
	// ============================================================================
	// Register Usage:
	//   R16[7:0]  = Function code (0x01=GETC, 0x02=PUTC, 0x09=PUTS, 0x0C=GETS)
	//   R16[63:8] = Reserved (may contain flags in future implementations)
	//   R17       = Argument 1 (function-specific)
	//   R18       = Argument 2 (function-specific)
	//   R19       = Argument 3 (function-specific)
	//   R0        = Return value (function-specific, sign-extended)
	//
	// Function-Specific Argument Mappings:
	//
	// GETC (0x01) - Get Character:
	//   R17 = timeout_ms (0=default, UINT32_MAX=infinite)
	//   R0  = character (0-255) or -1 (0xFFFFFFFFFFFFFFFF) on timeout/error
	//
	// PUTC (0x02) - Put Character:
	//   R17 = character (low 8 bits)
	//   R0  = status (0=success, -1=error)
	//
	// PUTS (0x09) - Put String:
	//   R17 = buffer_va (virtual address of string)
	//   R18 = length (number of bytes to write)
	//   R0  = bytes_written (may be less than length if fault occurs)
	//
	// GETS (0x0C) - Get String:
	//   R17 = buffer_va (virtual address of destination buffer)
	//   R18 = maxlen (maximum bytes to read, including null terminator)
	//   R19 = echo_flag (0=no echo, 1=echo characters)
	//   R0  = bytes_read (excluding null terminator)
	//
	// Privilege:
	//   CSERVE requires PAL mode (policy: prevent user-mode console corruption)
	//
	// Faults:
	//   Translation faults (DTB miss, access violation) during PUTS/GETS cause
	//   immediate exception delivery (no return to caller)
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE void executeCSERVE(PipelineSlot& slot, PalResult& result)  noexcept
	{
		// ========================================================================
		// Extract Function Code and Arguments
		// ========================================================================

		quint32 funcCode = static_cast<quint32>(slot.readIntReg(16) & 0xFF);
		quint64 arg1 = slot.readIntReg(17);  // R17
		quint64 arg2 = slot.readIntReg(18);  // R18
		quint64 arg3 = slot.readIntReg(19);  // R19



		TRACE_LOG(QString("CSERVE: func=0x%1 arg1=0x%2 arg2=0x%3 arg3=0x%4")
			.arg(funcCode, 2, 16, QChar('0'))
			.arg(arg1, 16, 16, QChar('0'))
			.arg(arg2, 16, 16, QChar('0'))
			.arg(arg3, 16, 16, QChar('0')));

		// ========================================================================
		// Dispatch to Handler
		// ========================================================================

		switch (funcCode) {

			// ------------------------------------------------------------------------
			// CSERVE 0x01 - GETC (Get Character)
			// ------------------------------------------------------------------------
		case 0x01:  // GETC
		{
			if (!m_consoleManager->isConsoleOpen(0)) {
				WARN_LOG("CSERVE GETC: Console not opened");
				result.returnValue = static_cast<quint64>(-1);
				result.hasReturnValue = true;
				result.returnReg = PalReturnReg::R0;
				break;
			}
			quint32 timeout = static_cast<quint32>(arg1);  // R17 = timeout_ms

			// Get character (blocking with timeout)
			int ch = m_consoleManager->getCharFromOPA(0, true, timeout);

			// Sign-extend result to 64-bit (Alpha convention)
			// 0-255: character received
			// -1 (0xFFFFFFFFFFFFFFFF): timeout or error
			result.returnValue = static_cast<quint64>(static_cast<qint64>(ch));
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;

			TRACE_LOG(QString("CSERVE GETC: returned 0x%1")
				.arg(result.returnValue, 16, 16, QChar('0')));
			break;
		}

		// ------------------------------------------------------------------------
		// CSERVE 0x02 - PUTC (Put Character)
		// ------------------------------------------------------------------------
		case 0x02:  // PUTC
		{
			quint8 ch = static_cast<quint8>(arg1 & 0xFF);  // R17 = character 

			m_consoleManager->putCharToOPA(0, ch);

			result.returnValue = 0;  // Success
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;

			TRACE_LOG(QString("CSERVE PUTC: char=0x%1 ('%2')")
				.arg(ch, 2, 16, QChar('0'))
				.arg(QChar(ch)));
			break;
		}

		case 0x03:  // POLL (check input availability)
		{
			bool hasInput = m_consoleManager->hasInputOnOPA(0);
			result.returnValue = hasInput ? 1 : 0;
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;
			break;
		}


		// ------------------------------------------------------------------------
		// CSERVE 0x07 - CONSOLE_OPEN (Open Console Terminal)
		// ------------------------------------------------------------------------
		case 0x07:  // CONSOLE_OPEN
		{
			int opaIndex = static_cast<int>(arg1);  // R17 = OPA device index

			// Check if device exists and is available
			bool success = m_consoleManager->isAvailable(opaIndex);
			if (success) {
				// Success: R0<63:61> = '000', R0<60:48> = device-specific status
				result.returnValue = 0x0000000000000000ULL;

				INFO_LOG(QString("CSERVE CONSOLE_OPEN: OPA%1 opened successfully")
					.arg(opaIndex));
			}
			else {
				// Failure: R0<63:61> = '100' (bit 61 set)
				// Error code: 0x2000_0000_0000_0000
				result.returnValue = 0x2000000000000000ULL;

				WARN_LOG(QString("CSERVE CONSOLE_OPEN: OPA%1 not available")
					.arg(opaIndex));
			}

			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;
			break;
		}
		// ------------------------------------------------------------------------
		// CSERVE 0x09 - PUTS (Put String)
		// ------------------------------------------------------------------------
		case 0x09:  // PUTS
		{
			quint64 bufferVA = arg1;  // R17 = buffer address ( Virtual address of string )
			quint64 length = arg2;    // R18 = length ( Length in bytes )
			quint64 written = 0;
			bool faulted = false;

			auto* opa0 = m_consoleManager->getOPA(0);
			// Ensure OPA0 is registered/connected
			if (!opa0 || length == 0) {
				result.returnValue = 0;
				result.hasReturnValue = true;
				result.returnReg = PalReturnReg::R0;
				break;
			}


			// Read into temp buffer, then bulk write
			QByteArray tempBuffer;
			for (quint64 i = 0; i < length; i++) {
				quint8 ch;
				if (m_ev6Translation->readVirtualByteFromVA(bufferVA + i, ch) != MEM_STATUS::Ok) {
					faulted = true;
					break;
				}
				tempBuffer.append(ch);
			}

			if (faulted) {
				// Fault was raised - signal pipeline immediately
				slot.faultPending = true;
				result.doesReturn = false;
				result.hasReturnValue = false;
				slot.needsWriteback = false;

#if AXP_INSTRUMENTATION_TRACE
				TRACE_LOG(QString("CSERVE PUTS: faulted after %1 bytes")
					.arg(tempBuffer.size()));
				EXECTRACE_DISCARD_PENDING(                       //  Fault Path
					m_cpuId,
					DiscardReason::FAULT,
					slot.m_pending.isValid() ? slot.m_pending.instrPC : 0);
#endif
				return;  // Early return - don't proceed
			}

			if (!tempBuffer.isEmpty()) {
				written = opa0->putString(
					reinterpret_cast<const quint8*>(tempBuffer.constData()),
					tempBuffer.size()
				);
			}

			// Success - return bytes written
			result.returnValue = written;
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;
#if AXP_INSTRUMENTATION_TRACE
			TRACE_LOG(QString("CSERVE PUTS: wrote %1 bytes").arg(written));
#endif
			break;
		}

		case 0x10:  // TRANSLATE (VA->PA)
		{
			VAType va = arg1;           // R17 = virtual address
			AccessKind mode = static_cast<AccessKind>(arg2);  // R18 = read/write/exec

			PAType pa;
			AlphaPTE pte{};

			TranslationResult tr = m_ev6Translation->ev6TranslateFastVA(va, mode, static_cast<Mode_Privilege>(m_iprGlobalMaster->h->cm), pa, &pte);

			if (tr == TranslationResult::Success) {
				result.returnValue = pa;  // Success: return physical address
			}
			else {
				// Translation failed - raise fault
				raiseTranslationFault(slot.cpuId, va, tr, m_faultDispatcher);
				slot.faultPending = true;
				result.doesReturn = false;
				slot.needsWriteback = false;
				result.hasReturnValue = false;
				return;
			}
			result = PalResult::Return(PalReturnReg::R0, pa);
			break;
		}


		case 0x20:  // GET_ENV
		{
			QString varName;
			if (!readVirtualString(arg1, 256, slot.cpuId, varName)) {
				slot.faultPending = true;
				result.doesReturn = false;
				slot.needsWriteback = false;
				return;
			}

			// Get from SRM environment store
			QString value = m_srmEnvStore->get(varName);

			if (value.isEmpty() && !m_srmEnvStore->exists(varName)) {
				result.returnValue = static_cast<quint64>(-1);
				result.hasReturnValue = true;
				result.returnReg = PalReturnReg::R0;
				break;
			}

			QByteArray valueBytes = value.toUtf8();
			if (static_cast<quint64>(valueBytes.length()) + 1 > arg3) {
				result.returnValue = static_cast<quint64>(-2);  // Buffer too small
				result.hasReturnValue = true;
				result.returnReg = PalReturnReg::R0;
				break;
			}

			quint64 written = m_ev6Translation->writeVirtualBuffer(
			
				arg2,
				reinterpret_cast<const quint8*>(valueBytes.constData()),
				valueBytes.length()
			);

			if (std::cmp_less(written, valueBytes.length())
			) {
				slot.faultPending = true;
				result.doesReturn = false;
				slot.needsWriteback = false;
				return;
			}

			// Write null terminator
			if (m_ev6Translation->writeVirtualByte( arg2 + written, 0) != MEM_STATUS::Ok) {
				slot.faultPending = true;
				result.doesReturn = false;
				slot.needsWriteback = false;
				return;
			}

			result.returnValue = written;
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;

			TRACE_LOG(QString("GET_ENV: %1='%2'").arg(varName).arg(value));
			break;
		}

		case 0x21:  // SET_ENV
		{
			QString varName, varValue;

			if (!readVirtualString(arg1, 256, slot.cpuId, varName) ||
				!readVirtualString(arg2, 1024, slot.cpuId, varValue)) {
				slot.faultPending = true;
				result.doesReturn = false;
				slot.needsWriteback = false;
				return;
			}

			if (varName.isEmpty() || varName.length() > 64) {
				result.returnValue = static_cast<quint64>(-1);
				result.hasReturnValue = true;
				result.returnReg = PalReturnReg::R0;
				break;
			}

			m_srmEnvStore->set(varName, varValue);
			result.returnValue = 0;
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;
			INFO_LOG(QString("SET_ENV: %1='%2'").arg(varName).arg(varValue));
			break;
		}

		case 0x22:  // SAVE_ENV
		{
			m_srmEnvStore->save();
			result.returnValue = 0;
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;
			INFO_LOG("SAVE_ENV: environment saved");
			break;
		}

		case 0x23:  // CLEAR_ENV
		{
			m_srmEnvStore->clear();
			result.returnValue = 0;
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;
			INFO_LOG("CLEAR_ENV: environment reset");
			break;
		}

		// BONUS: TOY Clock operations

		case 0x30:  // GET_TIME
		{
			QDateTime adjustedTime = m_srmEnvStore->getAdjustedTime();
			quint64 secondsSinceEpoch = adjustedTime.toSecsSinceEpoch();

			if (writeVirtualQword(arg1, secondsSinceEpoch, slot.cpuId) != MEM_STATUS::Ok ||
				writeVirtualLongword(arg1 + 8, 0, slot.cpuId) != MEM_STATUS::Ok) {
				slot.faultPending = true;
				result.doesReturn = false;
				slot.needsWriteback = false;
				return;
			}

			result.returnValue = 0;
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;
			TRACE_LOG(QString("GET_TIME: %1").arg(adjustedTime.toString()));
			break;
		}

		case 0x31:  // SET_TIME
		{
			quint64 desiredSeconds;
			if (readVirtualQword(arg1, desiredSeconds, slot.cpuId) != MEM_STATUS::Ok) {
				slot.faultPending = true;
				result.doesReturn = false;
				slot.needsWriteback = false;
				return;
			}

			qint64 offset = static_cast<qint64>(desiredSeconds) -
				QDateTime::currentDateTime().toSecsSinceEpoch();
			m_srmEnvStore->setTimeOffset(offset);

			result.returnValue = 0;
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;
			INFO_LOG(QString("SET_TIME: offset=%1").arg(offset));
			break;
		}

		case 0x32:  // GET_TIME_OFFSET
		{
			result.returnValue = static_cast<quint64>(m_srmEnvStore->getTimeOffset());
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;
			break;
		}

		// ------------------------------------------------------------------------
		// CSERVE 0x0C - GETS (Get String with Line Editing)
		// ------------------------------------------------------------------------
		case 0x0C:  // GETS
		{
			quint64 bufferVA = arg1;
			quint64 maxLen = arg2;
			quint64 flags = arg3;

			bool echo = (flags & 0x01) != 0;
			bool nullTerminate = (flags & 0x02) != 0;

			QByteArray tempBuffer(static_cast<int>(maxLen), 0);

			auto* opa0 = m_consoleManager->getOPA(0);
			quint64 bytesRead = 0;
			quint64 written = 0;  // Declare here (before if block)

			if (opa0) {
				bytesRead = opa0->getString(
					reinterpret_cast<quint8*>(tempBuffer.data()),
					maxLen,
					echo
				);

				// Now just assign (not declare)
				written = m_ev6Translation->writeVirtualBuffer(
					bufferVA,
					reinterpret_cast<const quint8*>(tempBuffer.data()),
					bytesRead
				);

				// Check for fault
				if (written < bytesRead) {
					slot.faultPending = true;
					result.doesReturn = false;
					result.hasReturnValue = false;
					slot.needsWriteback = false;
					return;
				}
			}

			// Now 'written' is in scope
			if (nullTerminate && written < maxLen) {
				if (m_ev6Translation->writeVirtualByte( bufferVA + written, 0) != MEM_STATUS::Ok) {
					slot.faultPending = true;
					result.doesReturn = false;
					result.hasReturnValue = false;
					slot.needsWriteback = false;
					return;
				}
			}

			result.returnValue = written;
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;
			break;
		}

		// ------------------------------------------------------------------------
		// Invalid Function Code
		// ------------------------------------------------------------------------
		default:
		{
			WARN_LOG(QString("CSERVE: Invalid function code 0x%1")
				.arg(funcCode, 2, 16, QChar('0')));

			result.returnValue = static_cast<quint64>(-1);  // Error
			result.hasReturnValue = true;
			result.returnReg = PalReturnReg::R0;
			break;
		}
		}

		// ========================================================================
		// Common Completion
		// ========================================================================

		result.doesReturn = true;
	}

	/*
	D-Cache = Data cache (stores recently accessed memory) - NOT the same as PTE/SPAM
	PTE/SPAM = TLB (translation lookaside buffer) - separate from data cache
	In functional emulation, caches don't exist (memory is always "hit")
	CFLUSH only affects cache lines, not TLB entries
	*/



	AXP_HOT AXP_ALWAYS_INLINE void executeCFLUSH(PipelineSlot& slot, PalResult& result) noexcept
	{
		// =========================================================================
		// CFLUSH - Cache Flush
		// =========================================================================
		// - Drains write buffers
		// - Does NOT affect TLB
		// - Returns normally
		// =========================================================================

		// PAL returns normally
		result.doesReturn = true;
		result.drainWriteBuffers();
		result.pcModified = false;

		slot.needsWriteback = false;
	}




	AXP_HOT AXP_ALWAYS_INLINE void executeDRAINA(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Drain write buffers (Alpha memory ordering primitive)
		result.drainWriteBuffers()
		.memoryBarrier()
		.requestPipelineFlush(m_iprGlobalMaster->h->pc);
		result.doesReturn = true;
		slot.needsWriteback = false;
	}

	AXP_HOT AXP_ALWAYS_INLINE void executeSLEEP(PipelineSlot& slot, PalResult& result) noexcept
	{
		//TODO - we need a grain.
		slot.needsWriteback = false;
	}

	// ============================================================================
	// IMB - Instruction Memory Barrier (Alpha ISA)
	// ============================================================================
	// ISA Reference:
	// IMB guarantees ordering between code stores and subsequent instruction fetch.
	// It invalidates stale instruction cache entries and forces the next fetch to
	// refetch from memory. No data ordering beyond code ordering is guaranteed.
	// ============================================================================

	AXP_HOT AXP_ALWAYS_INLINE  void executeIMB(PipelineSlot& slot, PalResult& result) noexcept
	{
		// ------------------------------------------------------------------------
		// 1. Full compiler + host CPU memory barrier
		// ------------------------------------------------------------------------
		result.memoryBarrier(); // ->syncMemoryBarrier();

		// ------------------------------------------------------------------------
		// 2. Flush front-end / instruction stream
		// ------------------------------------------------------------------------
		result.requestPipelineFlush(slot.di.pc);

		// ------------------------------------------------------------------------
		// 3. Result semantics
		// ------------------------------------------------------------------------
		result.hasReturnValue = false;
		result.doesReturn = true;
		result.pcModified = false;
	

		// ------------------------------------------------------------------------
		// 4. No pipeline writeback
		// ------------------------------------------------------------------------
		slot.needsWriteback = false;
	}

	AXP_HOT AXP_ALWAYS_INLINE	void executePROBER(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 va = readIntReg(slot, 16);

		const ProbeResult pr =
			probeVA(va, /*isWrite=*/false);

		result.hasReturnValue = true;
		result.returnReg = PalReturnReg::R0;
		result.returnValue = static_cast<quint64>(pr);
		result.doesReturn = true;

		slot.needsWriteback = false;
	}

	AXP_HOT AXP_ALWAYS_INLINE	void executePROBEW(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 va = readIntReg(slot, 16);

		const ProbeResult pr =
			probeVA(va, /*isWrite=*/true);

		result.hasReturnValue = true;
		result.returnReg = PalReturnReg::R0;
		result.returnValue = static_cast<quint64>(pr);
		result.doesReturn = true;
		slot.needsWriteback = false;
	}

	AXP_HOT AXP_ALWAYS_INLINE  void executeRD_PS(PipelineSlot& slot, PalResult& result) noexcept
	{
		// ---------------------------------------------------------------------
		// Populate result via shared service
		// ---------------------------------------------------------------------
		serviceRD_PS(slot, result);

		result = PalResult::Return(PalReturnReg::R0, static_cast<quint64>(result.returnValue));
		slot.needsWriteback = false;
	}

	AXP_HOT AXP_ALWAYS_INLINE  void executeWR_PS(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 currentPS = m_iprGlobalMaster->h->ps;
		const quint64 requestedPS = readIntReg(slot, 16);

		m_iprGlobalMaster->setPS(sanitizePS_ForWR_PS(currentPS, requestedPS));
		result.doesReturn = true;
		slot.needsWriteback = false;
	}

	// TODO - not implemented.  - check CBox
	AXP_HOT AXP_ALWAYS_INLINE  void executeRSCC(PipelineSlot& slot, PalResult& result) noexcept
	{
		// RSCC: atomically read cycle counter, then clear it
		auto& cc = m_iprGlobalMaster->r->cc;
		result = PalResult::Return(PalReturnReg::R0, static_cast<quint64>(cc));
		cc = 0;
		slot.needsWriteback = false;
	}

	// READ_UNQ returns the platform "unique" 64-bit value in R0.
	// Backing store policy: HWPCB owns UNQ (stable per-CPU/platform identity).
	// This value persists across PAL transitions and REI. 
	// Refer to the setUNQ_Active regarding computation of UNQ values.

	AXP_HOT AXP_ALWAYS_INLINE   void executeREAD_UNQ(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->unq);

	}
	// WRITE_UNQ updates the platform "unique" value.
		// Value is supplied in R16 per PAL calling convention.
		// Stored in HWPCB active context. 
		// Refer to the setUNQ_Active regarding computation of UNQ values.

	AXP_HOT AXP_ALWAYS_INLINE void executeWRITE_UNQ(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_iprGlobalMaster->h->unq = readIntReg(slot, 16);
	}





	/*
	GENTRAP is CALL_PAL 0xAA (software-defined trap)
	Trap code comes from R16 (Alpha calling convention)
	ExceptionFactory::makeSoftwareTrapEvent() sets:

	exceptionClass = ExceptionClass::GENTRAP
	extraInfo = trapCode (from R16)

	Uses CALL_PAL entry calculation
	OS examines extraInfo to determine trap handler

	*/

	AXP_HOT AXP_ALWAYS_INLINE	void executeGENTRAP(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 trapCode = readIntReg(slot, 16);

		PendingEvent ev =
			makeSoftwareTrapEvent(
				slot.cpuId,
				slot.di.pc,
				trapCode);

		m_faultDispatcher->raiseFault(ev);

		// ---------------------------------------------------------------------
		// PAL semantics
		// ---------------------------------------------------------------------
		result.doesReturn = false;     // control transfers to PAL vector
		result.raisesException = true;

		// ---------------------------------------------------------------------
		// Pipeline semantics
		// ---------------------------------------------------------------------
		slot.needsWriteback = false;
	}
	/*
	  //TODO this is a CALLPAL VEctor
	*/



	AXP_HOT AXP_ALWAYS_INLINE void executeBPT(PipelineSlot& slot, PalResult& result) noexcept
	{
		
		auto& persona = m_iprGlobalMaster->r->pal_personality;
		PalPersonality plat_persona = static_cast<PalPersonality>(persona);
		quint8 instructionIndex = palFunction(slot.di.rawBits());
		PalVectorId_EV6 palVectorId = resolveCallPalVector(instructionIndex);

		PalArgumentPack palArgPack;
		palArgPack.ipl =m_iprGlobalMaster->h->ipl;

		enterPALVector(slot, palVectorId, m_iprGlobalMaster->h->pc, palArgPack);


		result.doesReturn = false;
		slot.needsWriteback = false;
	}





	AXP_HOT AXP_ALWAYS_INLINE  void executeMFPR_TBCHK(PipelineSlot& slot, PalResult& result) noexcept
	{

		// Virtual address to check is in Ra
		const quint64 va = readIntReg(slot, slot.di.ra);

		// Query Data TLB (presence check only)
		PFNType pfn;
		AlphaN_S::PermMask perm;
		SC_Type sizeClass;

		const bool hit = m_tlb->tlbLookup(
			slot.cpuId,
			Realm::D,      // Data TLB only
			va,
			m_iprGlobalMaster->h->asn,
			pfn,
			perm,
			sizeClass
		);
		result = PalResult::Return(PalReturnReg::R0, hit ? 1ULL : 0ULL);

	}


	AXP_HOT AXP_ALWAYS_INLINE  void executeBPT_VMS(PipelineSlot& slot, PalResult& result) noexcept
	{
		// BPT is a CALL_PAL function (VMS uses it for breakpoints/debug entry).
		// The correct emulator model here is:
		//   1) create a CALL_PAL pending event
		//   2) dispatch to PAL via PalService / FaultDispatcher
		// Do NOT synthesize an OS exception frame here.


		PendingEvent ev{};
		ev.kind = PendingEventKind::Exception;
		ev.exceptionClass = ExceptionClass_EV6::CallPal;
		ev.palFunc = static_cast<quint16>(PalCallPalFunction::BPT);  // or your field type
		ev.faultPC = m_iprGlobalMaster->h->pc;

		m_faultDispatcher->raiseFault(ev);

		// CALL_PAL itself does not "write back" via pipeline payload.
		slot.needsWriteback = false;

		// Result: PAL entry will occur; this instruction does not "return" in user mode directly.
		result.doesReturn = false;
	}


	/*
		BUGCHECK is CALL_PAL 0x81 (software trap)
		Maps to OPCDEC vector (0x0400)
		Machine checks are hardware errors, not software traps
		extraInfo = 0x81 identifies it as BUGCHECK vs other illegal opcodes
		*/
	AXP_HOT AXP_ALWAYS_INLINE  void executeBUGCHK(PipelineSlot& slot, PalResult& result) noexcept
	{

		PendingEvent ev = makeCallPalEvent(slot.cpuId, slot.di.pc, 0x81);
		m_faultDispatcher->raiseFault(ev);
		slot.needsWriteback = false;
		result.doesReturn = false;
	}
	/*
			CALLSYS is CALL_PAL 0x83 (standard Unix system call entry)
			Uses dynamic PAL entry calculation (not a named vector)
			Function code 0x83 identifies it as CALLSYS
			PAL entry PC = PAL_BASE + (0x83 * 64) or from CALL_PAL table

			Policy: PAL-format instructions commit architectural results (R0 return, shadow regs, IPR updates)
			directly inside the PAL/MBox dispatcher and do not use PipelineSlot payload writeback.
		*/
	AXP_HOT AXP_ALWAYS_INLINE void executeCALLSYS(PipelineSlot& slot, PalResult& result) noexcept
	{
		// CALLSYS is CALL_PAL 0x83
		PendingEvent ev = makeCallPalEvent(slot.cpuId, slot.di.pc, 0x83);
		m_faultDispatcher->raiseFault(ev);
		slot.needsWriteback = false;
	}

	/*
		Enters PAL at vector 0x0000 (RESET)
		PAL initializes CPU state (IPRs, TLB, caches)
		Sets PC to console firmware entry point
		*/
	AXP_HOT inline void executePALReset(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Sequence of Operations which must be covered.
		// PCST Register
		// --  FPE[2]     // Floating Point Enable - clear FP instruction generate FEN exceptions. Reset by Hardware Reset.
		// ========================================================================
		// 1. IPR_HOT - Reset architectural state
		// ========================================================================
		//	m_iprStorageHot64->reset();  // Clears: PS, PC, IER, ASTEN, ASTSR, SIER, SIRR, etc.

		// ========================================================================
		// 2. IPR_COLD - Reset implementation-specific registers
		// ========================================================================
		//m_iprStorageHotExt->reset();  // Clears: ICCSR, performance counters, caches, etc.

		// Set PAL_BASE to 0 (architectural power-on default)
		m_iprGlobalMaster->x->pal_base = 0;

		// ========================================================================
		// 3. HWPCB - Reset hardware process control block
		// ========================================================================
		// Is performed in AlphaCPU // Clears: KSP, USP, PTBR, ASN, FEN, saved PC/PS

		// ========================================================================
		// 4. TLB State - Invalidate all translations
		// ========================================================================
		m_tlb->invalidateAllTLBs(slot.cpuId);  // Flush ITB + both DTB banks

		// ========================================================================
		// 5. Reservation State - Clear LL/SC reservations
		// ========================================================================
		m_reservationManager->breakReservationsOnCacheLine(slot.cpuId);

		// ========================================================================
		// 6. Pipeline / Pending Events - Clear all pending state
		// ========================================================================

		m_faultDispatcher->clearPendingEvents();

		// ========================================================================
		// 7. Shadow Registers - Reset PAL shadow bank
		// ========================================================================
		//globalIPRHot_RESET(slot.cpuId);  // Clears PAL R0-R24 shadow copies (only R8-25 are used)

		// ========================================================================
		// 8. Branch Predictor - Clear history (if you have per-CPU predictor)
		// -- is routed via the pipeline to CBOX
		// ========================================================================


		// ========================================================================
		// 9. PS - Initialize to clean state
		// ========================================================================
		// PS = 0: Kernel mode, IPL=0, interrupts enabled, VM off
		m_iprGlobalMaster->h->ps = 0;// setPS_Active(slot.cpuId, 0);

		// ========================================================================
		// 10. PC - Set to PAL reset vector
		// ========================================================================
		// Reset vector is at PAL_BASE + 0x0000
		m_iprGlobalMaster->h->pc = m_iprGlobalMaster->x->pal_base;

		// ========================================================================
		// 11. PAL Mode - Enter PAL execution
		// ========================================================================
		setPalMode(true);

		// ========================================================================
		// 12. FP State - Reset floating point (if applicable)
		// ========================================================================
		// Clear FPCR (floating-point control register)
		m_iprGlobalMaster->f->clear();
		m_iprGlobalMaster->i->clear();			// TODO confirm the registers must be cleared on reset.
		m_iprGlobalMaster->x->reset();
		m_iprGlobalMaster->p->clear();
		m_iprGlobalMaster->r->reset();
		// ========================================================================
		// 13. Memory Barriers - Clear any pending write buffers
		// ========================================================================
		result.drainWriteBuffers();

		// ========================================================================
		// 14. MBox-specific state
		// ========================================================================
		// Clear any staged DTB/ITB updates

		slot.needsWriteback = false;
	}
	AXP_HOT AXP_ALWAYS_INLINE void executeReset(PipelineSlot& slot, PalResult& result) noexcept {

		// System Initialization
		m_iprGlobalMaster->h->setIPL_Unsynced( 31);
	}
	AXP_HOT AXP_ALWAYS_INLINE  void final_stage_before_exit(quint64 pc) noexcept
	{
		m_iprGlobalMaster->h->forceUserPC(pc);
	}

	// ============================================================================
		// palUnimplemented - Handle Unimplemented PAL Functions
		// ============================================================================
		/**
		 * @brief Proper handler for unimplemented PAL functions
		 *
		 * Raises OPCDEC (illegal opcode) exception through the standard exception
		 * pathway. Does NOT use hardcoded vectors - lets the exception dispatcher
		 * route through SCB properly.
		 *
		 * @param slot Pipeline slot containing CPU context and fault sink
		 * @param result DecodeInstruction::extractFunction to populate
		 */
	AXP_HOT inline void palUnimplemented(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Extract CPU ID and PAL function code

		const quint64 palFunction = getFunctionCode(slot.di); // or slot.palFunction, depending on your structure
		quint64 faultPc = m_iprGlobalMaster->h->pc;
		// Log the unimplemented PAL call
		WARN_LOG(
			QString("CPU %1: Unimplemented PAL function 0x%2")
			.arg(slot.cpuId)
			.arg(palFunction, 4, 16, QChar('0')));

		// Create OPCDEC exception event
		PendingEvent ev = makeIllegalOpcodeEvent(slot.cpuId, faultPc, slot.di.rawBits());
	
		// Queue exception through slot's fault mechanism (consistent with console operations)
		m_faultDispatcher->setPendingEvent(ev);
		slot.faultPending = true;

		// Mark that this PAL call does not return normally
		result.doesReturn = false;
	}


	AXP_HOT inline	  ProbeResult probeVA(quint64 va, bool isWrite) noexcept
	{
		PFNType pfn;
		AlphaN_S::PermMask perm;

		SC_Type sizeClass = 0;
		// DTB lookup (Realm::D)
		if (!m_tlb->tlbLookup(m_cpuId, Realm::D, va, m_iprGlobalMaster->h->asn, pfn, perm, sizeClass))
		{
			return ProbeResult::NO_MAPPING;
		}

		const quint8 mode = m_iprGlobalMaster->h->cm;

		bool allowed = false;
		if (isWrite) {
			allowed =
				(mode == Kernel && AlphaN_S::canWriteKernel(perm)) ||
				(mode == User && AlphaN_S::canWriteUser(perm));
		}
		else {
			allowed =
				(mode == Kernel && AlphaN_S::canReadKernel(perm)) ||
				(mode == User && AlphaN_S::canReadUser(perm));
		}

		return allowed ? ProbeResult::Ok
			: ProbeResult::NO_PERMISSION;
	}



	/**
	 * @brief Handle DTB miss - PIPELINE EXIT PATTERN
	 *
	 * This handler does NOT walk page tables internally.
	 * Instead, it stages the fault and exits to AlphaPipeline for proper handling.
	 *
	 * Benefits:
	 * - Clean separation: PAL dispatches, Pipeline handles faults
	 * - Proper retry mechanism via pipeline stall/resume
	 * - Reduced call stack depth
	 * - Testable fault handling
	 */
	AXP_HOT AXP_ALWAYS_INLINE  void handleDTBMiss(VAType va, ASNType asn, PipelineSlot& slot) noexcept
	{
		// ========================================================================
		// STAGE 1: Extract context from slot (NO slot.apc() references)
		// ========================================================================

		const bool isWrite = isStore(slot.di);
		const quint64 faultPC = slot.di.pc;

		// ========================================================================
		// STAGE 2: Create fault event using SPAM helpers format
		// ========================================================================
		PendingEvent ev = makeDTBMissSingleEvent(slot.cpuId, va, asn, faultPC, isWrite);
	
		// ========================================================================
		// STAGE 3: Stage fault for pipeline delivery (ARCHITECTURE EXIT POINT)
		// ========================================================================
		m_faultDispatcher->setPendingEvent(ev);
		slot.faultPending = true;

	}

	/**
	 * @brief Handle ITB miss - PIPELINE EXIT PATTERN
	 */
	AXP_HOT AXP_ALWAYS_INLINE   void handleITBMiss(VAType va, ASNType asn, PipelineSlot& slot) noexcept
	{
		const quint64 faultPC = slot.di.pc;

		PendingEvent ev = makeITBMissEvent(slot.cpuId, va);
		ev.asn = asn;
		ev.faultPC = faultPC;

		m_faultDispatcher->setPendingEvent(ev);
		slot.faultPending = true;
	}

	/**
	 * @brief Handle unaligned access - PIPELINE EXIT PATTERN
	 */
	AXP_HOT AXP_ALWAYS_INLINE  void handleUnalignedAccess(VAType va, quint8 accessSize, PipelineSlot& slot) noexcept
	{

		const bool isWrite = isStore(slot.di);

		PendingEvent ev = makeUnalignedEvent(slot.cpuId, va, isWrite);
		ev.extraInfo = accessSize;

		m_faultDispatcher->setPendingEvent(ev);
		slot.faultPending = true;
	}



	/**
	 * @brief Translate virtual address using SPAM helpers (updated format)
	 *
	 * This replaces internal page walking with proper SPAM helper usage.
	 * If translation fails, fault is staged for pipeline handling.
	 */
	AXP_HOT AXP_ALWAYS_INLINE bool translateVA_Updated(
		quint64 va,
		quint8 accessSize,
		bool isWrite,
		PipelineSlot& slot,
		/*out*/ quint64& pa) const noexcept
	{

		const quint64 pc = slot.di.pc;

		// ========================================================================
		// Use SPAM translation helpers (modern format)
		// ========================================================================
		TranslationResult result = m_ev6Translation->translateVA_WithAlignment(
			 va, pc, accessSize,  isWrite, pa);

		if (result != TranslationResult::Success) {
			// Translation failed - fault already staged by SPAM helpers
			// PAL just needs to set pipeline fault flag
			slot.faultPending = true;
			return false;
		}

		return true;
	}

	// ============================================================================
		// CONSOLE MANAGER INTEGRATION - MISSING FUNCTIONS ADDRESSED
		// ============================================================================

		/**
		 * @brief PAL console operations with proper error handling
		 */
	AXP_HOT AXP_ALWAYS_INLINE  void handleConsoleOperation(quint8 operation, PipelineSlot& slot) noexcept
	{
		switch (operation) {
		case 0x01: // GETC
		{
			if (m_consoleManager->isAvailable(0)) {
				int ch = m_consoleManager->getOPA(0)->readChar();
				slot.payLoad = static_cast<quint64>(ch);
				slot.needsWriteback = true;
			}
			else {
			
				PendingEvent ev = makeDeviceNotAvailableEvent(slot.cpuId, 0x01);
				m_faultDispatcher->setPendingEvent(ev);
				slot.faultPending = true;
			}
		}
		break;
		// ========================================================================
		// PUTC (0x02) - Put Character
		// ========================================================================
		case 0x02:
		{
			if (m_consoleManager->isAvailable(0)) {
				// Get character from a0 (R16)
				const quint8 ch = static_cast<quint8>(slot.readIntReg(16) & 0xFF);

				// Write character
				m_consoleManager->getOPA(0)->writeChar(static_cast<char>(ch));

				slot.payLoad = 0; // Success
				slot.needsWriteback = true;
			}
			else {
				// Console not available - stage fault

				PendingEvent ev = makeDeviceNotAvailableEvent(slot.cpuId, 0x02);
				m_faultDispatcher->setPendingEvent(ev);
				slot.faultPending = true;
			}
		}
		break;

		default:

			// Unknown console operation - stage fault
			PendingEvent ev = makeIllegalInstructionEvent(slot.cpuId);
			ev.extraInfo = operation;
			m_faultDispatcher->setPendingEvent(ev);
			slot.faultPending = true;
			break;
		}
	}

	// ============================================================================
	// HELPER: Handle Page Walk Failure
	// ============================================================================

	/**
	 * @brief Handle page walk failure - STAGING ONLY
	 *
	 * Updated to remove slot.apc() references and use slot.apc() pattern.
	 * All fault handling is staged for pipeline, not processed internally.
	 */
	AXP_HOT AXP_ALWAYS_INLINE  void stagePageWalkFailure(const Ev6Translator::WalkResultEV6& walkResult, VAType va, ASNType asn, PipelineSlot& slot) noexcept
	{

		const bool isWrite = isStore(slot.di);

		switch (walkResult.status) {
		case Ev6Translator::WalkStatus::InvalidPTE:
		{
		
			PendingEvent ev = makeInvalidPTE(slot.cpuId, va, walkResult.pte);
			m_faultDispatcher->setPendingEvent(ev);
			slot.faultPending = true;
		}
		break;

		case Ev6Translator::WalkStatus::AccessViolation:
		{
	
			PendingEvent ev = makeAccessViolationFault(slot.cpuId, va, isWrite);
			m_faultDispatcher->setPendingEvent(ev);
			slot.faultPending = true;
		}
		break;

		case Ev6Translator::WalkStatus::PageNotPresent:
		{
	
			PendingEvent ev = makeDTBMissSingleEvent(slot.cpuId, va, asn, slot.di.pc, isWrite);
			m_faultDispatcher->setPendingEvent(ev);
			slot.faultPending = true;
		}
		break;

		case Ev6Translator::WalkStatus::FaultOnRead:
		{
	
			PendingEvent ev = makeFaultOnReadEvent(slot.cpuId, va);
			m_faultDispatcher->setPendingEvent(ev);
			slot.faultPending = true;
		}
		break;

		case Ev6Translator::WalkStatus::FaultOnWrite:
		{
	
			PendingEvent ev = makeFaultOnWriteEvent(slot.cpuId, va);
			m_faultDispatcher->setPendingEvent(ev);
			slot.faultPending = true;
		}
		break;

		case Ev6Translator::WalkStatus::BusError:
		{
	
			PendingEvent ev = makeMemoryFault(slot.cpuId, va);
			m_faultDispatcher->setPendingEvent(ev);
			slot.faultPending = true;
		}
		break;

		default:
		{
			PendingEvent ev;
			ev.kind = PendingEventKind::Exception;
			ev.exceptionClass = ExceptionClass_EV6::MachineCheck;
			ev.faultVA = va;
			ev.faultPC = slot.di.pc;
			ev.asn = asn;
			ev.cm = m_iprGlobalMaster->h->cm;
	
			m_faultDispatcher->setPendingEvent(ev);
			slot.faultPending = true;
		}
		break;
		}

		// NO internal processing - all faults staged for pipeline
	}

	

	AXP_HOT AXP_ALWAYS_INLINE void executeMTPR_DTB_TAG(PipelineSlot& slot, PalResult& result) {

		// 2. Read source register and write to staging register
		quint64 tagValue = readIntReg(slot, slot.di.ra);
		m_iprGlobalMaster->x->dtb_tag = tagValue;  // Direct write - no buffering needed

		// 3. Set up result (MTPR has no return value)
		result.hasReturnValue = false;
		result.doesReturn = true;
		result.flushPendingIPRWrites();
		return;

	}

	AXP_HOT inline void executeMTPR_DTB_PTE(PipelineSlot& slot, PalResult& result) {

		// 2. Read source register and write to staging register
		quint64 pteValue = readIntReg(slot, slot.di.ra);
		m_iprGlobalMaster->x->dtb_pte_temp = pteValue;  // Direct write to staging register

		// 3. Set up result (MTPR has no return value)
		result.hasReturnValue = false;
		result.doesReturn = true;

	}


	AXP_HOT inline void executeMFPR_DTB_TAG(PipelineSlot& slot, PalResult& result) {

		// 1. Flush any pending IPR writes
		slot.palResult.flushPendingIPRWrites();
		quint64 tagValue = m_iprGlobalMaster->x->dtb_tag;

		result = PalResult::Return(PalReturnReg::R0, tagValue);
	}

	AXP_HOT inline void executeTB_FILL_DTB(PipelineSlot& slot) {
		// 1. Flush any pending IPR writes
		slot.palResult.flushPendingIPRWrites();

		// 2. Extract ALL parameters from staged registers
		quint64 dtbTag = m_iprGlobalMaster->x->dtb_tag;
		quint64 va = Ev6Translator::extractVAFromTLBTag(dtbTag);
		ASNType asn = Ev6Translator::extractASNFromTLBTag(dtbTag);  //  From staged DTB_TAG
		quint64 pte = m_iprGlobalMaster->x->dtb_pte_temp;

		// 3. Commit to hardware TLB
		m_tlb->tlbInsert(slot.cpuId, Realm::D, asn, va, pte);

		// 4. Clear staging registers  
		m_iprGlobalMaster->x->dtb_tag = 0;
		m_iprGlobalMaster->x->dtb_pte_temp = 0;
		// Flush any pending IPR writes
		slot.palResult.flushPendingIPRWrites();

	}


	AXP_HOT AXP_ALWAYS_INLINE void setR0(PalResult& r, quint64 v) noexcept
	{
		r.hasReturnValue = true;
		r.returnReg = PalReturnReg::R0;
		r.returnValue = v;
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	AXP_HOT AXP_ALWAYS_INLINE   void  setR1(PalResult& r, quint64 v) noexcept
	{
		// Optional if you ever return 2 values
		// r.hasReturnValue2 = true ... (extend PalResult if needed)
		Q_UNUSED(r); Q_UNUSED(v);
	}

	// ============================================================================
// MISSING PALSERVICE STUBS
// Paste into Pal_Service.h (inside the PalService class body)
//
// Legend:
//   [DELEGATE]  - Forwards to existing _OSF or differently-named method
//   [IMPL]      - Real implementation provided
//   [STUB]      - TODO stub (needs future implementation)
//   [ALIAS]     - Name mismatch bridge to existing method
// ============================================================================

#pragma region Missing PalService Stubs

// ============================================================================
// GROUP 1: DELEGATES TO EXISTING _OSF METHODS
// These are personality-neutral names that route to the OSF implementation.
// When VMS/NT personalities are added, add a switch on pal_personality here.
// ============================================================================

// [DELEGATE] RDPS -> RDPS_OSF
	AXP_HOT AXP_ALWAYS_INLINE void executeRDPS(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeRDPS_OSF(slot, result);
	}

	// [DELEGATE] RDUSP -> RDUSP_OSF
	AXP_HOT AXP_ALWAYS_INLINE void executeRDUSP(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeRDUSP_OSF(slot, result);
	}

	// [DELEGATE] RDVAL -> RDVAL_OSF
	AXP_HOT AXP_ALWAYS_INLINE void executeRDVAL(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeRDVAL_OSF(slot, result);
	}

	// [DELEGATE] SWPIPL -> SWPIPL_OSF
	AXP_HOT AXP_ALWAYS_INLINE void executeSWPIPL(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeSWPIPL_OSF(slot, result);
	}

	// [DELEGATE] TBI -> TBI_OSF
	AXP_HOT AXP_ALWAYS_INLINE void executeTBI(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeTBI_OSF(slot, result);
	}

	// [DELEGATE] WHAMI -> WHAMI_OSF
	AXP_HOT AXP_ALWAYS_INLINE void executeWHAMI(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeWHAMI_OSF(slot, result);
	}

	// [DELEGATE] WRENT -> WRENT_OSF
	AXP_HOT AXP_ALWAYS_INLINE void executeWRENT(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeWRENT_OSF(slot, result);
	}

	// [DELEGATE] WRKGP -> WRKGP_OSF
	AXP_HOT AXP_ALWAYS_INLINE void executeWRKGP(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeWRKGP_OSF(slot, result);
	}

	// [DELEGATE] WRPERFMON -> WRPERFMON_OSF
	AXP_HOT AXP_ALWAYS_INLINE void executeWRPERFMON(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeWRPERFMON_OSF(slot, result);
	}

	// [DELEGATE] WRUSP -> WRUSP_OSF
	AXP_HOT AXP_ALWAYS_INLINE void executeWRUSP(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeWRUSP_OSF(slot, result);
	}

	// [DELEGATE] WRVAL -> WRVAL_OSF
	AXP_HOT AXP_ALWAYS_INLINE void executeWRVAL(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeWRVAL_OSF(slot, result);
	}

	// [DELEGATE] WRVPTPTR -> WRVPTPTR_OSF
	AXP_HOT AXP_ALWAYS_INLINE void executeWRVPTPTR(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeWRVPTPTR_OSF(slot, result);
	}

	// ============================================================================
	// GROUP 2: ALIASES (name mismatch bridges)
	// ============================================================================

	// [ALIAS] WR_PS_SW -> existing executeWR_PS
	AXP_HOT AXP_ALWAYS_INLINE void executeWR_PS_SW(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeWR_PS(slot, result);
	}

	// [ALIAS] RDUNIQUE -> existing executeREAD_UNQ
	AXP_HOT AXP_ALWAYS_INLINE void executeRDUNIQUE(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeREAD_UNQ(slot, result);
	}

	// [ALIAS] WRUNIQUE -> existing executeWRITE_UNQ
	AXP_HOT AXP_ALWAYS_INLINE void executeWRUNIQUE(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeWRITE_UNQ(slot, result);
	}

	// ============================================================================
	// GROUP 3: SIMPLE REGISTER READS - Real implementations
	// ============================================================================

	// [IMPL] RDPSR - Read Processor Status Register
	// Returns full PS in R0 (same as RDPS but different PAL function code)
	AXP_HOT AXP_ALWAYS_INLINE void executeRDPSR(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->ps);
	}

	// [IMPL] RDMCES - Read Machine Check Error Summary
	AXP_HOT AXP_ALWAYS_INLINE void executeRDMCES(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->x->mces);
	}

	// [IMPL] RDPCBB - Read Process Control Block Base (physical address)
	AXP_HOT AXP_ALWAYS_INLINE void executeRDPCBB(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->x->pcbb);
	}

	// [IMPL] RDPER - Read Performance Enable Register
	AXP_HOT AXP_ALWAYS_INLINE void executeRDPER(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->x->perfmon);
	}

	// [IMPL] RDIRQL - Read current Interrupt Request Level (NT terminology for IPL)
	AXP_HOT AXP_ALWAYS_INLINE void executeRDIRQL(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0,
			static_cast<quint64>(m_iprGlobalMaster->h->getIPL()));
	}

	// [IMPL] RDKSP - Read Kernel Stack Pointer
	AXP_HOT AXP_ALWAYS_INLINE void executeRDKSP(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->ksp);
	}

	// [IMPL] RDCOUNTERS - Read Performance Counters
	AXP_HOT AXP_ALWAYS_INLINE void executeRDCOUNTERS(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Return combined counter value (EV6: CC register)
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->r->cc);
	}

	// [IMPL] RDTEB - Read Thread Environment Block (NT-specific)
	// NT stores TEB pointer in the unique value field
	AXP_HOT AXP_ALWAYS_INLINE void executeRDTEB(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->unq);
	}

	// [IMPL] RDTHREAD - Read Thread pointer (NT-specific)
	// NT uses the unique value as thread self pointer
	AXP_HOT AXP_ALWAYS_INLINE void executeRDTHREAD(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->unq);
	}

	// [IMPL] THIS - Read current thread pointer (NT-specific)
	// Same backing store as RDTHREAD/RDTEB
	AXP_HOT AXP_ALWAYS_INLINE void executeTHIS(PipelineSlot& slot, PalResult& result) noexcept
	{
		result = PalResult::Return(PalReturnReg::R0, m_iprGlobalMaster->h->unq);
	}

	// ============================================================================
	// GROUP 4: SIMPLE REGISTER WRITES - Real implementations
	// ============================================================================

	// [IMPL] WRMCES - Write Machine Check Error Summary
	// W1C semantics: writing a 1 to a bit clears it
	AXP_HOT AXP_ALWAYS_INLINE void executeWRMCES(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 value = readIntReg(slot, 16);
		// W1C: clear bits that are written as 1
		m_iprGlobalMaster->x->mces &= ~value;

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// [IMPL] WRPRBR - Write Processor Base Register
	AXP_HOT AXP_ALWAYS_INLINE void executeWRPRBR(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_iprGlobalMaster->x->prbr = readIntReg(slot, 16);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// [IMPL] WRIPIR - Write Interprocessor Interrupt Request
	// Sends IPI to target CPU specified in R16
	AXP_HOT AXP_ALWAYS_INLINE void executeWRIPIR(PipelineSlot& slot, PalResult& result) noexcept
	{
		const int targetCpu = static_cast<int>(readIntReg(slot, 16));
		m_router->raiseIPI(targetCpu);

		if (targetCpu == static_cast<int>(slot.cpuId))
			result.flushPendingTraps();

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// [IMPL] WRFEN - Write Floating-point Enable
	// R16 bit 0: 0=disable FP, 1=enable FP
	AXP_HOT AXP_ALWAYS_INLINE void executeWRFEN(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 value = readIntReg(slot, 16);
		m_iprGlobalMaster->h->fen = (value & 1) ? 1 : 0;

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// ============================================================================
	// GROUP 5: INTERRUPT CONTROL - Real implementations
	// ============================================================================

	// [IMPL] DI - Disable Interrupts
	// Raises IPL to 31 (blocks all interrupts), returns old IPL in R0
	AXP_HOT AXP_ALWAYS_INLINE void executeDI(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint8 oldIPL = m_iprGlobalMaster->getIPL();
		m_iprGlobalMaster->h->setIPL_Unsynced( 31);

		result = PalResult::Return(PalReturnReg::R0, static_cast<quint64>(oldIPL));
		result.iplChanged();
		
	}

	// [IMPL] EI - Enable Interrupts
	// Restores IPL from R16, returns old IPL in R0
	AXP_HOT AXP_ALWAYS_INLINE void executeEI(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint8 newIPL = static_cast<quint8>(readIntReg(slot, 16) & 0x1F);
		const quint8 oldIPL = m_iprGlobalMaster->getIPL();
		m_iprGlobalMaster->h->setIPL_Unsynced(newIPL);

		result = PalResult::Return(PalReturnReg::R0, static_cast<quint64>(oldIPL));
		result.iplChanged();
		if (newIPL < oldIPL) result.flushPendingTraps();
	}

	// [IMPL] CSIR - Clear Software Interrupt Request
	// Clears the software interrupt bits specified in R16
	// Optional: define which SIRR bits are architecturally meaningful in your emulator.
	// If you don't know yet, leave as all-ones and tighten later.
	static constexpr quint64 SIRR_VALID_MASK = ~0ULL;

	// Helper: sanitize SIRR writes so reserved bits remain 0.
	AXP_HOT AXP_ALWAYS_INLINE static quint64 sanitizeSirr(quint64 sirr) noexcept
	{
		return (sirr & SIRR_VALID_MASK);
	}

	AXP_HOT AXP_ALWAYS_INLINE void executeCSIR(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint16 clearMask = static_cast<quint16>(readIntReg(slot, 16) & 0xFFFEu);
		m_iprGlobalMaster->h->sisr &= ~clearMask;

		// Clear corresponding pending sources
		for (quint8 lvl = 1; lvl <= 15; ++lvl) {
			if (clearMask & (1u << lvl))
				m_pending->clear(static_cast<IrqSourceId>(lvl), lvl);
		}

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// Sets software interrupt bits specified in R16
	// SSIR - Set Software Interrupt Request (set specified bits)
	AXP_HOT AXP_ALWAYS_INLINE void executeMFPR_SISR(PipelineSlot& slot, PalResult& result) noexcept
	{
		// SSIR consumes R16 as a set mask.
		const quint64 setMask = readIntReg(slot, 16) & SIRR_VALID_MASK;

		// Canonical read-modify-write via IRQController.
		const quint64 current = sanitizeSirr(m_iprGlobalMaster->h->sirr);
		const quint64 newSirr = sanitizeSirr(current | setMask);

		m_iprGlobalMaster->h->sirr = newSirr;

		// Side-effect PAL call: no return value.
		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// [IMPL] SWPIRQL - Swap IRQL (NT terminology for SWPIPL)
	// Atomically reads old IPL and writes new IPL
	AXP_HOT AXP_ALWAYS_INLINE void executeSWPIRQL(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Same semantics as SWPIPL - NT just uses different name
		executeSWPIPL_OSF(slot, result);
	}

	// ============================================================================
	// GROUP 6: TLB INVALIDATION - Real implementations
	// ============================================================================

	// [IMPL] DTBIS - DTB Invalidate Single
	// Invalidates single DTB entry for VA in R16
	AXP_HOT AXP_ALWAYS_INLINE void executeDTBIS(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 va = readIntReg(slot, 16);
		m_tlb->tbisdInvalidate(slot.cpuId, va, m_iprGlobalMaster->h->asn);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// [IMPL] TBIS - TB Invalidate Single (both ITB and DTB)
	// Invalidates both ITB and DTB entries matching VA in R16
	AXP_HOT AXP_ALWAYS_INLINE void executeTBIS(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 va = readIntReg(slot, 16);
		const ASNType asn = m_iprGlobalMaster->h->asn;

		m_tlb->tbisdInvalidate(slot.cpuId, va, asn);  // DTB
		m_tlb->tbisiInvalidate(slot.cpuId, va, asn);  // ITB

		result.hasReturnValue = false;
		result.doesReturn = true;
	}
	// [IMPL] TBIS - TB Invalidate Single (both ITB and DTB)
	// Invalidates both ITB and DTB entries matching VA in R16
	AXP_HOT AXP_ALWAYS_INLINE void executeTBISD(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 va = readIntReg(slot, 16);
		const ASNType asn = m_iprGlobalMaster->h->asn;

		m_tlb->tbisdInvalidate(slot.cpuId, va, asn);  // DTB

		result.hasReturnValue = false;
		result.doesReturn = true;
	}
	// [IMPL] TBIS - TB Invalidate Single (both ITB and DTB)
	// Invalidates both ITB and DTB entries matching VA in R16
	AXP_HOT AXP_ALWAYS_INLINE void executeTBISI(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 va = readIntReg(slot, 16);
		const ASNType asn = m_iprGlobalMaster->h->asn;

		m_tlb->tbisiInvalidate(slot.cpuId, va, asn);  // DTB

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// [IMPL] TBIA - TB Invalidate All (both ITB and DTB)
	AXP_HOT AXP_ALWAYS_INLINE void executeTBIA(PipelineSlot& slot, PalResult& result) noexcept
	{
		m_tlb->invalidateAllTLBs(slot.cpuId);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// [IMPL] TBISASN - TB Invalidate by ASN
	// Invalidates all TLB entries for ASN in R16
	AXP_HOT AXP_ALWAYS_INLINE void executeTBISASN(PipelineSlot& slot, PalResult& result) noexcept
	{
		const ASNType asn = static_cast<ASNType>(readIntReg(slot, 16) & 0xFF);
		m_tlb->invalidateTLBsByASN(slot.cpuId, asn);

		result.hasReturnValue = false;
		result.doesReturn = true;
	}

	// ============================================================================
	// GROUP 7: CONTEXT / STACK OPERATIONS - Real implementations
	// ============================================================================

	// [IMPL] SWPKSP - Swap Kernel Stack Pointer
	// R16 = new KSP, R0 = old KSP
	AXP_HOT AXP_ALWAYS_INLINE void executeSWPKSP(PipelineSlot& slot, PalResult& result) noexcept
	{
		const quint64 newKSP = readIntReg(slot, 16);
		const quint64 oldKSP = m_iprGlobalMaster->h->ksp;
		m_iprGlobalMaster->h->ksp = newKSP;

		result = PalResult::Return(PalReturnReg::R0, oldKSP);
	}

	// [IMPL] SWPPROCESS - Swap Process (NT variant of SWPCTX)
	// Delegates to generic SWPCTX
	AXP_HOT AXP_ALWAYS_INLINE void executeSWPPROCESS(PipelineSlot& slot, PalResult& result) noexcept
	{
		executeSWPCTX(slot, result);
	}

	// ============================================================================
	// GROUP 8: CONTROL FLOW - Fault/Exception delivery
	// ============================================================================

	// [IMPL] RETSYS - Return from System Call
	// Restores user-mode context saved by CALLSYS
	AXP_HOT AXP_ALWAYS_INLINE void executeRETSYS(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Restore PC from EXC_ADDR
		const quint64 returnPC = m_iprGlobalMaster->h->exc_addr;

		// Exit kernel mode -> user mode
		m_iprGlobalMaster->h->setCM(CM_USER);

		// Restore USP
		// (KSP is implicitly restored when we switch to user mode)

		// Set return PC
		result.pcModified = true;
		result.newPC = returnPC;
		result.doesReturn = false;          // Control transfer, not normal return
		result.requestPipelineFlush(returnPC);
	}

	// [IMPL] RTI - Return from Interrupt
	// Restores PC and PS from stack frame, exits PAL mode
	/*  // NOLINT(clang-diagnostic-invalid-utf8)
	 	

		## The Stacking Behavior

		Now the "descending ordered IPL list" you mentioned works naturally :
	
		User code running at IPL 0
		|
		 Device interrupt at IPL 20
		Push frame(PC, PS with IPL = 0) onto KSP
		IPL = 20, vector to ent_int
		|
		 IPI arrives at IPL 22 (higher than 20, so delivered)
		Push frame(PC, PS with IPL = 20) onto KSP
		IPL = 22, vector to ent_int
		|
		 Timer at IPL 22 (equal, NOT delivered — deferred)
		|
		 IPI handler does REI
		Pop frame -> restore IPL = 20, PC = PC
		kFlushPendingTraps -> reevaluate -> timer at 22 > 20 -> deliver!
		Push frame(PC, PS with IPL = 20) onto KSP
		IPL = 22, vector to ent_int
		|
		 Timer handler does REI
		Pop frame -> restore IPL = 20, PC = PC
		|
		 Device handler does REI
		Pop frame -> restore IPL = 0, PC = PC
		kFlushPendingTraps -> reevaluate -> nothing pending above 0
		Resume user code
		 */
	AXP_HOT AXP_ALWAYS_INLINE void executeRTI(PipelineSlot& slot, PalResult& result) noexcept
	{
		// Pop frame from kernel stack (reverse of push)
		quint64 ksp = m_iprGlobalMaster->h->ksp;

		quint64 restoredPC = 0;
		quint64 restoredPS = 0;

		// Pop in reverse order: PC first (was pushed last), then PS
		if (m_ev6Translation->popStack(ksp, restoredPC, slot.di.pc, PrivilegeLevel::KERNEL) ) {
			// fault during pop — escalate
			result.doesReturn = false;
			return;
		}
		if (!m_ev6Translation->popStack(ksp, restoredPS, slot.di.pc, PrivilegeLevel::KERNEL)) {
			result.doesReturn = false;
			return;
		}
		m_iprGlobalMaster->h->ksp = ksp;

		// Restore processor status from the FRAME, not from IPR
		const quint8 restoredIPL = static_cast<quint8>((restoredPS >> 8) & 0x1F);
		const quint8 restoredCM = static_cast<quint8>(restoredPS & 0x3);

		m_iprGlobalMaster->h->setIPL_Unsynced( restoredIPL);
		m_iprGlobalMaster->h->setCM(restoredCM);
		updateAstEligibility(slot.cpuId);

		// Return to interrupted code
		result.pcModified = true;
		result.newPC = restoredPC & ~0x1ULL;
		result.doesReturn = false;
		result.iplChanged();                           // IPL changed — may unmask interrupts
		result.flushPendingTraps();                    // re-evaluate: another interrupt may be pending
		result.requestPipelineFlush(restoredPC);
	}



	// [IMPL] RFE - Return from Exception
	// Same mechanism as RTI for Alpha
	AXP_HOT AXP_ALWAYS_INLINE void executeRFE(PipelineSlot& slot, PalResult& result) noexcept
	{
		// RFE and RTI have identical semantics on Alpha EV6
		executeRTI(slot, result);
	}

	// [IMPL] CALLKD - Call Kernel Delivery (VMS)
	// Vectors to kernel AST delivery entry point
	AXP_HOT AXP_ALWAYS_INLINE void executeCALLKD(PipelineSlot& slot, PalResult& result) noexcept
	{
		// TODO: Full VMS kernel delivery implementation
		// For now, vector through SCB like other CALL_PAL functions
		PendingEvent ev = makeCallPalEvent(slot.cpuId, slot.di.pc, 0x0D);  // CALLKD function code
		m_faultDispatcher->raiseFault(ev);

		result.doesReturn = false;
		slot.needsWriteback = false;
	}

	// [IMPL] KBPT - Kernel Breakpoint
	// Identical to BPT but from kernel mode
	AXP_HOT AXP_ALWAYS_INLINE void executeKBPT(PipelineSlot& slot, PalResult& result) noexcept
	{
		PendingEvent ev{};
		ev.kind = PendingEventKind::Exception;
		ev.exceptionClass = ExceptionClass_EV6::CallPal;
		ev.palFunc = static_cast<quint16>(PalCallPalFunction::KBPT);
		ev.faultPC = m_iprGlobalMaster->h->pc;

		m_faultDispatcher->raiseFault(ev);

		result.doesReturn = false;
		slot.needsWriteback = false;
	}

	// ============================================================================
	// GROUP 9: QUEUE OPERATIONS - Missing variant
	// ============================================================================

	// [STUB] REMQUE_UD - Remove from Unaligned Doubly-linked Queue
	AXP_HOT AXP_ALWAYS_INLINE void executeREMQUE_UD(PipelineSlot& slot, PalResult& result) noexcept
	{
		// TODO: Implement unaligned queue remove
		// For now, delegate to aligned version
		WARN_LOG(QString("CPU %1: REMQUE_UD not fully implemented, using aligned path")
			.arg(slot.cpuId));
		executeREMQUEQ(slot, result);
	}

	// ============================================================================
	// GROUP 10: SYSTEM CONTROL - Stubs for complex operations
	// ============================================================================

	// [IMPL] INITPAL - Initialize PAL environment
	// Resets PAL state without full CPU reset
	// Implements a PAL INITPAL-style routine for emulator bring-up.
	//
	// Contract (emulator-side, minimal safe behavior):
	//  1) Clear all pending fault/trap events so PAL init starts from a clean state.
	//  2) Reset PAL-visible interrupt / AST state:
	//        - SIRR = 0 (no software interrupts requested)
	//        - ASTER = 0 (disable AST delivery for all modes)
	//        - ASTSR = 0 (clear AST summary for all modes)
	//  3) Invalidate ITB/DTB (TLB) state so subsequent translations refault/refill.
	//  4) Return to caller, and request a pipeline flush at the current PC.
	//
	// Notes / correctness considerations:
	//  - INITPAL is platform/firmware-defined; ASA specifies mechanisms, but specific
	//    PAL services are implementation-specific. This is a conservative and useful
	//    emulator contract.
	//  - Prefer slot.cpuId everywhere (do not mix with m_cpuId) to stay SMP-correct.
	//  - If you model "PAL mode" via PC bit0, you may choose to force PAL mode here.
	//    (Some implementations enter PAL already before calling INITPAL; keep your
	//    state machine consistent.)
	//  - If your TLB invalidation is split by ITB/DTB, call both.
	//
	// Source reference:
	//  - Use the SRM/PAL specification for INITPAL semantics on your target platform.
	//  - ASA v6 (1994) describes TB invalidation mechanisms and privileged state
	//    transitions; specific INITPAL behavior is not universally architectural.


	AXP_HOT AXP_ALWAYS_INLINE void executeINITPAL(PipelineSlot& slot, PalResult& result) noexcept
	{
		// --------------------------------------------------------------------
		// 1) Clear pending PAL/fault events (emulator event queue)
		// --------------------------------------------------------------------
		m_faultDispatcher->clearPendingEvents();

		// --------------------------------------------------------------------
		// 2) Reset PAL-specific interrupt/AST state
		// --------------------------------------------------------------------
		// SMP correctness: use slot.cpuId, not m_cpuId.
		m_iprGlobalMaster->h->sirr =  0ULL;

		// Clear AST enable/summary (4-bit fields: K/E/S/U)
		m_iprGlobalMaster->h->aster = 0;
		m_iprGlobalMaster->h->astsr = 0;

		// Optional: clear SISR shadow if you maintain one in HWPCB
		// m_iprGlobalMaster->h->sisr = 0; // only if sisr is used as a live register in your model

		// --------------------------------------------------------------------
		// 3) Invalidate translation buffers (ITB/DTB)
		// --------------------------------------------------------------------
		// Ensure this invalidates both I and D translation structures.
		m_tlb->invalidateAllTLBs(slot.cpuId);

		// --------------------------------------------------------------------
		// 4) Return with pipeline flush
		// --------------------------------------------------------------------
		result.hasReturnValue = false;
		result.doesReturn = true;

		// Flush to the current PC. If your PAL mode is PC bit0, and you want to
		// preserve PAL mode, do NOT clear bit0 here.
		const quint64 pc = m_iprGlobalMaster->getPC();
		result.requestPipelineFlush(pc);
	}


	// [IMPL] REBOOT - System reboot
	// Initiates full system reset
	AXP_HOT AXP_ALWAYS_INLINE void executeREBOOT(PipelineSlot& slot, PalResult& result) noexcept
	{
		INFO_LOG(QString("CPU %1: REBOOT requested").arg(slot.cpuId));

		// Delegate to PAL reset sequence
		executePALReset(slot, result);

		result.doesReturn = false;

		result.notifyHalt().requestPipelineFlush(m_iprGlobalMaster->getPC());
		
	}

	// [IMPL] RESTART - Restart from halt
	AXP_HOT AXP_ALWAYS_INLINE void executeRESTART(PipelineSlot& slot, PalResult& result) noexcept
	{
		INFO_LOG(QString("CPU %1: RESTART requested").arg(slot.cpuId));

		// Set PC to restart vector (console entry)
		result.pcModified = true;
		result.newPC = m_iprGlobalMaster->x->pal_base;  // PAL reset vector
		result.doesReturn = false;
		result.requestPipelineFlush(m_iprGlobalMaster->x->pal_base);
		//result.haltCode(false);
	}

#pragma endregion Missing PalService Stubs

	/*

		The key architectural difference from OSF / 1:
	```
		VMS : PAL reads SCB -> vectors directly to device handler
		The SCB IS the dispatch table
		Each device has its own SCB entry with its own handler PC

		OSF / 1 : PAL vectors to ent_int -> OS dispatches from there
		The SCB may exist but OS interprets it
		Single entry point, OS uses vector parameter to dispatch

	*/
	// In PalService:
	AXP_HOT AXP_ALWAYS_INLINE void deliverInterrupt(
		const ClaimedInterrupt& claimed) noexcept
	{
		// Interrupted PC is simply the current PC (next instruction not yet fetched)
		const quint64 savedPC = m_iprGlobalMaster->h->pc;
		const quint64 savedPS = m_iprGlobalMaster->h->ps;

		m_iprGlobalMaster->h->setCM(CM_KERNEL);

		quint64 ksp = m_iprGlobalMaster->h->ksp;
		if (!m_ev6Translation->pushStack(ksp, savedPS, savedPC, PrivilegeLevel::KERNEL))
			return;
		if (!m_ev6Translation->pushStack(ksp, savedPC, savedPC, PrivilegeLevel::KERNEL))
			return;
		m_iprGlobalMaster->h->ksp = ksp;

		m_iprGlobalMaster->h->setIPL_Unsynced(claimed.ipl);

		if (m_palVariant == GrainPlatform::VMS) {
			const quint64 entryPA = m_iprGlobalMaster->x->scbb + (claimed.vector & 0xFFFF);
			quint64 handlerPC = 0, handlerParam = 0;
			if (m_guestMemory->read64(entryPA, handlerPC) != MEM_STATUS::Ok)
				return;
			m_guestMemory->read64(entryPA + 8, handlerParam);

			// TODO comment for VMS device interrupts that use disposition 01 (interrupt stack).
			auto decoded = decodeScbHandler(handlerPC);
			writeIntReg(4, handlerParam);
			m_iprGlobalMaster->h->pc = decoded.handlerPc | 0x1; // PAL mode bit
		}
		else {
			writeIntReg(16, claimed.vector);
			writeIntReg(17, static_cast<quint64>(claimed.ipl));
			m_iprGlobalMaster->h->pc = m_iprGlobalMaster->o->ent_int | 0x1;
		}
	}

	AXP_HOT AXP_ALWAYS_INLINE void clearSisrIfSoftware(const ClaimedInterrupt& claimed) noexcept
	{
		if (IrqSource::isSoftwareSource(claimed.source)) {
			m_iprGlobalMaster->h->sisr &=
				~static_cast<quint16>(1u << claimed.ipl);
		}
	}

private:

	/*
	
		Purpose:
		  Provide a single, canonical implementation of the Alpha ASTEN/ASTSR
		  masked read-modify-write (MTPR) semantics.

		Why:
		  EmulatR has two entry paths into PAL/IPR mutation:
			(1) CALL_PAL grains (pipeline path)
			(2) Fault handling entry (runloop -> PalBox direct)
		  Both MUST apply identical ASTEN/ASTSR semantics or you will see divergence.

		Alpha System Reference Manual (SRM) / Alpha Architecture:
		  ASTEN and ASTSR are 4-bit masks and are written via an MTPR operation
		  that uses bits in R16:
			- R16[3:0]  -> "keep" mask (when 1, preserve old bit; when 0, clear it)
			- R16[7:4]  -> "set"  mask (when 1, force bit on)
		  NewValue = (OldValue AND KeepMask) OR SetMask
		  Return value: R0 gets the old 4-bit value zero-extended.

		Source reference:
		  Alpha AXP System Reference Manual, Version 6.0 (1994),
		  IPR descriptions for ASTEN and ASTSR (MTPR semantics and R0 return).
		  (Use the ASTEN / ASTSR IPR entries; they spell out the AND/OR mask form.)

		TODO: None
	*/


	struct AXP_IPR_AST
	{
		// Apply masked read-modify-write on 4-bit value.
		// old4: only low nibble is used.
		// r16 : mask is in bits <7:0>:
		//       <3:0> preserve/clear mask (AND)
		//       <7:4> set mask (OR)
		// static AXP_ALWAYS_INLINE quint8 applyMaskedRmw4(quint8 old4, quint64 r16) noexcept
		// {
		// 	const quint8 andMask = static_cast<quint8>(r16 & 0x0F);
		// 	const quint8 orMask = static_cast<quint8>((r16 >> 4) & 0x0F);
		// 	const quint8 oldNib = static_cast<quint8>(old4 & 0x0F);
		// 	return static_cast<quint8>((oldNib & andMask) | orMask);
		// }
		static AXP_HOT AXP_ALWAYS_INLINE
			quint8 keepMaskFromR16(quint64 r16) noexcept
		{
			return static_cast<quint8>(r16) & 0x0F; // R16[3:0]
		}

		static AXP_HOT AXP_ALWAYS_INLINE
			quint8 setMaskFromR16(quint64 r16) noexcept
		{
			return static_cast<quint8>((r16 >> 4) & 0x0F); // R16[7:4]
		}

		static AXP_HOT AXP_ALWAYS_INLINE
			quint8 applyMaskedRmw4(quint8 old4, quint64 r16) noexcept
		{
			// Canonical SRM semantics:
			// new4 = (old4 AND R16[3:0]) OR R16[7:4]
			const quint8 keep = keepMaskFromR16(r16);
			const quint8 setv = setMaskFromR16(r16);
			return static_cast<quint8>((old4 & keep) | setv) & 0x0F;
		}

		// Helper that returns old value and updates a stored 4-bit register.

		static AXP_HOT AXP_ALWAYS_INLINE
			quint8 mtpr_update4(quint64& reg64_in_out, quint64 r16) noexcept
		{
			const quint8 old4 = static_cast<quint8>(reg64_in_out & 0x0F);
			const quint8 new4 = static_cast<quint8>(applyMaskedRmw4(old4, r16) & 0x0F);
			reg64_in_out = (reg64_in_out & ~quint64(0x0F)) | quint64(new4);
			return old4;
		}
	};

	


#pragma region PAL Argument Builders


	
	

// ------------------------------------------------------------------------
// RESET Vector Arguments
// ------------------------------------------------------------------------
static AXP_HOT AXP_ALWAYS_INLINE    void  buildResetArgs(PalArgumentPack& pack, CPUIdType cpuId, const PendingEvent& ev) noexcept
{
	Q_UNUSED(cpuId)
	// RESET has no arguments
	pack.a0 = 0;
	pack.a1 = 0;
	pack.a2 = 0;
	pack.a3 = 0;
	pack.a4 = 0;
	pack.a5 = 0;
}

// ------------------------------------------------------------------------
// DTB_MISS_SINGLE Vector Arguments
// ------------------------------------------------------------------------
AXP_HOT AXP_ALWAYS_INLINE   void buildDTBMissArgs(PalArgumentPack& pack, CPUIdType cpuId, const PendingEvent& ev) noexcept
{
	Q_UNUSED(cpuId)
	pack.a0 = ev.faultVA;        // Faulting virtual address
	pack.a1 = ev.mmAccessType;   // Read=0, Write=1, Execute=2
	pack.a2 = static_cast<quint64>(m_iprGlobalMaster->h->asn);  // Current ASN
	pack.a3 = 0;  // Reserved (PTE flags if needed)
	pack.a4 = 0;  // Reserved
	pack.a5 = 0;  // Reserved
}

// ------------------------------------------------------------------------
// DTB_MISS_DOUBLE Vector Arguments
// ------------------------------------------------------------------------

AXP_HOT AXP_ALWAYS_INLINE    void buildDTBMissDoubleArgs(PalArgumentPack& pack,
	CPUIdType cpuId,
	const PendingEvent& ev) noexcept
{
	pack.a0 = ev.faultVA;            // Original faulting VA
	pack.a1 = ev.dtbFaultVA;         // VA that faulted during PTE fetch
	pack.a2 = static_cast<quint64>(m_iprGlobalMaster->h->asn);			 // Current ASN
	pack.a3 = ev.mmAccessType;       // Access type
	pack.a4 = 0;  // Reserved
	pack.a5 = 0;  // Reserved
}

// ------------------------------------------------------------------------
// ITB_MISS Vector Arguments
// ------------------------------------------------------------------------

 AXP_HOT AXP_ALWAYS_INLINE   void buildITBMissArgs(PalArgumentPack& pack,
	CPUIdType cpuId,
	const PendingEvent& ev) noexcept
{
	Q_UNUSED(cpuId)
	pack.a0 = ev.faultPC;            // Faulting PC
	pack.a1 = static_cast<quint64>(m_iprGlobalMaster->h->asn);			 // Current ASN
	pack.a2 = 0;  // Reserved
	pack.a3 = 0;  // Reserved
	pack.a4 = 0;  // Reserved
	pack.a5 = 0;  // Reserved
}

// ------------------------------------------------------------------------
// DFAULT Vector Arguments
// ------------------------------------------------------------------------

 AXP_HOT AXP_ALWAYS_INLINE   void buildDFaultArgs(PalArgumentPack& pack,
	CPUIdType cpuId,
	const PendingEvent& ev) noexcept
{
	Q_UNUSED(cpuId)
	pack.a0 = ev.faultVA;            // Faulting VA
	pack.a1 = ev.mmFaultReason;      // Fault reason (ACV, FOE, FOW, FOR)
	pack.a2 = static_cast<quint64>(m_iprGlobalMaster->h->asn);			 // Current ASN
	pack.a3 = ev.mmAccessType;       // Access type
	pack.a4 = 0;  // Reserved
	pack.a5 = 0;  // Reserved
}

// ------------------------------------------------------------------------
// IACCVIO (Instruction Access Violation) Arguments
// ------------------------------------------------------------------------

 AXP_HOT AXP_ALWAYS_INLINE   void buildIACVArgs(PalArgumentPack& pack,
	CPUIdType cpuId,
	const PendingEvent& ev) noexcept
{
	Q_UNUSED(cpuId)
	pack.a0 = ev.faultPC;            // Faulting PC
	pack.a1 = ev.mmFaultReason;      // Reason (sign check, ACV)
	pack.a2 = static_cast<quint64>(m_iprGlobalMaster->h->asn);			 // Current ASN
	pack.a3 = 0;  // Reserved
	pack.a4 = 0;  // Reserved
	pack.a5 = 0;  // Reserved
}

// ------------------------------------------------------------------------
// UNALIGN Vector Arguments
// ------------------------------------------------------------------------

static AXP_HOT AXP_ALWAYS_INLINE   void buildUnalignArgs(PalArgumentPack& pack, CPUIdType cpuId, const PendingEvent& ev) noexcept
{
	Q_UNUSED(cpuId)
	pack.a0 = ev.faultVA;        // Unaligned address
	pack.a1 = ev.opcode;         // Faulting instruction opcode
	pack.a2 = ev.destReg;        // Destination register
	pack.a3 = 0;  // Reserved
	pack.a4 = 0;  // Reserved
	pack.a5 = 0;  // Reserved
}

// ------------------------------------------------------------------------
// ARITH Vector Arguments
// ------------------------------------------------------------------------
static AXP_HOT AXP_ALWAYS_INLINE   void buildArithArgs(PalArgumentPack& pack, CPUIdType cpuId, const PendingEvent& ev) noexcept
{
	Q_UNUSED(cpuId)
	pack.a0 = ev.exc_Sum;         // Exception summary register
	pack.a1 = ev.exc_Mask;        // Exception mask
	pack.a2 = 0;  // Reserved
	pack.a3 = 0;  // Reserved
	pack.a4 = 0;  // Reserved
	pack.a5 = 0;  // Reserved
}

// ------------------------------------------------------------------------
// FEN (Floating-point Disabled) Arguments
// ------------------------------------------------------------------------

static AXP_HOT AXP_ALWAYS_INLINE    void buildFENArgs(PalArgumentPack& pack, CPUIdType cpuId, const PendingEvent& ev) noexcept
{
	pack.a0 = ev.opcode;  // Faulting FP instruction
	pack.a1 = 0;  // Reserved
	pack.a2 = 0;  // Reserved
	pack.a3 = 0;  // Reserved
	pack.a4 = 0;  // Reserved
	pack.a5 = 0;  // Reserved
}

// ------------------------------------------------------------------------
// OPCDEC (Illegal Opcode) Arguments
// ------------------------------------------------------------------------

static AXP_HOT AXP_ALWAYS_INLINE   void buildOPCDECArgs(PalArgumentPack& pack, CPUIdType cpuId, const PendingEvent& ev) noexcept
{
	pack.a0 = ev.opcode;  // Illegal instruction
	pack.a1 = 0;  // Reserved
	pack.a2 = 0;  // Reserved
	pack.a3 = 0;  // Reserved
	pack.a4 = 0;  // Reserved
	pack.a5 = 0;  // Reserved
}

// ------------------------------------------------------------------------
// MCHK (Machine Check) Arguments
// ------------------------------------------------------------------------

static AXP_HOT AXP_ALWAYS_INLINE   void buildMCHKArgs(PalArgumentPack& pack, CPUIdType cpuId, const PendingEvent& ev) noexcept
{
	pack.a0 = ev.mchkCode;   // Machine check code
	pack.a1 = ev.mchkAddr;   // Physical address (if applicable)
	pack.a2 = 0;  // Reserved
	pack.a3 = 0;  // Reserved
	pack.a4 = 0;  // Reserved
	pack.a5 = 0;  // Reserved
}

// ------------------------------------------------------------------------
// RESET Vector Arguments
// ------------------------------------------------------------------------


// ========================================================================
// BUILDER DISPATCH - Select Builder by Vector ID
// ========================================================================

 AXP_HOT AXP_ALWAYS_INLINE   void buildPalArgs(CPUIdType cpuId, PalArgumentPack& pack, PalVectorId_EV6 vecId, const PendingEvent& ev) noexcept
{
	switch (vecId) {
	case PalVectorId_EV6::DTB_MISS_SINGLE:
		buildDTBMissArgs(pack, cpuId, ev);
		break;

	case PalVectorId_EV6::DTB_MISS_DOUBLE:
		buildDTBMissDoubleArgs(pack, cpuId, ev);
		break;

	case PalVectorId_EV6::ITB_MISS:
		buildITBMissArgs(pack, cpuId, ev);
		break;

	case PalVectorId_EV6::DTB_MISS_NATIVE:
		buildDFaultArgs(pack, cpuId, ev);
		break;

	case PalVectorId_EV6::ITB_ACV:
		buildIACVArgs(pack, cpuId, ev);
		break;

	case PalVectorId_EV6::UNALIGN:
		buildUnalignArgs(pack, cpuId, ev);
		break;

	case PalVectorId_EV6::ARITH:
		buildArithArgs(pack, cpuId, ev);
		break;

	case PalVectorId_EV6::FEN:
		buildFENArgs(pack, cpuId, ev);
		break;

	case PalVectorId_EV6::OPCDEC:
		buildOPCDECArgs(pack, cpuId, ev);
		break;

	case PalVectorId_EV6::MCHK:
		buildMCHKArgs(pack, cpuId, ev);
		break;

	case PalVectorId_EV6::RESET:
		buildResetArgs(pack, cpuId, ev);
		break;

	default:
		// Unknown vector - clear args
		pack.a0 = pack.a1 = pack.a2 = 0;
		pack.a3 = pack.a4 = pack.a5 = 0;
		break;
	}
}



#pragma endregion PAL Argument Builders


/**
 * @brief PAL_PUTC - Write character to console (0x02).
 *
 * Args:
 *   a0 = character to write
 *
 * Returns:
 *   v0 = 0 on success, -1 on error
 */
inline PalResult pal_PUTC_handler(PalArgumentPack& args, CPUIdType cpuId)
{
	Q_UNUSED(cpuId);

	quint8 ch = static_cast<quint8>(args.a0 & 0xFF);

	// Write to OPA0 (primary console)
	bool success = m_consoleManager->putCharToOPA(0, ch);

	PalResult result;
	result.returnValue = success ? 0 : static_cast<quint64>(-1);
	result.hasReturnValue = true;

	return result;
}

/**
 * @brief PAL_PUTS - Write string to console (0x09).
 *
 * Args:
 *   a0 = virtual address of string
 *   a1 = length in bytes
 *
 * Returns:
 *   v0 = number of characters written
 */
inline PalResult pal_PUTS_handler(PalArgumentPack& args, CPUIdType cpuId)
{
	quint64 addr = args.a0;
	quint64 len = args.a1;

	if (len == 0) {
		PalResult result;
		result.returnValue = 0;
		result.hasReturnValue = true;
		return result;
	}

	// Get OPA0 device
	auto* opa0 = m_consoleManager->getOPA(0);
	if (!opa0) {
		PalResult result;
		result.returnValue = 0;
		result.hasReturnValue = true;
		return result;
	}

	// Read string from virtual memory and write to console
	quint64 written = 0;
	for (quint64 i = 0; i < len; i++) {
		quint8 ch;

		// Read byte from virtual memory (with TLB translation)
		if (m_ev6Translation->readVirtualByteFromVA(addr + i, ch) != MEM_STATUS::Ok) {
			break;  // Memory fault - stop
		}

		// Write to console
		opa0->putChar(ch);
		written++;
	}

	PalResult result;
	result.returnValue = written;
	result.hasReturnValue = true;
	return result;
}

/**
 * @brief PAL_GETC - Read character from console (0x01).
 *
 * Args:
 *   (none)
 *
 * Returns:
 *   v0 = character (0-255), or -1 if no data
 */
inline PalResult pal_GETC_handler(PalArgumentPack& args, CPUIdType cpuId)
{
	Q_UNUSED(args);
	Q_UNUSED(cpuId);

	// Read from OPA0 (primary console)
	int ch = m_consoleManager->getCharFromOPA(0);

	PalResult result;
	result.returnValue = static_cast<quint64>(ch);
	result.hasReturnValue = true;

	return result;
}

// ============================================================================
// MULTI-CONSOLE VARIANTS (Optional)
// ============================================================================

/**
 * @brief Extended PUTS - Write to specific OPA device.
 *
 * Args:
 *   a0 = virtual address of string
 *   a1 = length in bytes
 *   a2 = OPA index (0 = OPA0, 1 = OPA1, etc.)
 *
 * Returns:
 *   v0 = number of characters written
 */
inline PalResult pal_PUTS_EXT_handler(PalArgumentPack& args, CPUIdType cpuId)
{
	quint64 addr = args.a0;
	quint64 len = args.a1;
	int opaIndex = static_cast<int>(args.a2);

	if (len == 0) {
		PalResult result;
		result.returnValue = 0;
		result.hasReturnValue = true;
		return result;
	}

	// Get specified OPA device
	auto* opa = m_consoleManager->getOPA(opaIndex);
	if (!opa) {
		PalResult result;
		result.returnValue = 0;
		result.hasReturnValue = true;
		return result;
	}

	// Write string
	quint64 written = 0;
	for (quint64 i = 0; i < len; i++) {
		quint8 ch;
		if (m_ev6Translation->readVirtualByteFromVA(addr + i, ch) != MEM_STATUS::Ok) {
			break;
		}
		opa->putChar(ch);
		written++;
	}

	PalResult result;
	result.returnValue = written;
	result.hasReturnValue = true;
	return result;
}


	/* Pack Helpers */

	// Minimal helper to ensure consistent 64-bit return packing.
AXP_HOT AXP_ALWAYS_INLINE quint64 packSISR_toMFPR(quint16 sisr) noexcept
{
	// bit0 unused per your HWPCB comment, so clear it.
	const quint16 masked = quint16(sisr & 0xFFFEu);
	return quint64(masked); // zero-extend to 64-bit
}
/**
 * @brief Extended GETC - Read from specific OPA device.
 *
 * Args:
 *   a0 = OPA index
 *
 * Returns:
 *   v0 = character (0-255), or -1 if no data
 */
inline PalResult pal_GETC_EXT_handler(PalArgumentPack& args, CPUIdType cpuId)
{
	int opaIndex = static_cast<int>(args.a0);

	// Read from specified OPA device
	int ch = m_consoleManager->getCharFromOPA(opaIndex);

	PalResult result;
	result.returnValue = static_cast<quint64>(ch);
	result.hasReturnValue = true;

	return result;
}

// ============================================================================
// OPTIMIZED BULK PUTS (Uses readVirtualString helper)
// ============================================================================

/**
 * @brief Optimized PUTS - Uses bulk string read.
 *
 * More efficient for long strings - reads in chunks.
 *
 * Args:
 *   a0 = virtual address of string
 *   a1 = length in bytes
 *
 * Returns:
 *   v0 = number of characters written
 */
AXP_HOT AXP_ALWAYS_INLINE PalResult pal_PUTS_BULK_handler(PalArgumentPack& args, CPUIdType cpuId)
{
	quint64 addr = args.a0;
	quint64 len = args.a1;

	if (len == 0) {
		PalResult result;
		result.returnValue = 0;
		result.hasReturnValue = true;
		return result;
	}

	// Get OPA0 device
	auto* opa0 = m_consoleManager->getOPA(0);
	if (!opa0) {
		PalResult result;
		result.returnValue = 0;
		result.hasReturnValue = true;
		return result;
	}

	// Read and write in chunks (more efficient)
	constexpr quint64 CHUNK_SIZE = 256;
	quint8 buffer[CHUNK_SIZE];
	quint64 totalWritten = 0;
	quint64 remaining = len;

	while (remaining > 0) {
		quint64 chunkSize = qMin(remaining, CHUNK_SIZE);

		// Correct parameter order
		quint64 bytesRead = m_ev6Translation->readVirtualString( addr, buffer, chunkSize);

		if (bytesRead == 0) {
			break;  // Fault
		}

		// Write chunk to console
		opa0->putString(buffer, bytesRead);

		totalWritten += bytesRead;
		addr += bytesRead;
		remaining -= bytesRead;

		if (bytesRead < chunkSize) {
			break;  // Fault or end of string
		}
	}

	PalResult result;
	result.returnValue = totalWritten;
	result.hasReturnValue = true;
	return result;
}

// ============================================================================
// REGISTRATION HELPER
// ============================================================================

/**
 * @brief Register CSERVE console handlers with PAL dispatcher.
 *
 * Call during PAL initialization.
 */
inline void registerCSERVEConsoleHandlers()
{
	auto& palTable = global_PalVectorTable();

	auto reg = [this, &palTable]<typename T0>(quint16     id,
											  quint8      targetIPL,
											  quint8      requiredCM,
											  quint32     flags,
											  const char* name,
											  T0&&      fn)
		{
			const auto vec = static_cast<PalVectorId_EV6>(id);
			palTable.registerVector(vec, targetIPL, requiredCM, flags, name);
			palTable.registerHandler(static_cast<quint8>(id),
				PalHandlerFunc([this, f = std::forward<T0>(fn)]
				(PalArgumentPack& args, CPUIdType cpu) -> PalResult {
						return f(args, cpu);
					}));
		};

	reg(0x01, 0, 0, 0, "CSERVE_GETC",
		[this](PalArgumentPack& args, CPUIdType cpu) -> PalResult {
			return pal_GETC_handler(args, cpu);
		});

	reg(0x02, 0, 0, 0, "CSERVE_PUTC",
		[this](PalArgumentPack& args, CPUIdType cpu) -> PalResult {
			return pal_PUTC_handler(args, cpu);
		});

	reg(0x09, 0, 0, 0, "CSERVE_PUTS",
		[this](PalArgumentPack& args, CPUIdType cpu) -> PalResult {
			return pal_PUTS_handler(args, cpu);
		});

}



};
#endif // PAL_SERVICE_H

