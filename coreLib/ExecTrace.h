#ifndef EXECTRACE_H
#define EXECTRACE_H
// ============================================================================
// ExecTrace.h - Static Global ExecTrace API
// ============================================================================
// Extended with pipeline lifecycle, PAL entry/exit, fault chain, and IPR
// instrumentation for debugging Alpha AXP emulation boundary transitions.
// ============================================================================

#include <QFile>
#include <QTextStream>
#include <QtGlobal>

#include "types_core.h"
#include "machineLib/PipeLineSlot.h"

// Forward declarations
struct WriteEntry;
struct PalResult;
enum class TrapCode_Class : quint8;
enum class PalCallPalFunction ;
//enum class HW_IPR : quint16;

// ============================================================================
// Formatting Helpers
// ============================================================================

static inline QString hx64(quint64 v) {
    return QString("0x%1").arg(v, 16, 16, QChar('0'));
}
static inline QString hx32(quint32 v) {
    return QString("0x%1").arg(v, 8, 16, QChar('0'));
}
static inline QString hx16(quint16 v) {
    return QString("0x%1").arg(v, 4, 16, QChar('0'));
}
static inline QString hx8(quint8 v) {
    return QString("0x%1").arg(v, 2, 16, QChar('0'));
}

// ============================================================================
// Enumerations
// ============================================================================

// Trigger reasons
enum TriggerReason : quint16 {
    TRIGGER_EXCEPTION = 1,
    TRIGGER_PAL_ENTRY = 2,
    TRIGGER_PAL_EXIT = 3,
    TRIGGER_IPI = 4,
    TRIGGER_PC_RANGE = 5
};

// TLB operations
enum TlbOp : quint16 {
    TLB_OP_TBIA = 1,
    TLB_OP_TBIS = 2,
    TLB_OP_TBISD = 3,
    TLB_OP_TBISI = 4
};

// Pipeline discard reasons
enum class DiscardReason : quint8 {
    FAULT = 1,    // Faulting instruction in WB
    PAL_CALL = 2,    // CALL_PAL detected in WB
    FLUSH = 3,    // Pipeline flush (external)
    SQUASH = 4     // Branch misprediction squash
};

// Pipeline stage identifiers (for fault source tracking)
enum class PipelineStage_enum : quint8 {
    IF = 0,
    DE = 1,
    IS = 2,
    EX = 3,
    MEM = 4,
    WB = 5
};

// PAL entry reasons (matches your PalEntryReason)
enum class PalEntryReasonTrace : quint8 {
    CALL_PAL = 1,
    FAULT = 2,
    INTERRUPT = 3,
    TRAP = 4,
    MACHINE_CHECK = 5
};

// Write entry (register/IPR write)
struct WriteEntry {
    uint8_t type;           // 0=IntReg, 1=FpReg, 2=IPR
    uint8_t index;          // Register/IPR number
    uint16_t reserved;
    uint64_t value;
};

// ============================================================================
// String Converters (for text trace output)
// ============================================================================

static inline const char* discardReasonName(DiscardReason r) noexcept {
    switch (r) {
    case DiscardReason::FAULT:      return "FAULT";
    case DiscardReason::PAL_CALL:   return "PAL_CALL";
    case DiscardReason::FLUSH:      return "FLUSH";
    case DiscardReason::SQUASH:     return "SQUASH";
    default:                        return "UNKNOWN";
    }
}

static inline const char* pipelineStageName(PipelineStage_enum s) noexcept {
    switch (s) {
    case PipelineStage_enum::IF:  return "IF";
    case PipelineStage_enum::DE:  return "DE";
    case PipelineStage_enum::IS:  return "IS";
    case PipelineStage_enum::EX:  return "EX";
    case PipelineStage_enum::MEM: return "MEM";
    case PipelineStage_enum::WB:  return "WB";
    default:                 return "??";
    }
}

static inline const char* palEntryReasonName(PalEntryReasonTrace r) noexcept {
    switch (r) {
    case PalEntryReasonTrace::CALL_PAL:      return "CALL_PAL";
    case PalEntryReasonTrace::FAULT:         return "FAULT";
    case PalEntryReasonTrace::INTERRUPT:     return "INTERRUPT";
    case PalEntryReasonTrace::TRAP:          return "TRAP";
    case PalEntryReasonTrace::MACHINE_CHECK: return "MCHK";
    default:                                 return "UNKNOWN";
    }
}

enum RecordType : uint8_t {
    RECORD_COMMIT = 1,
    RECORD_IPI_SEND = 2,
    RECORD_IPI_RECV = 3,
    RECORD_TLB_INV = 4,
    RECORD_MARKER = 5,
    RECORD_TRIGGER = 6,
    RECORD_INTERRUPT = 7,    // <-- NEW
};


static const char* trapCodeClassName(quint8 tc) noexcept
{
    switch (static_cast<TrapCode_Class>(tc))
    {
    case TrapCode_Class::NONE:                  return "NONE";
    case TrapCode_Class::ARITHMETIC_TRAP:       return "ARITH";
    case TrapCode_Class::DTB_MISS:              return "DTB_MISS";
    case TrapCode_Class::DTB_FAULT:             return "DTB_FAULT";
    case TrapCode_Class::DTB_ACCESS_VIOLATION:  return "DTB_ACV";
    case TrapCode_Class::FP_DISABLED:           return "FP_DIS";
    case TrapCode_Class::FP_OVERFLOW:           return "FP_OVF";
    case TrapCode_Class::ILLEGAL_INSTRUCTION:   return "ILLOP";
    case TrapCode_Class::INTEGER_OVERFLOW:       return "INT_OVF";
    case TrapCode_Class::ITB_ACCESS_VIOLATION:  return "ITB_ACV";
    case TrapCode_Class::ITB_FAULT:             return "ITB_FAULT";
    case TrapCode_Class::ITB_MISS:              return "ITB_MISS";
    case TrapCode_Class::ITB_MISALIGN_FAULT:    return "ITB_MISALIGN";
    case TrapCode_Class::MACHINE_CHECK:         return "MCHK";
    case TrapCode_Class::OPCODE_RESERVED:       return "RESOP";
    case TrapCode_Class::PRIVILEGE_VIOLATION:    return "PRIVVIO";
    case TrapCode_Class::ALIGNMENT_FAULT:       return "ALIGN";
    case TrapCode_Class::UN_ALIGNED:            return "UNALIGN";
    case TrapCode_Class::FEN_FAULT:             return "FEN";
    case TrapCode_Class::TRANSLATION_FAULT:     return "XLATE";
    default:                                     return "???";
    }
}

// ============================================================================
// Register name helper (R0..R31)
// ============================================================================
// static inline QString regName(quint8 reg) noexcept
// {
//     if (reg == 31) return QStringLiteral("R31/zero");
//     if (reg == 30) return QStringLiteral("R30/sp");
//     if (reg == 29) return QStringLiteral("R29/gp");
//     if (reg == 26) return QStringLiteral("R26/ra");
//     return QString("R%1").arg(reg);
// }


// ============================================================================
// ExecTrace - Static Global Interface
// ============================================================================

class ExecTrace
{
public:
    //  Impl MUST be public for ExecTraceWriter to access it ***
    class Impl;

    // ========================================================================
    // Configuration
    // ========================================================================
    static void setFormat(const QString& format) noexcept;
    static QString getFormat() noexcept;

    // ========================================================================
    // Initialization (called once at startup)
    // ========================================================================

    static bool initialize(const QString& format = "csv") noexcept;
    static void shutdown() noexcept;

    // ========================================================================
    // Instruction Commit Recording (existing)
    // ========================================================================

    // Basic commit (PC + instruction word)
    static void recordCommit(quint16 cpuId, quint64 pc, quint32 instrWord) noexcept;

    // Commit with register writes
    static void recordCommitWithWrites(quint16 cpuId, quint64 pc, quint32 instrWord,
        const WriteEntry* writes, uint8_t writeCount) noexcept;

    static void recordCommitWithGrain(uint16_t cpuId, uint64_t pc, uint32_t instrWord,
        uint8_t opcode, uint16_t functionCode,
        const char* mnemonic, const char* grainTypeName,
        uint8_t grainType, bool grainFound,
        const PipelineSlot* slot) noexcept;

    static void recordCommitAsAssembly(uint16_t cpuId, uint64_t pc, uint32_t instrWord,
        const char* mnemonic,
        const PipelineSlot& slot) noexcept;

    // ========================================================================
    // TIER 1: Pipeline Lifecycle
    // ========================================================================

    // stage_WB retired an instruction normally
    static void recordWBRetire(quint16 cpuId, quint64 pc, quint32 instrWord,
        const char* mnemonic) noexcept;

    // stage_WB committed a deferred register write from m_pending
    static void recordCommitPending(quint16 cpuId, quint8 reg, quint64 value,
        quint64 fromPC) noexcept;

    // stage_WB discarded m_pending (fault, PAL_CALL, or flush)
    static void recordDiscardPending(quint16 cpuId, DiscardReason reason,
        quint64 discardedPC) noexcept;


    // ========================================================================
    // TIER 5: Interrupt Events
    // ========================================================================

    /// Interrupt taken - asynchronous control transfer to PAL
    static void recordInterrupt(quint16 cpuId, quint64 fromPC,
        quint64 vector, quint8 interruptType,
        quint8 ipl) noexcept;

    // Pipeline flushed - who requested, at what PC
    static void recordPipelineFlush(quint16 cpuId, const char* source,
        quint64 pc) noexcept;

    // ========================================================================
    // TIER 2: PAL Entry / Exit
    // ========================================================================

    // Entering PAL mode (from any source)
    static void recordPalEnter(quint16 cpuId, PalEntryReasonTrace reason,
        quint64 vector, quint64 faultPC,
        quint64 oldPC, quint8 oldIPL, quint8 oldCM) noexcept;

    // Exiting PAL mode (HW_REI / REI)
    static void recordPalExit(quint16 cpuId, quint64 returnPC,
        quint8 newIPL, quint8 newCM) noexcept;

    // CALL_PAL function dispatched to PalService::execute()
    static void recordPalDispatch(quint16 cpuId, quint16 palFunction,
        quint64 pc, const char* name) noexcept;

    // commitPalResult applied to architectural state
    static void recordPalCommit(quint16 cpuId, quint8 destReg, quint64 value,
        bool pcModified, quint64 newPC,
        bool flushRequested) noexcept;
   
    

    // ========================================================================
    // TIER 3: Fault Chain
    // ========================================================================

    // Fault raised (detected in any pipeline stage)
    static void recordFaultRaised(quint16 cpuId, quint8 trapCode, quint64 faultVA, quint64 faultPC,
        PipelineStage_enum stage) noexcept;

    // Fault dispatched from stage_WB to run loop
    static void recordFaultDispatched(quint16 cpuId, quint8 trapCode,
        quint64 faultVA, quint64 faultPC) noexcept;

    // ========================================================================
    // TIER 4: IPR Read / Write (HW_MFPR / HW_MTPR)
    // ========================================================================

    // HW_MFPR - read IPR value
    static void recordIPRRead(quint16 cpuId, quint16 iprIndex,
        quint64 value) noexcept;

    // HW_MTPR - write IPR value (with old value for RMW tracking)
    static void recordIPRWrite(quint16 cpuId, quint16 iprIndex,
        quint64 newValue, quint64 oldValue) noexcept;

    // ========================================================================
    // SMP Events (existing)
    // ========================================================================

    static void recordIpiSend(quint16 srcCpu, quint32 dstMask, quint16 reason,
        quint64 ipiSeq) noexcept;

    static void recordIpiRecv(quint16 dstCpu, quint16 srcCpu, quint16 reason,
        quint64 ipiSeq) noexcept;

    static void recordTlbInvalidate(quint16 cpuId, quint16 op, quint64 va,
        quint32 asn) noexcept;

    // ========================================================================
    // Triggers (existing)
    // ========================================================================

    static void trigger(quint16 cpuId, TriggerReason reason) noexcept;

    // ========================================================================
    // Markers (existing)
    // ========================================================================

    static void recordMarker(quint16 cpuId, quint32 markerId,
        quint64 arg0, quint64 arg1, quint64 arg2) noexcept;

    // ========================================================================
    // Configuration Query
    // ========================================================================

    static bool isEnabled() noexcept;
    static bool isEnabledForCpu(quint16 cpuId) noexcept;

private:

    static void formatDECAssembly(
        QTextStream* stream,
        uint64_t pc,
        uint32_t raw,
        const char* mnemonic,
        const PipelineSlot& slot) noexcept;
};
#endif
