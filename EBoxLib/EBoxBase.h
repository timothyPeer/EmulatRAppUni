#ifndef EBOXBASE_INL_H
#define EBOXBASE_INL_H

#include "coreLib/AMASK_Constants_inl.h"
#include "faultLib/FaultDispatcher.h"
#include "grainFactoryLib/DecodedInstruction.h"
#include "grainFactoryLib/DecodedInstruction_inl.h"
#include "coreLib/alpha_int_helpers_inl.h"
#include "coreLib/alpha_alu_inl.h"
#include "coreLib/alpha_int_byteops_inl.h"
#include "faultLib/GlobalFaultDispatcherBank.h"
#include "faultLib/global_faultDispatcher.h"
#include "faultLib/PendingEvent_Refined.h"
#include "cpuCoreLib/registerBank_coreFramework.h"

#include "coreLib/EnvironVariables.h"
#include "coreLib/register_core_inl.h"
#include "machineLib/PipeLineSlot.h"

// Forward declarations
class PipelineSlot;

// ============================================================================
// EBox - Integer Execution Unit (All-Inline Implementation)
// ============================================================================
// Responsibilities:
// - Integer arithmetic operations (ADDQ, SUBQ, MULQ, etc.)
// - Logical operations (AND, OR, XOR, NOT)
// - Shift operations (SLL, SRL, SRA)
// - Byte manipulation operations (ZAP, ZAPNOT, EXTBL, etc.)
// - Address calculations (LDA, LDAH)
// - Comparison operations (CMPEQ, CMPLT, CMPULE, etc.)
// - Conditional move operations (CMOVEQ, CMOVLT, etc.)
// - Scaled arithmetic (S4ADDL, S8ADDQ, etc.)
// - Integer overflow trap handling
// ============================================================================

class EBox
{
public:
    // ====================================================================
    // Constructor & Destructor
    // ====================================================================
    explicit EBox(CPUIdType cpuId)
        : m_busy(false)
          , m_cyclesRemaining(0)
          , m_cpuId(cpuId)
          , m_faultSink(&globalFaultDispatcher(cpuId))
          , m_intRegisterDirty{0}
        , m_iprGlobalMaster(getCPUStateView(cpuId))
    {

    }

    ~EBox() = default;

    // ====================================================================
    // Halt Code Management
    // ====================================================================
    AXP_ALWAYS_INLINE auto hasHaltReason(BIP_RC_Flag f) const noexcept -> bool
    {
        return (m_iprGlobalMaster->r->halt_code &
            static_cast<quint8>(f)) != 0;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto clearHaltCode() const noexcept -> void
    {
        m_iprGlobalMaster->r->halt_code = 0;
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto setScoreboard(const PipelineSlot& slot)  noexcept -> void
    {
        if (!writesRegister(slot.di)) return;

        const quint8 destReg = destRegister(slot.di);
        if (destReg == 31) return;  // R31 never dirty

        // Set dirty bit
        m_intRegisterDirty |= (1u << destReg);
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto clearScoreboard(const PipelineSlot& slot)  noexcept -> void
    {
        if (!writesRegister(slot.di)) return;

        const quint8 destReg = destRegister(slot.di);
        if (destReg == 31) return;

        // Clear dirty bit
        m_intRegisterDirty &= ~(1u << destReg);
    }

    // ====================================================================
    // Pipeline Control
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE  auto isBusy() const noexcept -> bool
    {
        return m_busy;
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto hasTrapThisCycle() const noexcept -> bool
    {
        return globalFaultDispatcher(m_cpuId).eventPending();
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto tick()  noexcept -> void
    {
        if (m_busy && m_cyclesRemaining > 0)
        {
            m_cyclesRemaining--;
            if (m_cyclesRemaining == 0)
            {
                m_busy = false;
            }
        }
    }

    // ====================================================================
    // Address Calculation Instructions
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE auto executeLDA(PipelineSlot& slot) const noexcept -> void
    {
        // LDA RC, displacement(RA)
        // RC = RA + sign_extend(displacement)
        quint64 raValue      = slot.readIntReg(slot.di.ra);
        qint16  displacement = static_cast<qint16>(slot.di.branch_disp);
        quint64 result       = raValue + static_cast<qint64>(displacement);


        quint32 raw2 = slot.di.rawBits();
        qDebug() << "=== LDA GRAIN EXECUTE ===";
        qDebug() << "         raw:" << Qt::hex << raw2;
        qDebug() << "          PC:" << Qt::hex << slot.di.pc;
        qDebug() << "      PC + 4:" << Qt::hex << (slot.di.pc + 4);
        qDebug() << "          ra: " << raValue;
        qDebug() << "br displment: " << displacement;
        qDebug() << " result: " << result;

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeLDAH(PipelineSlot& slot) const noexcept -> void
    {
        // LDAH RC, displacement(RA) 
        // RC = RA + (sign_extend(displacement) << 16)
        quint64 raValue      = slot.readIntReg(slot.di.ra);
        qint16  displacement = static_cast<qint16>(slot.di.branch_disp);
        qint64  shiftedDisp  = static_cast<qint64>(displacement) << 16;
        quint64 result       = raValue + shiftedDisp;

        quint32 raw2 = slot.di.rawBits();
        qDebug() << "=== LDAH GRAIN EXECUTE ===";
        qDebug() << "         raw:" << Qt::hex << raw2;
        qDebug() << "          PC:" << Qt::hex << slot.di.pc;
        qDebug() << "      PC + 4:" << Qt::hex << (slot.di.pc + 4);
        qDebug() << "          ra: " << raValue;
        qDebug() << "br displment: " << displacement;
        qDebug() << "shift displt: " << shiftedDisp;
        qDebug() << " result: " << result;
        slot.payLoad        = result;
        slot.needsWriteback = true;
    }


    /*
AMASK is used by system software to query which architectural extensions are implemented by the processor.
Architecturally safe EV6 AMASK bits

These are the bits that EV6-class systems commonly expose and OSes expect:

Bit	Name	EV6 status	Notes
0	BWX	x Yes	Byte/word extensions
1	FIX	x Yes	Integer extensions
2	CIX	x Yes	Count extensions
3	MVI	! Optional	Multimedia (only if implemented)
4	PAT	! Optional	Prefetch assist
5	PM	! Optional	Performance monitoring
*/

    AXP_HOT AXP_ALWAYS_INLINE	void executeAMASK(PipelineSlot& slot) noexcept
    {
        // =========================================================================
        // AMASK - Architectural Feature Mask Query
        // =========================================================================
        // Input : R16 = mask selector
        // Output: R0  = feature mask
        // =========================================================================

        const quint64 selector = slot.readIntReg( 16);
        quint64 mask = 0;

        switch (selector)
        {
        case 0:
            // Architectural feature mask
            mask = AMASK_EMULATOR_SUPPORTED;
            break;

        case 1:
            // Implementation-specific extensions
            // (none exposed yet)
            mask = 0;
            break;

        default:
            // Reserved selectors return zero
            mask = 0;
            break;
        }

      
        slot.needsWriteback = false;
    }
    // ====================================================================
    // Integer Arithmetic Operations
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE auto executeADDL(PipelineSlot& slot)  noexcept -> void
    {
        const qint32 srcA = static_cast<qint32>(slot.readIntReg(slot.di.ra));
        const qint32 srcB = getOperandB_32(slot);

        IntStatus    status;
        const qint32 result = addL(srcA, srcB, status);

        if (status.hasError())
        {
            handleTrap(slot, status);
            slot.faultPending = true;
        }

        quint32 raw2 = slot.di.rawBits();
        qDebug() << "=== ADDL GRAIN EXECUTE ===";
        qDebug() << "  raw:" << Qt::hex << raw2;
        qDebug() << "  PC:" << Qt::hex << slot.di.pc;
        qDebug() << "  PC + 4:" << Qt::hex << (slot.di.pc + 4);
        qDebug() << "     ra: " << srcA;
        qDebug() << "     rb: " << srcB;
        qDebug() << " result: " << result;

        slot.payLoad        = static_cast<qint64>(result);  // Sign-extend
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeADDQ(PipelineSlot& slot) const noexcept -> void
    {
        // ADDQ Ra, Rb/#lit, Rc
        // Rc = Ra + Rb (or literal)

        const quint64 raValue = slot.readIntReg(slot.di.ra);
        const quint64 rbValue = getOperandB_64(slot);
        const quint64 result  = raValue + rbValue;

        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, raValue, rbValue, result, "ADDQ");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSUBL(PipelineSlot& slot)  noexcept -> void
    {
        const qint32 srcA = static_cast<qint32>(slot.readIntReg(slot.di.ra));
        const qint32 srcB = getOperandB_32(slot);

        IntStatus    status;
        const qint32 result = subL(srcA, srcB, status);

        if (status.hasError())
        {
            handleTrap(slot, status);
            slot.faultPending = true;
        }


        /**
             * @brief Check if instruction can dual-issue
             *
             * Dual-issue instructions can execute in parallel with another instruction
             * in the same cycle (superscalar execution).
             *
             * @return true if can dual-issue, false otherwise
             */

        slot.payLoad        = static_cast<qint64>(result);  // Sign-extend
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSUBQ(PipelineSlot& slot) const noexcept -> void
    {
        const quint64 raValue = slot.readIntReg(slot.di.ra);
        const quint64 rbValue = getOperandB_64(slot);
        const quint64 result  = raValue - rbValue;

        debugInteger("EXEC", slot, raValue, rbValue, result, "SUBQ");

        if (slot.di.rc != 31)
        {
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMULL(PipelineSlot& slot)  noexcept -> void
    {
        const qint32 srcA = static_cast<qint32>(slot.readIntReg(slot.di.ra));
        const qint32 srcB = getOperandB_32(slot);

        IntStatus    status;
        const qint32 result = mulL(srcA, srcB, status);

        if (status.hasError())
        {
            handleTrap(slot, status);
            slot.faultPending = true;
        }


        slot.payLoad        = static_cast<qint64>(result);  // Sign-extend
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMULQ(PipelineSlot& slot)  noexcept -> void
    {
        const qint64 srcA = static_cast<qint64>(slot.readIntReg(slot.di.ra));
        const qint64 srcB = getOperandB_64(slot);

        IntStatus    status;
        const qint64 result = mulQ(srcA, srcB, status);

        if (status.hasError())
        {
            handleTrap(slot, status);
            slot.faultPending = true;
        }

        debugInteger("EXEC", slot, srcA, srcB, result, "SUBQ");
        slot.payLoad        = static_cast<quint64>(result);
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeUMULH(PipelineSlot& slot) const noexcept -> void
    {
        // UMULH - Unsigned Multiply High
        // RC = (Ra * Rb) >> 64 (upper 64 bits of 128-bit product)
        const quint64 srcA = slot.readIntReg(slot.di.ra);
        const quint64 srcB = getOperandB_64(slot);

        // Use __uint128_t or manual multiplication
#if defined(__SIZEOF_INT128__)
        __uint128_t   product = static_cast<__uint128_t>(srcA) * static_cast<__uint128_t>(srcB);
        const quint64 result  = static_cast<quint64>(product >> 64);
#else
        // Fallback: Split into 32-bit parts
        const quint64 a_lo = srcA & 0xFFFFFFFF;
        const quint64 a_hi = srcA >> 32;
        const quint64 b_lo = srcB & 0xFFFFFFFF;
        const quint64 b_hi = srcB >> 32;

        const quint64 p0 = a_lo * b_lo;
        const quint64 p1 = a_lo * b_hi;
        const quint64 p2 = a_hi * b_lo;
        const quint64 p3 = a_hi * b_hi;

        const quint64 mid    = p1 + p2 + (p0 >> 32);
        const quint64 result = p3 + (mid >> 32);
#endif


        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    /*
     *VAX Compatibility Read and Set
     */
    AXP_HOT AXP_ALWAYS_INLINE  auto executeRS(PipelineSlot& slot) const -> void
    {
        slot.payLoad = m_iprGlobalMaster->r->intrFlag ? 1 : 0;
        slot.needsWriteback = true;
        slot.writeRa = true;
        m_iprGlobalMaster->r->intrFlag = false;
    }

    // Suggested placement: add this bit to your per-CPU hot IPR storage or processor context.
    // Example:
    //   struct IPRStorage_Hot64 { ... bool intrFlag{false}; ... };
    //
    // IMPORTANT: Also clear this bit in your PAL REI path (CALL_PAL REI).
    AXP_HOT AXP_ALWAYS_INLINE auto executeRC(PipelineSlot& slot) noexcept -> void
    {
        slot.payLoad = m_iprGlobalMaster->r->intrFlag ? 1 : 0;
        slot.needsWriteback = true;
        slot.writeRa = true;
        m_iprGlobalMaster->r->intrFlag = true;
    }

    // ====================================================================
    // Scaled Arithmetic (Sx{ADD|SUB}{L|Q})
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE auto executeS4ADDL(PipelineSlot& slot) const noexcept -> void
    {
        // RC = (RA * 4) + RB (32-bit, sign-extended)
        const qint32 raValue = static_cast<qint32>(slot.readIntReg(slot.di.ra));
        const qint32 rbValue = getOperandB_32(slot);
        const qint32 result  = (raValue * 4) + rbValue;

        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, raValue, rbValue, result, "S4ADDL");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeS8ADDL(PipelineSlot& slot) const noexcept -> void
    {
        // RC = (RA * 8) + RB (32-bit, sign-extended)
        const qint32 raValue = static_cast<qint32>(slot.readIntReg(slot.di.ra));
        const qint32 rbValue = getOperandB_32(slot);
        const qint32 result  = (raValue * 8) + rbValue;

        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, raValue, rbValue, result, "S8ADDL");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeS4ADDQ(PipelineSlot& slot) const noexcept -> void
    {
        // RC = (RA * 4) + RB (64-bit)
        const qint64 raValue = static_cast<qint64>(slot.readIntReg(slot.di.ra));
        const qint64 rbValue = getOperandB_64(slot);
        const qint64 result  = (raValue * 4) + rbValue;


        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, raValue, rbValue, result, "S4ADDQ");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeS8ADDQ(PipelineSlot& slot) const noexcept -> void
    {
        // RC = (RA * 8) + RB (64-bit)
        const qint64 raValue = static_cast<qint64>(slot.readIntReg(slot.di.ra));
        const qint64 rbValue = getOperandB_64(slot);
        const qint64 result  = (raValue * 8) + rbValue;


        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, raValue, rbValue, result, "S8ADDQ");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeS4SUBL(PipelineSlot& slot) const noexcept -> void
    {
        // RC = (RA * 4) - RB (32-bit, sign-extended)
        const qint32 raValue = static_cast<qint32>(slot.readIntReg(slot.di.ra));
        const qint32 rbValue = getOperandB_32(slot);
        const qint32 result  = (raValue * 4) - rbValue;


        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, raValue, rbValue, result, "S4SUBL");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeS8SUBL(PipelineSlot& slot) const noexcept -> void
    {
        // RC = (RA * 8) - RB (32-bit, sign-extended)
        const qint32 raValue = static_cast<qint32>(slot.readIntReg(slot.di.ra));
        const qint32 rbValue = getOperandB_32(slot);
        const qint32 result  = (raValue * 8) - rbValue;


        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, raValue, rbValue, result, "ADDQ");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeS4SUBQ(PipelineSlot& slot) const noexcept -> void
    {
        // RC = (RA * 4) - RB (64-bit)
        const qint64 raValue = static_cast<qint64>(slot.readIntReg(slot.di.ra));
        const qint64 rbValue = getOperandB_64(slot);
        const qint64 result  = (raValue * 4) - rbValue;


        slot.payLoad        = static_cast<quint64>(result);
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeS8SUBQ(PipelineSlot& slot) const noexcept -> void
    {
        // RC = (RA * 8) - RB (64-bit)
        const qint64 raValue = static_cast<qint64>(slot.readIntReg(slot.di.ra));
        const qint64 rbValue = getOperandB_64(slot);
        const qint64 result  = (raValue * 8) - rbValue;


        slot.payLoad        = static_cast<quint64>(result);
        slot.needsWriteback = true;
    }

    // ====================================================================
    // Logical Operations
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE auto executeAND(PipelineSlot& slot) const noexcept -> void
    {
        const quint64 raValue = slot.readIntReg(slot.di.ra);
        const quint64 rbValue = getOperandB_64(slot);
        const quint64 result  = raValue & rbValue;

        debugInteger("EXEC", slot, raValue, rbValue, result, "AND");

        if (slot.di.rc != 31)
        {
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeBIS(PipelineSlot& slot) const noexcept -> void
    {
        const quint64 raValue = slot.readIntReg(slot.di.ra);
        const quint64 rbValue = getOperandB_64(slot);
        const quint64 result  = raValue | rbValue;

        debugInteger("EXEC", slot, raValue, rbValue, result, "BIS (OR)");

        if (slot.di.rc != 31)
        {
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeXOR(PipelineSlot& slot) const noexcept -> void
    {
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = srcA ^ srcB;


        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeBIC(PipelineSlot& slot) const noexcept -> void
    {
        // BIC = Bit Clear (AND NOT)
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = srcA & (~srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeORNOT(PipelineSlot& slot) const noexcept -> void
    {
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = srcA | (~srcB);


        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeEQV(PipelineSlot& slot) const noexcept -> void
    {
        // EQV = XNOR (Equivalence)
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = ~(srcA ^ srcB);


        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    // ====================================================================
    // Shift Operations
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE  auto executeSLL(PipelineSlot& slot) const noexcept -> void
    {
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_alu::sll(srcA, srcB);


        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeSRL(PipelineSlot& slot) const noexcept -> void
    {
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_alu::srl(srcA, srcB);


        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeSRA(PipelineSlot& slot) const noexcept -> void
    {
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_alu::sra(srcA, srcB);


        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    // ====================================================================
    // Comparison Operations
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE void executeCMPEQ(PipelineSlot& slot) const noexcept
    {
        const DecodedInstruction& di = slot.di;

        // Read operands
        const quint64 raValue = slot.readIntReg(di.ra);
        const quint64 rbValue = (di.literal_val != 0)
            ? static_cast<quint64>(di.literal_val)
            : slot.readIntReg(di.rb);

        // Compare
        const quint64 result = (raValue == rbValue) ? 1ULL : 0ULL;

        // Debug
        qDebug() << QString("[EXEC::INTEGER] CMPEQ | PC: 0x%1 | Ra: R%2 | Rb: R%3 | Rc: R%4")
            .arg(di.pc, 16, 16, QChar('0'))
            .arg(di.ra)
            .arg(di.rb)
            .arg(di.rc);
        qDebug() << QString("                CMPEQ: 0x%1 == 0x%2 = %3")
            .arg(raValue, 16, 16, QChar('0'))
            .arg(rbValue, 16, 16, QChar('0'))
            .arg(result);

        // Writeback (EXACT SAME PATTERN!)
        if (di.rc != 31) {
            slot.payLoad = result;
            slot.needsWriteback = true;
            slot.writeRa = true;
        }
        else {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeCMPLT(PipelineSlot& slot) const noexcept -> void
    {
        const qint64 srcA   = slot.readIntReg(slot.di.ra);
        const qint64 srcB   = getOperandB_64(slot);
        const quint64 result = (srcA < srcB) ? 1ULL : 0ULL;

        debugInteger("EXEC", slot, srcA, srcB, result, "CMPLT");

        // Writeback (SAME PATTERN!)
        if (slot.di.rc != 31) {
            slot.payLoad = result;
            slot.needsWriteback = true;
            slot.writeRa = true;  // Generic "write int register" flag
        }
        else {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeCMPLE(PipelineSlot& slot) const noexcept -> void
    {
        const qint64 srcA   = slot.readIntReg(slot.di.ra);
        const qint64 srcB   = getOperandB_64(slot);
        const quint64 result = (srcA <= srcB) ? 1ULL : 0ULL;

        debugInteger("EXEC", slot, srcA, srcB, result, "CMPLE");

        // Writeback (SAME PATTERN!)
        if (slot.di.rc != 31) {
            slot.payLoad = result;
            slot.needsWriteback = true;
            slot.writeRa = true;  // Generic "write int register" flag
        }
        else {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeCMPULT(PipelineSlot& slot) const noexcept -> void
    {
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = (srcA < srcB) ? 1ULL : 0ULL;

        debugInteger("EXEC", slot, srcA, srcB, result, "CMPULT");

        // Writeback (SAME PATTERN!)
        if (slot.di.rc != 31) {
            slot.payLoad = result;
            slot.needsWriteback = true;
            slot.writeRa = true;  // Generic "write int register" flag
        }
        else {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeCMPULE(PipelineSlot& slot) const noexcept -> void
    {
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = (srcA <= srcB) ? 1 : 0;  // Unsigned
        
        debugInteger("EXEC", slot, srcA, srcB, result, "CMPULE");

        // Writeback (SAME PATTERN!)
        if (slot.di.rc != 31) {
            slot.payLoad = result;
            slot.needsWriteback = true;
            slot.writeRa = true;  // Generic "write int register" flag
        }
        else {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeCMPBGE(PipelineSlot& slot) const noexcept -> void
    {
        // CMPBGE - Compare Bytes Greater or Equal
        // For each byte i: if RA[i] >= RB[i] then set bit i in result
        const quint64 srcA = slot.readIntReg(slot.di.ra);
        const quint64 srcB = getOperandB_64(slot);

        quint64 result = 0;
        for (int i = 0; i < 8; i++)
        {
            const quint8 byteA = (srcA >> (i * 8)) & 0xFF;
            const quint8 byteB = (srcB >> (i * 8)) & 0xFF;
            if (byteA >= byteB)
            {
                result |= (1ULL << i);
            }
        }


        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    // ====================================================================
    // Conditional Move Operations
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE  auto executeCMOVEQ(PipelineSlot& slot) const noexcept -> void
    {
        const quint64 srcA = slot.readIntReg(slot.di.ra);
        const quint64 srcB = getOperandB_64(slot);

        if (srcA == 0)
        {
            slot.payLoad        = srcB;
            slot.needsWriteback = true;
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeCMOVNE(PipelineSlot& slot) const noexcept -> void
    {
        const quint64 srcA = slot.readIntReg(slot.di.ra);
        const quint64 srcB = getOperandB_64(slot);

        if (srcA != 0)
        {
            slot.payLoad        = srcB;
            slot.needsWriteback = true;
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeCMOVLT(PipelineSlot& slot) const noexcept -> void
    {
        const qint64  srcA = static_cast<qint64>(slot.readIntReg(slot.di.ra));
        const quint64 srcB = getOperandB_64(slot);

        if (srcA < 0)
        {
            slot.payLoad        = srcB;
            slot.needsWriteback = true;
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeCMOVGE(PipelineSlot& slot) const noexcept -> void
    {
        const qint64  srcA = static_cast<qint64>(slot.readIntReg(slot.di.ra));
        const quint64 srcB = getOperandB_64(slot);

        if (srcA >= 0)
        {
            slot.payLoad        = srcB;
            slot.needsWriteback = true;
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeCMOVLE(PipelineSlot& slot) const noexcept -> void
    {
        const qint64  srcA = static_cast<qint64>(slot.readIntReg(slot.di.ra));
        const quint64 srcB = getOperandB_64(slot);

        if (srcA <= 0)
        {
            slot.payLoad        = srcB;
            slot.needsWriteback = true;
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeCMOVGT(PipelineSlot& slot) const noexcept -> void
    {
        const qint64  srcA = static_cast<qint64>(slot.readIntReg(slot.di.ra));
        const quint64 srcB = getOperandB_64(slot);

        if (srcA > 0)
        {
            slot.payLoad        = srcB;
            slot.needsWriteback = true;
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeCMOVLBS(PipelineSlot& slot) const noexcept -> void
    {
        // Move if Low Bit Set
        const quint64 srcA = slot.readIntReg(slot.di.ra);
        const quint64 srcB = getOperandB_64(slot);

        if ((srcA & 1) != 0)
        {
            slot.payLoad        = srcB;
            slot.needsWriteback = true;
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto executeCMOVLBC(PipelineSlot& slot) const noexcept -> void
    {
        // Move if Low Bit Clear
        const quint64 srcA = slot.readIntReg(slot.di.ra);
        const quint64 srcB = getOperandB_64(slot);

        if ((srcA & 1) == 0)
        {
            slot.payLoad        = srcB;
            slot.needsWriteback = true;
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    // ====================================================================
    // Overflow-Trapping Arithmetic
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE auto executeADDL_V(PipelineSlot& slot)  noexcept -> void
    {
        const qint32 srcA = static_cast<qint32>(slot.readIntReg(slot.di.ra));
        const qint32 srcB = getOperandB_32(slot);

        IntStatus    status;
        const qint32 result = addL(srcA, srcB, status);

        if (status.hasError())
        {
            handleTrap(slot, status);
            slot.faultPending = true;
        }

        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, srcA, srcB, result, "ADDL_V");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeADDQ_V(PipelineSlot& slot)  noexcept -> void
    {
        const qint64 srcA = slot.readIntReg(slot.di.ra);
        const qint64 srcB = getOperandB_64(slot);

        IntStatus    status;
        const qint64 result = addQ(srcA, srcB, status);

        if (status.hasError())
        {
            handleTrap(slot, status);
            slot.faultPending = true;
        }


        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, srcA, srcB, result, "ADDQ_V");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSUBL_V(PipelineSlot& slot)  noexcept -> void
    {
        const qint32 srcA = static_cast<qint32>(slot.readIntReg(slot.di.ra));
        const qint32 srcB = getOperandB_32(slot);

        IntStatus    status;
        const qint32 result = subL(srcA, srcB, status);

        if (status.hasError())
        {
            handleTrap(slot, status);
            slot.faultPending = true;
        }

        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, srcA, srcB, result, "SUBL_V");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSUBQ_V(PipelineSlot& slot)  noexcept -> void
    {
        const qint64 srcA = slot.readIntReg(slot.di.ra);
        const qint64 srcB = getOperandB_64(slot);

        IntStatus    status;
        const qint64 result = subQ(srcA, srcB, status);

        if (status.hasError())
        {
            handleTrap(slot, status);
            slot.faultPending = true;
        }

        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, srcA, srcB, result, "SUBQ_V");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMULL_V(PipelineSlot& slot)  noexcept -> void
    {
        const qint32 srcA = static_cast<qint32>(slot.readIntReg(slot.di.ra));
        const qint32 srcB = getOperandB_32(slot);

        IntStatus    status;
        const qint32 result = mulL(srcA, srcB, status);

        if (status.hasError())
        {
            handleTrap(slot, status);
            slot.faultPending = true;
        }


        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, srcA, srcB, result, "MULL_V");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMULQ_V(PipelineSlot& slot)  noexcept -> void
    {
        const qint64 srcA = slot.readIntReg(slot.di.ra);
        const qint64 srcB = getOperandB_64(slot);

        IntStatus    status;
        const qint64 result = mulQ(srcA, srcB, status);

        if (status.hasError())
        {
            handleTrap(slot, status);
            slot.faultPending = true;
        }


        // ================================================================
        // DEBUG: Show integer operation
        // ================================================================
        debugInteger("EXEC", slot, srcA, srcB, result, "MULQ_V");

        // ================================================================
        // Writeback setup
        // ================================================================
        if (slot.di.rc != 31)
        {  // Don't write to R31
            slot.payLoad        = result;
            slot.needsWriteback = true;
            slot.writeRa        = true;  // Actually writes to Rc
        }
        else
        {
            slot.needsWriteback = false;
        }
    }

    // ====================================================================
    // Byte Manipulation Operations
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE auto executeZAP(PipelineSlot& slot) const noexcept -> void
    {
        // ZAP - Zero Bytes (mask specifies which bytes to zero)
        const quint64 srcA = slot.readIntReg(slot.di.ra);
        const quint64 mask = getOperandB_64(slot);

        quint64 result = srcA;
        for (int i = 0; i < 8; i++)
        {
            if (mask & (1ULL << i))
            {
                result &= ~(0xFFULL << (i * 8));  // Zero byte i
            }
        }

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeZAPNOT(PipelineSlot& slot) const noexcept -> void
    {
        // ZAPNOT - Zero All Bytes NOT in mask
        const quint64 srcA = slot.readIntReg(slot.di.ra);
        const quint64 mask = getOperandB_64(slot);

        quint64 result = 0;
        for (int i = 0; i < 8; i++)
        {
            if (mask & (1ULL << i))
            {
                result |= srcA & (0xFFULL << (i * 8));  // Keep byte i
            }
        }

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    // TODO - Multimedia Support 
    AXP_HOT AXP_ALWAYS_INLINE auto executeMAXUB8(PipelineSlot& slot) -> void
    {
        static bool warned = false;
        if (!warned)
        {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    } 
    AXP_HOT AXP_ALWAYS_INLINE auto executeMSKBL(PipelineSlot& slot) const noexcept -> void
    {
        // MSKBL - Mask Byte Low
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::mskbl(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMSKWL(PipelineSlot& slot) const noexcept -> void
    {
        // MSKWL - Mask Word Low
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::mskwl(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMSKLL(PipelineSlot& slot) const noexcept -> void
    {
        // MSKLL - Mask Longword Low
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::mskll(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMSKQL(PipelineSlot& slot) const noexcept -> void
    {
        // MSKQL - Mask Quadword Low
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::mskql(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMSKWH(PipelineSlot& slot) const noexcept -> void
    {
        // MSKWH - Mask Word High
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::mskwh(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMSKLH(PipelineSlot& slot) const noexcept -> void
    {
        // MSKLH - Mask Longword High
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::msklh(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMSKQH(PipelineSlot& slot) const noexcept -> void
    {
        // MSKQH - Mask Quadword High
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::mskqh(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }



    AXP_HOT AXP_ALWAYS_INLINE auto executeEXTBL(PipelineSlot& slot) const noexcept -> void
    {
        // EXTBL - Extract Byte Low
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::extbl(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeEXTWL(PipelineSlot& slot) const noexcept -> void
    {
        // EXTWL - Extract Word Low
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::extwl(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeEXTLL(PipelineSlot& slot) const noexcept -> void
    {
        // EXTLL - Extract Longword Low
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::extll(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeEXTQL(PipelineSlot& slot) const noexcept -> void
    {
        // EXTQL - Extract Quadword Low
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::extql(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeEXTWH(PipelineSlot& slot) const noexcept -> void
    {
        // EXTWH - Extract Word High
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::extwh(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeEXTLH(PipelineSlot& slot) const noexcept -> void
    {
        // EXTLH - Extract Longword High
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::extlh(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeEXTQH(PipelineSlot& slot) const noexcept -> void
    {
        // EXTQH - Extract Quadword High
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::extqh(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSBL(PipelineSlot& slot) const noexcept -> void
    {
        // INSBL - Insert Byte Low
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::insbl(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSWL(PipelineSlot& slot) const noexcept -> void
    {
        // INSWL - Insert Word Low
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::inswl(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSLL(PipelineSlot& slot) const noexcept -> void
    {
        // INSLL - Insert Longword Low
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::insll(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQL(PipelineSlot& slot) const noexcept -> void
    {
        // INSQL - Insert Quadword Low
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::insql(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSWH(PipelineSlot& slot) const noexcept -> void
    {
        // INSWH - Insert Word High
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::inswh(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSLH(PipelineSlot& slot) const noexcept -> void
    {
        // INSLH - Insert Longword High
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::inslh(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQH(PipelineSlot& slot) const noexcept -> void
    {
        // INSQH - Insert Quadword High
        const quint64 srcA   = slot.readIntReg(slot.di.ra);
        const quint64 srcB   = getOperandB_64(slot);
        const quint64 result = alpha_byteops::insqh(srcA, srcB);

        slot.payLoad        = result;
        slot.needsWriteback = true;
    }

    // ====================================================================
    // Scoreboard / Hazard Tracking
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE auto markRegisterDirty(quint8 reg)  noexcept -> void
    {
        if (reg == 31) return;  // R31/F31 never dirty

        const quint32 mask = (1u << reg);
        m_intRegisterDirty |= mask;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto markRegisterDirty(const DecodedInstruction& di)  noexcept -> void
    {
        const quint8 dst = destRegister(di);
        if (dst == 31) return;

        markRegisterDirty(dst);
    }

    AXP_HOT AXP_ALWAYS_INLINE auto clearRegisterDirty(const DecodedInstruction& di)  noexcept -> void
    {
        const quint8 dst = destRegister(di);
        if (dst == 31) return;

        const quint32 mask = (1u << dst);
        m_intRegisterDirty &= ~mask;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto isIntRegDirty(quint8 r) const noexcept -> bool
    {
        return (m_intRegisterDirty & (1u << r)) != 0;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto isRegDirty(const DecodedInstruction& di, quint8 reg) noexcept -> bool
    {
        if (reg == 31) return false;
        return isIntRegDirty(reg);
    }

    AXP_HOT AXP_ALWAYS_INLINE auto clearDirty(quint8 reg) -> void
    {
        if (reg != 31)
        {
            m_intRegisterDirty &= ~(1u << reg);
        }
    }



private:
    // ====================================================================
    // Helper Methods
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE  auto getOperandB_32(PipelineSlot& slot) const noexcept -> qint32
    {
        if (hasLiteralBit(slot.di))
            return static_cast<qint32>(slot.di.literal_val);
        return static_cast<qint32>(slot.readIntReg(slot.di.rb));
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto getOperandB_64(PipelineSlot& slot) const noexcept -> qint64
    {
        if (hasLiteralBit(slot.di))
            return static_cast<qint64>(slot.di.literal_val);
        return static_cast<qint64>(slot.readIntReg(slot.di.rb));
    }

    // ====================================================================
    // Trap Handling
    // ====================================================================
    AXP_HOT inline  auto handleTrap(PipelineSlot& slot, const IntStatus& status)  noexcept -> void
    {
        // Integer arithmetic traps
        if (status.hasOverflow() && canOverflowTrap(slot.di))
        {
            PendingEvent trap;
            trap.exceptionClass = ExceptionClass_EV6::Arithmetic;
            trap.faultPC        = slot.di.pc;
            trap.palVectorId    = PalVectorId_EV6::ARITH;
            trap.eventOperand   = slot.payLoad;

            slot.m_faultDispatcher->setPendingEvent(trap);
            slot.faultPending = true;
            return;
        }

        // Division by zero
        if (status.hasDivideByZero())
        {
            PendingEvent trap;
            trap.exceptionClass = ExceptionClass_EV6::Arithmetic;
            trap.faultPC        = slot.di.pc;
            trap.palVectorId    = PalVectorId_EV6::ARITH;
            trap.eventOperand   = slot.payLoad;

            slot.m_faultDispatcher->setPendingEvent(trap);
            slot.faultPending = true;
            return;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto canOverflowTrap(const DecodedInstruction& di) noexcept -> bool
    {
        return isOverflowTrapInstruction(di) && isTrapsEnabled();
    }

    AXP_HOT AXP_ALWAYS_INLINE  auto isTrapsEnabled() noexcept -> bool
    {
        return m_iprGlobalMaster->h->isIntegerOverflowTrapEnabled();
    }

    // ====================================================================
    // Member Data
    // ====================================================================
    bool                m_busy;
    int                 m_cyclesRemaining;
    CPUIdType           m_cpuId;
    RegisterBankInteger m_intRegister;
    FaultDispatcher*    m_faultSink;

    // IPRs
    CPUStateView  m_cpuView;                            // value member
    CPUStateView* m_iprGlobalMaster{ &m_cpuView };
   

    // ====================================================================
    // Register Scoreboard (RAW Hazard Detection)
    // ====================================================================
    quint32 m_intRegisterDirty;   // Bits 0-31 for R0-R31 integer registers
};

#endif // EBOXBASE_INL_H
