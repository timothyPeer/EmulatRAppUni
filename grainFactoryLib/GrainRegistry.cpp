// ============================================================================
// GrainRegistry.cpp - Static member initialization
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

#include "GrainRegistry.h"
#include <iostream>
#include <set>

namespace AlphaAXP {

// Static member initialization
std::map<GrainKey, GrainMetadata> GrainRegistry::grains_;

void GrainRegistry::registerGrain(
    uint8_t opcode,
    uint16_t function_code,
    const std::string& mnemonic,
    const std::string& qualifier,
    ArchVariant variant,
    const std::string& format,
    GrainFunction handler
) {
    GrainKey key{opcode, function_code, qualifier, variant, format};
    
    // Build full name
    std::string full_name = mnemonic;
    if (!qualifier.empty() && qualifier != " ") {
        full_name += "_" + qualifier;
    }
    
    GrainMetadata metadata{
        mnemonic,
        qualifier,
        full_name,
        format,
        variant,
        handler
    };
    
    // Insert into registry
    auto result = grains_.insert({key, metadata});
    
    if (!result.second) {
        std::cerr << "WARNING: Duplicate grain registration: "
                  << full_name << " (opcode=0x" << std::hex << (int)opcode
                  << ", fc=0x" << function_code << ")" << std::dec << std::endl;
    }
}

GrainMetadata* GrainRegistry::lookupGrain(
    uint8_t opcode,
    uint16_t function_code,
    ArchVariant variant,
    const std::string& format
) {
    // Note: This lookup doesn't include qualifier yet - that needs to be extracted
    // from the instruction word itself (for IEEE FP instructions)
    GrainKey key{opcode, function_code, "", variant, format};
    
    auto it = grains_.find(key);
    if (it != grains_.end()) {
        return &it->second;
    }
    
    // Fallback: try to find with COMMON variant
    if (variant != ArchVariant::COMMON) {
        key.variant = ArchVariant::COMMON;
        it = grains_.find(key);
        if (it != grains_.end()) {
            return &it->second;
        }
    }
    
    return nullptr;
}

ExecutionResult GrainRegistry::execute(const InstructionContext& ctx) {
    // Extract opcode and function code from instruction
    uint8_t opcode = (ctx.instruction >> 26) & 0x3F;
    uint16_t function_code = ctx.instruction & 0x7FF;  // Simplified extraction
    
    // Lookup grain with context
    GrainMetadata* grain = lookupGrain(
        opcode,
        function_code,
        ctx.variant,
        ""  // Format can be determined from instruction analysis
    );
    
    if (grain && grain->handler) {
        return grain->handler(ctx);
    }
    
    return ExecutionResult::UNIMPLEMENTED;
}

const std::map<GrainKey, GrainMetadata>& GrainRegistry::getAllGrains() {
    return grains_;
}

size_t GrainRegistry::getGrainCount() {
    return grains_.size();
}

size_t GrainRegistry::getOpcodeCount() {
    std::set<uint8_t> unique_opcodes;
    for (const auto& entry : grains_) {
        unique_opcodes.insert(entry.first.opcode);
    }
    return unique_opcodes.size();
}

} // namespace AlphaAXP
