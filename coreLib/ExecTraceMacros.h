// ============================================================================
// ExecTraceMacros.h - Static Global ExecTrace Interface
// ============================================================================

#pragma once

#ifdef EXECTRACE_ENABLED

// ============================================================================
// Instruction Commit Macros
// ============================================================================

// Basic commit (PC + instruction word only)
#define EXECTRACE_COMMIT(cpuId, pc, instrWord) \
ExecTrace::recordCommit(cpuId, pc, instrWord)

// Commit with register writes
#define EXECTRACE_COMMIT_WRITES(cpuId, pc, instrWord, writes, writeCount) \
    ExecTrace::recordCommitWithWrites(cpuId, pc, instrWord, writes, writeCount)

// Commit with full context (flags, memory, etc.)
#define EXECTRACE_COMMIT_FULL(cpuId, record) \
    ExecTrace::recordCommitFull(cpuId, record)

// Commit with grain information
#define EXECTRACE_COMMIT_GRAIN(cpuId, pc, instrWord, opcode, func, mnem, type, found) \
    ExecTrace::recordCommitWithGrain(cpuId, pc, instrWord, opcode, func, mnem, type, found)

// ============================================================================
// SMP Event Macros
// ============================================================================

// IPI send
#define EXECTRACE_IPI_SEND(srcCpu, dstMask, reason, ipiSeq) \
    ExecTrace::recordIpiSend(srcCpu, dstMask, reason, ipiSeq)

// IPI receive
#define EXECTRACE_IPI_RECV(dstCpu, srcCpu, reason, ipiSeq) \
    ExecTrace::recordIpiRecv(dstCpu, srcCpu, reason, ipiSeq)

// TLB invalidate
#define EXECTRACE_TLB_INV(cpuId, op, va, asn) \
    ExecTrace::recordTlbInvalidate(cpuId, op, va, asn)

// ============================================================================
// Trigger Macros
// ============================================================================

// Trigger dump on this CPU
#define EXECTRACE_TRIGGER(cpuId, reason) \
    ExecTrace::trigger(cpuId, reason)

// Trigger on exception
#define EXECTRACE_TRIGGER_EXCEPTION(cpuId) \
    ExecTrace::trigger(cpuId, TRIGGER_EXCEPTION)

// Trigger on PAL entry
#define EXECTRACE_TRIGGER_PAL_ENTRY(cpuId) \
    ExecTrace::trigger(cpuId, TRIGGER_PAL_ENTRY)

// Trigger on IPI
#define EXECTRACE_TRIGGER_IPI(cpuId) \
    ExecTrace::trigger(cpuId, TRIGGER_IPI)

// ============================================================================
// Marker Macros
// ============================================================================

// Generic marker
#define EXECTRACE_MARKER(cpuId, markerId, arg0, arg1, arg2) \
    ExecTrace::recordMarker(cpuId, markerId, arg0, arg1, arg2)


#else
// ============================================================================
// Disabled - all macros compile to nothing
// ============================================================================

#define EXECTRACE_COMMIT(cpuId, pc, instrWord) do {} while(0)
#define EXECTRACE_COMMIT_WRITES(cpuId, pc, instrWord, writes, writeCount) do {} while(0)
#define EXECTRACE_COMMIT_FULL(cpuId, record) do {} while(0)
#define EXECTRACE_IPI_SEND(srcCpu, dstMask, reason, ipiSeq) do {} while(0)
#define EXECTRACE_IPI_RECV(dstCpu, srcCpu, reason, ipiSeq) do {} while(0)
#define EXECTRACE_TLB_INV(cpuId, op, va, asn) do {} while(0)
#define EXECTRACE_TRIGGER(cpuId, reason) do {} while(0)
#define EXECTRACE_TRIGGER_EXCEPTION(cpuId) do {} while(0)
#define EXECTRACE_TRIGGER_PAL_ENTRY(cpuId) do {} while(0)
#define EXECTRACE_TRIGGER_IPI(cpuId) do {} while(0)
#define EXECTRACE_MARKER(cpuId, markerId, arg0, arg1, arg2) do {} while(0)
#define EXECTRACE_COMMIT_GRAIN(cpuId, pc, instrWord, opcode, func, mnem, type, found) do {} while(0)


#endif // EXECTRACE_ENABLED
