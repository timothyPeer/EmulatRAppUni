#ifndef GRAIN_CONST_CORE_H
#define GRAIN_CONST_CORE_H

#include <qtypes.h>
constexpr quint32 OPCODE_LDBU = 0x0A;  // Load Byte Unsigned
constexpr quint32 OPCODE_LDWU = 0x0C;  // Load Word Unsigned
constexpr quint32 OPCODE_LDL = 0x28;   // Load Longword
constexpr quint32 OPCODE_STW = 0x0D;   // Store Word
constexpr quint32 OPCODE_STB = 0x0E;   // Store Byte
constexpr quint32 OPCODE_STL = 0x2C;   // Store Longword
constexpr quint32 OPCODE_BSR = 0x34;  // Branch to Subroutine
constexpr quint32 OPCODE_BR = 0x30;   // Branch
constexpr quint32 OPCODE_STL_C = 0x2E; // Store Longword Conditional
constexpr quint32 OPCODE_STQ_C = 0x2F; // Store Quadword Conditional
#endif // GRAIN_CONST_CORE_H
