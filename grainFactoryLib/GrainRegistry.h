// ============================================================================
// GrainRegistry.h - Alpha AXP Grain Registry - Context-Based Dispatch System
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

#pragma once
//
// Alpha AXP Grain Registry - Context-Based Dispatch System
//
// Supports multiple instruction variants per opcode/function code
// based on architecture variant and execution context.
//

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <memory>

namespace AlphaAXP {

// Architecture variants - different contexts for same opcode/fc
enum class ArchVariant : uint8_t {
    ALPHA,          // Alpha base architecture
    TRU64,          // Tru64 UNIX specific
    COMMON,         // Common across variants
    VAX,            // VAX compatibility mode
    IEEE,           // IEEE floating-point
    ALPHA_IEEE,     // Alpha/IEEE hybrid
    COMMON_VAX,     // Common VAX variant
    PAL             // Generic PAL
};

// Execution result codes
enum class ExecutionResult : uint8_t {
    SUCCESS,
    FAULT,
    EXCEPTION,
    UNIMPLEMENTED
};

// Instruction execution context
struct InstructionContext {
    uint32_t instruction;        // Raw instruction bits
    uint64_t pc;                 // Program counter
    ArchVariant variant;         // Architecture variant in effect
    bool privileged;             // Privilege level
    // Add more context as needed (registers, memory access, etc.)
};

// Grain implementation function type
using GrainFunction = std::function<ExecutionResult(const InstructionContext&)>;

// Complete grain key - includes all context needed for dispatch
struct GrainKey {
    uint8_t opcode;
    uint16_t function_code;
    std::string qualifier;       // IEEE FP rounding/trap qualifier (C, M, U, UC, etc.)
    ArchVariant variant;
    std::string format;          // Instruction format type
    
    // Comparison operator for map ordering
    bool operator<(const GrainKey& other) const {
        if (opcode != other.opcode) return opcode < other.opcode;
        if (function_code != other.function_code) return function_code < other.function_code;
        if (qualifier != other.qualifier) return qualifier < other.qualifier;
        if (variant != other.variant) return variant < other.variant;
        return format < other.format;
    }
};

// Grain metadata
struct GrainMetadata {
    std::string mnemonic;        // Base mnemonic (e.g., "ADDS")
    std::string qualifier;       // Qualifier (e.g., "C", "SU")
    std::string full_name;       // Complete name (e.g., "ADDS_C")
    std::string format;          // Instruction format
    ArchVariant variant;         // Architecture variant
    GrainFunction handler;       // Execution handler
};

// Grain Registry - manages all instruction grains
class GrainRegistry {
public:
    // Register a grain with full context
    static void registerGrain(
        uint8_t opcode,
        uint16_t function_code,
        const std::string& mnemonic,
        const std::string& qualifier,
        ArchVariant variant,
        const std::string& format,
        GrainFunction handler
    );
    
    // Lookup grain by full context
    static GrainMetadata* lookupGrain(
        uint8_t opcode,
        uint16_t function_code,
        ArchVariant variant,
        const std::string& format
    );
    
    // Execute instruction using context-based dispatch
    static ExecutionResult execute(const InstructionContext& ctx);
    
    // Get all registered grains (for debugging/inspection)
    static const std::map<GrainKey, GrainMetadata>& getAllGrains();
    
    // Statistics
    static size_t getGrainCount();
    static size_t getOpcodeCount();
    
private:
    static std::map<GrainKey, GrainMetadata> grains_;
};

} // namespace AlphaAXP
