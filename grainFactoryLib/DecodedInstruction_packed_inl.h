#ifndef DECODEDINSTRUCTION_PACKED_INL_H
#define DECODEDINSTRUCTION_PACKED_INL_H

#include "coreLib/Axp_Attributes_core.h"
#include <QtGlobal>
#include "grainFactoryLib/InstructionGrain.h"
#include "DecodedInstruction.h"
#include "grain_core.h"
/**
 * @brief Get Ra field from raw bits
 *
 * COST: 1 cycle
 */
AXP_HOT AXP_ALWAYS_INLINE quint8 getRaFromPacked(quint32 raw) noexcept {
    return static_cast<quint8>((raw >> 21) & 0x1F);
}

/**
 * @brief Get Rb field from raw bits
 *
 * COST: 1 cycle
 */
AXP_HOT AXP_ALWAYS_INLINE quint8 getRbFromPacked(quint32 raw) noexcept {
    return static_cast<quint8>((raw >> 16) & 0x1F);
}

/**
 * @brief Get Rc field from raw bits (operate format)
 *
 * COST: 1 cycle
 */
AXP_HOT AXP_ALWAYS_INLINE quint8 getRcFromPacked(quint32 raw) noexcept {
    return static_cast<quint8>(raw & 0x1F);
}

/**
 * @brief Get function code from raw bits (operate format)
 *
 * COST: 1 cycle
 */
AXP_HOT AXP_ALWAYS_INLINE quint8 getFunctionFromPacked(quint32 raw) noexcept {
    return static_cast<quint8>((raw >> 5) & 0x7F);
}

/**
 * @brief Get memory displacement (sign-extended)
 *
 * COST: 2 cycles
 */
AXP_HOT AXP_ALWAYS_INLINE qint16 getMemDispFromPacked(quint32 raw) noexcept {
    return static_cast<qint16>(raw & 0xFFFF);
}

/**
 * @brief Get branch displacement (sign-extended, pre-shifted by 2)
 *
 * COST: 2 cycles
 */
AXP_HOT AXP_ALWAYS_INLINE qint64 getBranchDispFromPacked(quint32 raw) noexcept {
    qint32 disp21 = static_cast<qint32>(raw << 11) >> 11;  // Sign extend 21 bits
    return static_cast<qint64>(disp21) << 2;  // Branch offsets are *4
}


/**
 * @brief Get opcode from raw instruction bits
 *
 * Alpha opcode is in bits [31:26] (6 bits)
 *
 * @param raw Raw 32-bit instruction word
 * @return Opcode (0-63)
 *
 * COST: 1 cycle (shift + mask)
 */
AXP_HOT AXP_ALWAYS_INLINE quint8 getOpcodeFromPacked(quint32 raw) noexcept {
    return static_cast<quint8>((raw >> 26) & 0x3F);
}


/**
 * @brief Convert GrainType enum to ASCII string
 */
inline const char* getGrainTypeName(GrainType type) noexcept {
    switch (type) {
    case GrainType::IntegerOperate:    return "IntOp";
    case GrainType::FloatOperate:      return "FloatOp";
   case GrainType::Branch:            return "Branch";
   
    case GrainType::Jump:              return "Jump";
    case GrainType::ControlFlow:       return "Control";
   
    case GrainType::PALcode:           return "PAL";
    case GrainType::Miscellaneous:     return "Misc";
    case GrainType::Unknown:           return "UNKNOWN";
    default:                           return "UNKNOWN";
    }
}
#endif
