// ============================================================================
// PAL_core.h - PAL Core Types, Constants, and Pipeline Effect Definitions
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
//
// Core definitions for the PAL (Privileged Architecture Library) subsystem:
//
//   PipelineEffect   -- Bitmask flags for side effects communicated from
//                       PAL service routines back to the CPU run loop.
//   PalResult        -- Structured outcome of a PAL function execution,
//                       carrying return values, PC changes, and pipeline
//                       side effects via PipelineEffect flags.
//   PalStatus        -- Overall success/failure of a PAL operation.
//   PalCallPalFunction -- Enumeration of all CALL_PAL function codes.
//   PAL_FLAG_*       -- Compile-time PAL operation behavior flags.
//
// Side-effect architecture:
//   PAL service routines set bits in PalResult::sideEffects via fluent
//   setters.  The CPU run loop reads the bitmask after the PAL call
//   returns and dispatches pipeline actions accordingly (drain write
//   buffers, flush TLB caches, resync IRQ, etc.).  This keeps pipeline
//   commit logic out of the register master and PAL service layer.
//
// ============================================================================

#ifndef PAL_CORE_H
#define PAL_CORE_H

#include <QtGlobal>
#include "../pteLib/alpha_pte_core.h"
#include "faultLib/fault_core.h"
#include "grainFactoryLib/InstructionGrain_core.h"

// Forward declarations
enum class PalStatus;
class GuestMemory;


// ============================================================================
// CALL_PAL Base Address
// ============================================================================
static constexpr quint64 CALL_PAL_BASE = 0x0;


// ============================================================================
// PAL Operation Behavior Flags (compile-time instruction metadata)
// ============================================================================

static constexpr quint32 PAL_FLAG_NONE = 0x00000000;

// Operation behavior type
static constexpr quint32 PAL_FLAG_SYNCHRONOUS = 0x00000001;
static constexpr quint32 PAL_FLAG_ASYNC_OP = 0x00000002;

// CALL_PAL semantic
static constexpr quint32 PAL_FLAG_CALLPAL = 0x00000004;

// Trap/exception behavior
static constexpr quint32 PAL_FLAG_RAISES_TRAP = 0x00000008;

// Privilege behavior
static constexpr quint32 PAL_FLAG_PRIV_REQUIRED = 0x00000010;

// State modification behavior
static constexpr quint32 PAL_FLAG_MODECHANGE = 0x00000020;
static constexpr quint32 PAL_FLAG_RETURNS_VALUE = 0x00000040;

// OS-specific PAL restrictions
static constexpr quint32 PAL_FLAG_VMS_ONLY = 0x00000100;
static constexpr quint32 PAL_FLAG_TRU64_ONLY = 0x00000200;


// ============================================================================
// Fence Kinds (MB, WMB, RMB)
// ============================================================================
enum class palCore_FenceKind : quint8 {
    MB = 0,    // Memory barrier (read + write)
    WMB = 1,    // Write barrier
    RMB = 2,    // Read barrier
};


// ============================================================================
// Probe Result (PROBER / PROBEW)
// ============================================================================
enum class ProbeResult : quint8 {
    Ok,
    NO_MAPPING,
    NO_PERMISSION
};


// ============================================================================
// Memory Region Descriptors (PAL ROM / RAM mapping)
// ============================================================================
enum class MemoryRegionKind {
    RAM,
    ROM,
    MMIO,
    PAL_ROM,    // Console/firmware PAL image
    PAL_RAM,    // OS-copied PAL
};

struct PALRegion {
    MemoryRegionKind kind;
    quint64 physBase;
    quint64 size;
    bool    writable;
};

struct PALMemoryMap {
    PALRegion consolePal;   // Initial ROM PAL
    PALRegion osPal;        // Current active PAL in RAM (optional)
};


// ============================================================================
// PalReturnReg -- Architectural destination for PAL return values
// ============================================================================
enum class PalReturnReg : quint8
{
    R0 = 0,
    R1 = 1,
    R2 = 2,
    R3 = 3,
    NONE = 126,
};


// ============================================================================
// PipelineEffect -- Side-effect bitmask (PAL -> run loop)
// ============================================================================
//
// Set by PAL service routines in PalResult::sideEffects.
// Read by the CPU run loop after PAL call retirement.
//
// The run loop dispatches actions based on which bits are set:
//   kTLBModified         -> invalidate cached translations
//   kIPLChanged          -> resync IRQ controller
//   kContextSwitched     -> full pipeline state update
//   kMemoryBarrier       -> drain load/store queues
//   kPCBBChanged         -> update PCBB tracking
//   kHalt                -> halt CPU (haltCode carries reason)
//   kNotifyHalt          -> notify system controller of halt
//   kDrainWriteBuffers   -> drain pending store buffer
//   kFlushPendingTraps   -> flush deferred trap queue
//   kRequestPipelineFlush -> squash speculative pipeline state
//   kClearBranchPredictor -> reset branch predictor tables
//   kFlushPendingIPRWrites -> commit deferred IPR side effects
//
// ============================================================================
namespace PipelineEffect {
    constexpr quint32 kNone = 0;
    constexpr quint32 kTLBModified = (1u << 0);
    constexpr quint32 kIPLChanged = (1u << 1);
    constexpr quint32 kContextSwitched = (1u << 2);
    constexpr quint32 kMemoryBarrier = (1u << 3);
    constexpr quint32 kPCBBChanged = (1u << 4);
    constexpr quint32 kHalt = (1u << 5);
    constexpr quint32 kNotifyHalt = (1u << 6);
    constexpr quint32 kDrainWriteBuffers = (1u << 7);
    constexpr quint32 kFlushPendingTraps = (1u << 8);
    constexpr quint32 kRequestPipelineFlush = (1u << 9);
    constexpr quint32 kClearBranchPredictor = (1u << 10);
    constexpr quint32 kFlushPendingIPRWrites = (1u << 11);
}


// ============================================================================
// PalCallPalFunction -- All CALL_PAL function codes
// ============================================================================
enum class PalCallPalFunction {
    // Privileged (0x00-0x3F) -- requires CM=Kernel
    HALT = 0x0000,
    CFLUSH = 0x0001,
    DRAINA = 0x0002,
    LDQP = 0x0003,
    STQP = 0x0004,
    SWPCTX = 0x0005,
    MFPR_ASN = 0x0006,
    MTPR_ASTEN = 0x0007,
    MTPR_ASTSR = 0x0008,
    CSERVE = 0x0009,
    SWPPAL = 0x000A,
    MFPR_FEN = 0x000B,
    MTPR_FEN = 0x000C,
    MTPR_IPIR = 0x000D,
    MFPR_IPL = 0x000E,
    MTPR_IPL = 0x000F,
    MFPR_MCES = 0x0010,
    MTPR_MCES = 0x0011,
    MFPR_PCBB = 0x0012,
    MFPR_PRBR = 0x0013,
    MTPR_PRBR = 0x0014,
    MFPR_PTBR = 0x0015,
    MFPR_SCBB = 0x0016,
    MTPR_SCBB = 0x0017,
    MFPR_SIRR = 0x0018,
    MFPR_SISR = 0x0019,
    MFPR_TBCHK = 0x001A,
    MTPR_TBIA = 0x001B,
    MTPR_TBIAP = 0x001C,
    MTPR_TBIS = 0x001D,
    MFPR_ESP = 0x001E,
    MTPR_ESP = 0x001F,
    MFPR_SSP = 0x0020,
    MTPR_SSP = 0x0021,
    MFPR_USP = 0x0022,
    MTPR_USP = 0x0023,
    MTPR_TBISD = 0x0024,
    MTPR_TBISI = 0x0025,
    MFPR_ASTEN = 0x0026,
    MFPR_ASTSR = 0x0027,
    MFPR_VPTB = 0x0029,
    MTPR_VPTB = 0x002A,
    MTPR_PERFMON = 0x002B,
    WRVPTPTR_OSF = 0x002D,
    MTPR_DATFX = 0x002E,
    SWPCTX_OSF = 0x0030,
    WRVAL_OSF = 0x0031,
    RDVAL_OSF = 0x0032,
    TBI_OSF = 0x0033,
    WRENT_OSF = 0x0034,
    SWPIPL_OSF = 0x0035,
    RDPS_OSF = 0x0036,
    WRKGP_OSF = 0x0037,
    WRUSP_OSF = 0x0038,
    WRPERFMON_OSF = 0x0039,
    RDUSP_OSF = 0x003A,
    WHAMI_OSF = 0x003C,
    RETSYS_OSF = 0x003D,
    WTINT = 0x003E,
    MFPR_WHAMI = 0x003F,

    // Unprivileged (0x80-0xBF)
    BPT = 0x0080,
    BUGCHECK = 0x0081,
    CHME = 0x0082,
    CHMK = 0x0083,
    CHMS = 0x0084,
    CHMU = 0x0085,
    IMB = 0x0086,
    INSQHIL = 0x0087,
    INSQTIL = 0x0088,
    INSQHIQ = 0x0089,
    INSQTIQ = 0x008A,
    INSQUEL = 0x008B,
    INSQUEQ = 0x008C,
    INSQUEL_D = 0x008D,
    INSQUEQ_D = 0x008E,
    PROBER = 0x008F,
    PROBEW = 0x0090,
    RD_PS = 0x0091,
    REI = 0x0092,
    REMQHIL = 0x0093,
    REMQTIL = 0x0094,
    REMQHIQ = 0x0095,
    REMQTIQ = 0x0096,
    REMQUEL = 0x0097,
    REMQUEQ = 0x0098,
    REMQUEL_D = 0x0099,
    REMQUEQ_D = 0x009A,
    SWASTEN = 0x009B,
    WR_PS_SW = 0x009C,
    RSCC = 0x009D,
    READ_UNQ = 0x009E,
    WRITE_UNQ = 0x009F,
    AMOVRR = 0x00A0,
    AMOVRM = 0x00A1,
    INSQHILR = 0x00A2,
    INSQTILR = 0x00A3,
    INSQHIQR = 0x00A4,
    INSQTIQR = 0x00A5,
    REMQHILR = 0x00A6,
    REMQTILR = 0x00A7,
    REMHIQR = 0x00A8,
    REMQTIQR = 0x00A9,
    GENTRAP = 0x00AA,
    KBPT = 0x00AC,
    CLRFEN = 0x00AE,

    MAX_PAL_FUNCTION
};


// ============================================================================
// PalResult -- Structured outcome of a PAL function execution
// ============================================================================
//
// Produced by PAL service routines, consumed by the CPU run loop.
//
// Architecture:
//   - Return value:     hasReturnValue + returnReg + returnValue
//   - Control flow:     doesReturn, pcModified, newPC, entryPC, faultPC/VA
//   - Processor state:  psModified/newPS, iplModified/newIPL, asnModified/newASN
//   - Pipeline effects: sideEffects bitmask (PipelineEffect flags)
//   - Exception:        raisesException + exceptionVector
//
// Side effects are communicated exclusively through the sideEffects bitmask.
// The run loop checks hasAnySideEffects() and dispatches accordingly.
// Fluent setters return *this for chaining:
//
//   slot.palResult
//       .tlbModified()
//       .drainWriteBuffers()
//       .requestPipelineFlush();
//
// ============================================================================
struct alignas(16) PalResult
{
    // -----------------------------------------------------------------
    // PAL function identification
    // -----------------------------------------------------------------
    quint32 palFunction{ 0 };



    // -----------------------------------------------------------------
    // Return value handling
    // -----------------------------------------------------------------
    bool         hasReturnValue{ false };
    PalReturnReg returnReg{ PalReturnReg::NONE };
    quint64      returnValue{ 0 };

    // -----------------------------------------------------------------
    // Control-flow effects
    // -----------------------------------------------------------------
    bool    doesReturn{ true };         // false for RESET, HALT, MCHK
    bool    pcModified{ false };
    quint64 entryPC{ 0 };               // PC at CALL_PAL entry
    quint64 newPC{ 0 };                 // Target PC if pcModified
    quint64 faultPC{ 0 };               // PC of faulting instruction
    quint64 faultVA{ 0 };               // Faulting VA (DTB miss, ACV, etc.)
    quint64 pipelineFlushPC{ 0 };       // Flush-to PC (if kRequestPipelineFlush)

    TrapCode_Class trapCode{};
    PalStatus      status{};

    // -----------------------------------------------------------------
    // Processor state changes (carry data values, not just flags)
    // -----------------------------------------------------------------
    bool    psModified{ false };
    quint64 newPS{ 0 };

    bool    iplModified{ false };
    quint8  newIPL{ 0 };

    bool    asnModified{ false };
    quint8  newASN{ 0 };

    // -----------------------------------------------------------------
    // Exception / trap result
    // -----------------------------------------------------------------
    bool    raisesException{ false };
    quint16 exceptionVector{ 0 };       // EV6 PAL vector offset

    // -----------------------------------------------------------------
    // Halt code (data field, accompanies kHalt flag)
    // -----------------------------------------------------------------
    quint32 haltCode{ 0 };

    // -----------------------------------------------------------------
    // Pipeline side-effect bitmask (PipelineEffect flags)
    // -----------------------------------------------------------------
    // ALL pipeline side effects are expressed through this bitmask.
    // The run loop reads it after PAL return and dispatches actions.
    // -----------------------------------------------------------------
    quint32 sideEffects{ PipelineEffect::kNone };

    // -----------------------------------------------------------------
    // Fluent side-effect setters (return *this for chaining)
    // -----------------------------------------------------------------

    AXP_HOT AXP_ALWAYS_INLINE PalResult& halt(quint32 code) noexcept {
        haltCode = code;
        sideEffects |= PipelineEffect::kHalt | PipelineEffect::kNotifyHalt;
        return *this;
    }

    AXP_HOT AXP_ALWAYS_INLINE PalResult& tlbModified() noexcept {
        sideEffects |= PipelineEffect::kTLBModified;
        return *this;
    }

    AXP_HOT AXP_ALWAYS_INLINE PalResult& iplChanged() noexcept {
        sideEffects |= PipelineEffect::kIPLChanged;
        return *this;
    }

    AXP_HOT AXP_ALWAYS_INLINE PalResult& contextSwitched() noexcept {
        sideEffects |= PipelineEffect::kContextSwitched;
        return *this;
    }

    AXP_HOT AXP_ALWAYS_INLINE PalResult& memoryBarrier() noexcept {
        sideEffects |= PipelineEffect::kMemoryBarrier;
        return *this;
    }

    AXP_HOT AXP_ALWAYS_INLINE PalResult& pcbbChanged() noexcept {
        sideEffects |= PipelineEffect::kPCBBChanged;
        return *this;
    }

    AXP_HOT AXP_ALWAYS_INLINE PalResult& drainWriteBuffers() noexcept {
        sideEffects |= PipelineEffect::kDrainWriteBuffers;
        return *this;
    }

    AXP_HOT AXP_ALWAYS_INLINE PalResult& flushPendingTraps() noexcept {
        sideEffects |= PipelineEffect::kFlushPendingTraps;
        return *this;
    }

   
    AXP_HOT AXP_ALWAYS_INLINE PalResult& notifyHalt() noexcept {
        sideEffects |= PipelineEffect::kNotifyHalt;
        return *this;
    }

    AXP_HOT AXP_ALWAYS_INLINE PalResult& requestPipelineFlush(quint64 flushPC = 0) noexcept {
        sideEffects |= PipelineEffect::kRequestPipelineFlush;
        pipelineFlushPC = flushPC;
        return *this;
    }

    AXP_HOT AXP_ALWAYS_INLINE PalResult& clearBranchPredictor() noexcept {
        sideEffects |= PipelineEffect::kClearBranchPredictor;
        return *this;
    }

    AXP_HOT AXP_ALWAYS_INLINE PalResult& flushPendingIPRWrites() noexcept {
        sideEffects |= PipelineEffect::kFlushPendingIPRWrites;
        return *this;
    }

    // -----------------------------------------------------------------
    // Side-effect queries (run loop reads these)
    // -----------------------------------------------------------------

    AXP_HOT AXP_ALWAYS_INLINE bool has(quint32 flag) const noexcept {
        return (sideEffects & flag) != 0;
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasAnySideEffects() const noexcept {
        return sideEffects != PipelineEffect::kNone;
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasMemoryBarrier()        const noexcept { return has(PipelineEffect::kMemoryBarrier); }
    AXP_HOT AXP_ALWAYS_INLINE bool hasDrainWriteBuffers()    const noexcept { return has(PipelineEffect::kDrainWriteBuffers); }
    AXP_HOT AXP_ALWAYS_INLINE bool hasFlushPendingTraps()    const noexcept { return has(PipelineEffect::kFlushPendingTraps); }
    AXP_HOT AXP_ALWAYS_INLINE bool hasNotifyHalt()           const noexcept { return has(PipelineEffect::kNotifyHalt); }
    AXP_HOT AXP_ALWAYS_INLINE bool hasRequestPipelineFlush() const noexcept { return has(PipelineEffect::kRequestPipelineFlush); }
    AXP_HOT AXP_ALWAYS_INLINE bool hasClearBranchPredictor() const noexcept { return has(PipelineEffect::kClearBranchPredictor); }
    // -----------------------------------------------------------------
    // Reset (clear all fields for reuse)
    // -----------------------------------------------------------------
    AXP_HOT AXP_ALWAYS_INLINE void clear() noexcept {
        palFunction = 0;
        hasReturnValue = false;
        returnReg = PalReturnReg::NONE;
        returnValue = 0;
        doesReturn = true;
        pcModified = false;
        entryPC = 0;
        newPC = 0;
        faultPC = 0;
        faultVA = 0;
        pipelineFlushPC = 0;
        trapCode = {};
        psModified = false;
        newPS = 0;
        iplModified = false;
        newIPL = 0;
        asnModified = false;
        newASN = 0;
        raisesException = false;
        exceptionVector = 0;
        haltCode = 0;
        sideEffects = PipelineEffect::kNone;
    }

    // -----------------------------------------------------------------
    // Factory helpers
    // -----------------------------------------------------------------

    static PalResult Return(PalReturnReg reg, quint64 value) noexcept
    {
        PalResult r{};
        r.hasReturnValue = true;
        r.returnReg = reg;
        r.returnValue = value;
        return r;
    }

    static PalResult NoReturn() noexcept
    {
        PalResult r{};
        r.doesReturn = false;
        return r;
    }
};


// ============================================================================
// PalStatus -- Overall PAL operation outcome
// ============================================================================
enum class PalStatus
{
    Success,
    Fault,
    RequiresPalMode,
    Halt,
    Retry
};


// ============================================================================
// PalEntryReason -- Why PAL was entered
// ============================================================================
enum class PalEntryReason {
    CALL_PAL_INSTRUCTION,
    FAULT_DTBM,
    FAULT_ITB,
    FAULT_ARITH,
    FAULT_UNALIGNED,
    INTERRUPT,
    AST,
    FAULT_ACV,
    MACHINE_CHECK,
    TRAP
};


// ============================================================================
// Privilege Mode Constants (from Mode_Privilege enum)
// ============================================================================
static constexpr quint8 CM_KERNEL = static_cast<quint8>(Mode_Privilege::Kernel);
static constexpr quint8 CM_USER = static_cast<quint8>(Mode_Privilege::User);
static constexpr quint8 CM_EXECUTIVE = static_cast<quint8>(Mode_Privilege::Executive);
static constexpr quint8 CM_SUPERVISOR = static_cast<quint8>(Mode_Privilege::Supervisor);


// ============================================================================
// PAL Privileged Opcodes (ASA 3-10)
// ============================================================================
static constexpr quint8 OPCODE_CALL_PAL = 0x00;
static constexpr quint8 HW_MFPR = 0x19;
static constexpr quint8 HW_MTPR = 0x1D;
static constexpr quint8 HW_ST = 0x1F;
static constexpr quint8 HW_LD = 0x1B;
static constexpr quint8 HW_REI = 0x1E;

#endif // PAL_CORE_H