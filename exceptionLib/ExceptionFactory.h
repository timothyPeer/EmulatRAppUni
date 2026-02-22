// ============================================================================
// ExceptionFactory.h - Create machine check event
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

#ifndef EXCEPTIONFACTORY_H
#define EXCEPTIONFACTORY_H

#include <QtGlobal>
#include <QString>
#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/types_core.h"
#include "faultLib/fault_core.h"
#include "coreLib/global_RegisterMaster_hot.h"
#include "faultLib/PendingEvent_Refined.h"

//forwarders 
struct PendingEvent;
struct AlphaPTE;
enum class MachineCheckReason : quint8;
enum class TranslationResult;
enum class MemoryFaultType : quint32;

// TODO move to a class - instantiate IPR storage once. 
AXP_HOT inline PendingEvent makeDeviceNotAvailableEvent(CPUIdType cpuId, quint8 deviceId) noexcept
{
	ASNType asn = globalHWPCB(cpuId).asn; 

	PendingEvent ev;
	ev.exceptionClass = ExceptionClass_EV6::General;  // or similar
	ev.asn = asn;
	ev.extraInfo = deviceId;
	// ... set other fields
	return ev;
}
AXP_HOT inline PendingEvent makeIllegalInstructionEvent(CPUIdType cpuId) noexcept
{
	ASNType asn = globalHWPCB(cpuId).asn;

	PendingEvent ev;
	ev.exceptionClass = ExceptionClass_EV6::OpcDec;  // or similar
	ev.asn = asn;


	// ... set other fields
	return ev;
}

  PendingEvent makeAccessViolationFault(CPUIdType cpuId, quint64 va, bool isWrite) noexcept;
  PendingEvent makeAccessViolationFault(quint64 va, bool isWrite) noexcept;
  PendingEvent makeArithmeticEvent(CPUIdType cpuId, quint64 faultPC, quint64 excSumBits) noexcept;
  PendingEvent makeBreakpointEvent(CPUIdType cpuId, quint64 faultPC) noexcept;
  PendingEvent makeBugcheckEvent(CPUIdType cpuId, quint64 faultPC) noexcept;
  PendingEvent makeCallPalEvent(CPUIdType cpuId, quint64 faultPC, quint8 palFunction) noexcept;
  PendingEvent makeDTBAccessViolationEvent(CPUIdType cpuId, quint64 faultVA, bool isWrite) noexcept;
  PendingEvent makeDTBDoubleMissEvent(CPUIdType cpuId, quint64 faultVA, bool isWrite) noexcept;
  PendingEvent makeDTBFaultEvent(CPUIdType cpuId, quint64 faultVA, bool isWrite, MemoryFaultType faultType) noexcept;
  PendingEvent makeDTBMissDouble3Event(CPUIdType cpuId, quint64 faultVA, bool isWrite) noexcept;

  PendingEvent makeDTBMissFault(CPUIdType cpuId, VAType va, ASNType asn, quint64 pc, bool isWrite = false)  noexcept;

  PendingEvent makeDTBMissSingleEvent(CPUIdType cpuId, quint64 faultVA, ASNType asn, quint64 faultPC, bool isWrite) noexcept;
  PendingEvent makeFENEvent(CPUIdType cpuId, quint64 faultPC) noexcept;
  PendingEvent makeFaultOnExecuteEvent(CPUIdType cpuId, quint64 faultVA) noexcept;
  PendingEvent makeFaultOnReadEvent(CPUIdType cpuId, quint64 faultVA) noexcept;
  PendingEvent makeFaultOnWriteEvent(CPUIdType cpuId, quint64 faultVA) noexcept;
  PendingEvent makeITBAccessViolationEvent(CPUIdType cpuId, quint64 faultVA) noexcept;
  PendingEvent makeITBMissEvent(CPUIdType cpuId, quint64 faultVA) noexcept;
  PendingEvent makeIllegalInstruction(TrapCode_Class trapCode, quint64 faultPC) noexcept;
  PendingEvent makeIllegalOpcodeEvent(CPUIdType cpuId, quint64 faultPC, quint32 instruction) noexcept;
 AXP_ALWAYS_INLINE PendingEvent makeInternalErrorEvent(
	  CPUIdType cpuId,
	  const QString& reason) noexcept
  {
	  PendingEvent ev;

	  // Core identification
	  ev.cpuId = cpuId;
	  ev.eventClass = EventClass::InternalError;
	  ev.priority = EventPriority::High;
	  ev.kind = PendingEventKind::Exception;

	  // Exception classification
	  ev.exceptionClass = ExceptionClass_EV6::InternalProcessorError;

	  // Address/context (not applicable for internal errors)
	  ev.faultVA = 0;
	  ev.faultPC = 0;

	  // Description
	  ev.description = QString("CPU %1 internal error: %2")
		  .arg(cpuId)
		  .arg(reason);

	  return ev;
  }

  PendingEvent makeInvalidPTE(CPUIdType cpuId, quint64 va, const AlphaPTE& pte) noexcept;
  PendingEvent makeMTFPCREvent(CPUIdType cpuId, quint64 faultPC) noexcept;
  PendingEvent makeMachineCheckEvent(CPUIdType cpuId, MachineCheckReason reason, quint64 errorAddr) noexcept;
  PendingEvent makeMemoryAccessFault(CPUIdType cpuId, quint64 faultVA, bool isWrite) noexcept;
  PendingEvent makeMemoryAccessFault(CPUIdType cpuId, quint64 faultVA, quint64 pa, bool isWrite) noexcept;
  PendingEvent makeMemoryFault(CPUIdType cpuId, quint64 va) noexcept;
  PendingEvent makeMemoryFault(quint64 va) noexcept;
  PendingEvent makeMemoryStreamFault(CPUIdType cpuId, quint64 faultVA, quint64 pa, bool isWrite) noexcept;
  PendingEvent makeNonCanonicalAddressEvent(CPUIdType cpuId, quint64 faultVA, bool isWrite) noexcept;
  PendingEvent makeResetEvent(CPUIdType cpuId) noexcept;
  PendingEvent makeSoftwareTrapEvent(CPUIdType cpuId, quint64 faultPC, quint64 trapCode) noexcept;
  PendingEvent makeTranslationFault(CPUIdType cpuId, quint64 va, TranslationResult tr, bool isWrite) noexcept;
  PendingEvent makeUnalignedEvent(CPUIdType cpuId, quint64 faultVA, bool isWrite) noexcept;
  PendingEvent makeSmpBarrierTimeoutEvent(CPUIdType cpuId, CPUIdType initiatingCpu, quint32 participatingCpus, quint32 acknowledgedCpus) noexcept;

  void updateMemoryTrap_IPR(CPUIdType cpuId, quint64 faultVA, bool isWrite, MemoryFaultType faultType) noexcept;

  /**
   * @brief Create machine check event
   * @param cpuId CPU that encountered the error
   * @param reason Error description
   * @return PendingEvent for machine check
   */
  PendingEvent makeMachineCheckEvent(
      CPUIdType cpuId,
      const QString& reason) noexcept;

  /**
   * @brief Create machine check event with addresses
   * @param cpuId CPU that encountered the error
   * @param reason Machine check reason code
   * @param faultVA Virtual address (if applicable)
   * @param faultPA Physical address (if applicable)
   * @return PendingEvent for machine check
   */
  PendingEvent makeMachineCheckEventDetailed(
      CPUIdType cpuId,
      MachineCheckReason reason,
      quint64 faultVA,
      quint64 faultPA) noexcept;

  static QString getMachineCheckReasonString(MachineCheckReason reason) noexcept;
#endif // EXCEPTIONFACTORY_H
