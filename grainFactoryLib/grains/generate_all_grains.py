#!/usr/bin/env python3
"""
generate_all_grains.py - Generate Alpha AXP instruction grain files from GrainMaster.tsv

Copyright (c) 2025 Timothy Peer / eNVy Systems, Inc.
Non-Commercial Use Only.
"""

import csv
import sys
from pathlib import Path
from datetime import datetime

# ============================================================================
# Configuration
# ============================================================================

def get_copyright_header(filename, description):
    """Generate copyright header with actual filename and description."""
    return f"""// ============================================================================
// {filename} - {description}
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
// ============================================================================"""

BASE_INCLUDE_PATH = "grainFactoryLib/grains"

# Box-specific directories
BOX_DIRECTORIES = {
    'EBox': 'generated/Integer',
    'FBox': 'generated/FloatingPoint',
    'MBox': 'generated/Memory',
    'CBox': 'generated/Branch',
    'PalBox': 'generated/PAL',
}

# Box-specific headers
BOX_HEADERS = {
    'EBox': 'EBoxLib/EBoxBase.h',
    'FBox': 'FBoxLib/FBoxBase.h',
    'MBox': 'MBoxLib_EV6/MBoxBase.h',
    'CBox': 'CBoxLib/CBoxBase.h',
    'PalBox': 'palBoxLib/PalBoxBase.h',
}

# ============================================================================
# Instruction Format Detection
# ============================================================================

def get_instruction_format(opcode_hex, mnemonic, box):
    """
    Determine instruction format based on opcode.
    Returns GrainFlags format constant.
    """
    # Convert hex string to integer
    opcode = int(opcode_hex, 16)
    
    # Operate format - Integer arithmetic/logical
    if opcode in [0x10, 0x11, 0x12, 0x13]:
        return "GF_OperateFormat"
    
    # Operate format - Floating point arithmetic
    elif opcode in [0x14, 0x15, 0x16, 0x17]:
        return "GF_OperateFormat"
    
    # Operate format - Byte manipulation
    elif opcode == 0x1C:
        return "GF_OperateFormat"
    
    # Memory format - Integer loads
    elif opcode in [0x0A, 0x0B, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F]:
        return "GF_MemoryFormat"
    
    # Memory format - Integer stores
    elif opcode in [0x0C, 0x0D, 0x0E, 0x0F]:
        return "GF_MemoryFormat"
    
    # Memory format - FP loads
    elif opcode in [0x20, 0x21, 0x22, 0x23]:
        return "GF_MemoryFormat"
    
    # Memory format - FP stores
    elif opcode in [0x24, 0x25, 0x26, 0x27]:
        return "GF_MemoryFormat"
    
    # Memory format - LDA/LDAH (address calculation)
    elif opcode in [0x08, 0x09]:
        return "GF_MemoryFormat"
    
    # Branch format - Unconditional
    elif opcode in [0x30, 0x34]:  # BR, BSR
        return "GF_BranchFormat"
    
    # Branch format - Conditional integer
    elif opcode in [0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F]:
        return "GF_BranchFormat"
    
    # Branch format - Conditional FP
    elif opcode in [0x31, 0x32, 0x33, 0x35, 0x36, 0x37]:
        return "GF_BranchFormat"
    
    # Jump format (treat as branch for dest register purposes)
    elif opcode == 0x1A:  # JMP, JSR, RET, JSR_COROUTINE
        return "GF_BranchFormat"
    
    # PAL format
    elif opcode == 0x00:
        return "GF_PALFormat"
    
    # Misc format
    elif opcode == 0x18:  # MISC (TRAPB, EXCB, MB, WMB, FETCH, etc.)
        return "GF_MemoryFormat"  # Most MISC treated as memory ops
    
    else:
        return "GF_None"

# ============================================================================
# Execution Flags Detection
# ============================================================================

def get_execution_flags(mnemonic, opcode_hex, box):
    """
    Determine execution characteristics.
    Returns list of GrainFlags execution flags.
    """
    flags = []
    
    # Convert to uppercase for comparison
    mnem = mnemonic.upper()
    
    # Dual-issue capable instructions (simple ALU ops)
    dual_issue_ops = [
        'ADDL', 'ADDQ', 'SUBL', 'SUBQ', 'S4ADDL', 'S4ADDQ', 'S8ADDL', 'S8ADDQ',
        'S4SUBL', 'S4SUBQ', 'S8SUBL', 'S8SUBQ',
        'AND', 'BIS', 'XOR', 'BIC', 'ORNOT', 'EQV',
        'CMPEQ', 'CMPLT', 'CMPLE', 'CMPULT', 'CMPULE',
        'SLL', 'SRL', 'SRA',
        'ZAP', 'ZAPNOT', 'EXTBL', 'EXTWL', 'EXTLL', 'EXTQL',
        'INSBL', 'INSWL', 'INSLL', 'INSQL',
        'MSKBL', 'MSKWL', 'MSKLL', 'MSKQL',
        'LDA', 'LDAH',  # Address computation
    ]
    
    if mnem in dual_issue_ops:
        flags.append('GF_CanDualIssue')
    
    # Instructions that need pipeline stalls
    stall_ops = [
        'TRAPB', 'EXCB', 'MB', 'WMB', 'RC', 'RS',
        'HW_REI', 'HW_RET',  # PAL ops
    ]
    
    if mnem in stall_ops:
        flags.append('GF_NeedsStall')
    
    return flags

# ============================================================================
# Latency Tables
# ============================================================================

def get_latency(mnemonic, opcode_hex, box):
    """
    Get instruction latency in cycles (EV6/21264 timings).
    """
    opcode = int(opcode_hex, 16)
    mnem = mnemonic.upper()
    
    # Integer multiply: 7 cycles
    if mnem in ['MULL', 'MULQ', 'UMULH']:
        if mnem == 'UMULH':
            return 14  # High multiply takes longer
        return 7
    
    # Integer loads: 3 cycles (to use, can forward earlier)
    elif opcode in [0x0A, 0x0B, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F]:
        return 3
    
    # FP loads: 4 cycles
    elif opcode in [0x20, 0x21, 0x22, 0x23]:
        return 4
    
    # Stores: 1 cycle (to store buffer)
    elif opcode in [0x0C, 0x0D, 0x0E, 0x0F, 0x24, 0x25, 0x26, 0x27]:
        return 1
    
    # FP arithmetic (most ops): 6 cycles
    elif opcode in [0x14, 0x15, 0x16, 0x17]:
        if mnem.startswith('DIV'):
            return 63  # FP divide
        elif mnem.startswith('SQRT'):
            return 70  # FP square root
        else:
            return 6  # FADD, FSUB, FMUL, FCMP
    
    # FP conversions: 10 cycles
    elif opcode == 0x14 or opcode == 0x1C:
        if mnem in ['CVTQS', 'CVTQT', 'CVTTS', 'CVTTQ']:
            return 10
        return 6 # fallback
    
    # Branches: 1 cycle (misprediction penalty handled elsewhere)
    elif box == 'CBox':
        return 1
    
    # Most other instructions: 1 cycle
    else:
        return 1

# ============================================================================
# Throughput
# ============================================================================

def get_throughput(mnemonic, opcode_hex):
    """
    Get throughput (instructions per cycle).
    Most Alpha instructions: 1 per cycle.
    Some complex ops may be lower.
    """
    mnem = mnemonic.upper()
    
    # Low throughput ops (less than 1 per cycle)
    if mnem in ['MULL', 'MULQ', 'UMULH', 'DIVS', 'DIVT', 'SQRTS', 'SQRTT']:
        return 1  # Simplified - still show as 1
    
    return 1

# ============================================================================
# Template Generator (Updated)
# ============================================================================

def sanitize_mnemonic(mnemonic):
    """Convert mnemonic to valid C++ identifier (replace special chars)."""
    return mnemonic.replace('/', '_').replace('-', '_')

def generate_header_guard(mnemonic):
    """Generate header guard name."""
    return f"{sanitize_mnemonic(mnemonic).upper()}_INSTRUCTIONGRAIN_H"

def get_grain_class_name(mnemonic):
    """Generate grain class name."""
    return f"{sanitize_mnemonic(mnemonic)}_InstructionGrain"

def get_grain_filename(mnemonic):
    """Generate grain filename."""
    return f"{sanitize_mnemonic(mnemonic)}_InstructionGrain.h"

def get_execute_method_name(box, mnemonic):
    """Generate execute method name for the box."""
    return f"execute{sanitize_mnemonic(mnemonic)}"

def get_box_member_name(box):
    """Get box member variable name (e.g., EBox -> m_eBox)."""
    return f"m_{box[0].lower()}{box[1:]}"

def generate_grain_flags(grain):
    """Generate complete flags expression for constructor."""
    opcode = grain['Opcode']
    mnemonic = grain['Mnemonic']
    box = grain['Box']
    
    # Get format flag
    format_flag = get_instruction_format(opcode, mnemonic, box)
    
    # Get execution flags
    exec_flags = get_execution_flags(mnemonic, opcode, box)
    
    # Combine flags
    all_flags = [format_flag] + exec_flags
    
    # Remove GF_None if other flags present
    if len(all_flags) > 1 and 'GF_None' in all_flags:
        all_flags.remove('GF_None')
    
    # Return combined expression
    if len(all_flags) == 0 or (len(all_flags) == 1 and all_flags[0] == 'GF_None'):
        return 'GF_None'
    else:
        return ' | '.join(all_flags)

def generate_grain_template(grain, box_dir):
    """Generate complete grain header file content."""
    
    opcode = grain['Opcode']
    function = grain['Function']
    mnemonic = grain['Mnemonic']
    description = grain['Description']
    box = grain['Box']
    grain_type = grain['Type']
    
    class_name = get_grain_class_name(mnemonic)
    header_guard = generate_header_guard(mnemonic)
    box_header = BOX_HEADERS[box]
    execute_method = get_execute_method_name(box, mnemonic)
    filename = get_grain_filename(mnemonic)
    box_member = get_box_member_name(box)
    
    # Generate flags, latency, throughput
    flags = generate_grain_flags(grain)
    latency = get_latency(mnemonic, opcode, box)
    throughput = get_throughput(mnemonic, opcode)
    
    # Generate copyright with actual filename and description
    copyright = get_copyright_header(filename, f"{mnemonic} Instruction Grain")
    
    template = f"""{copyright}
//
//  Instruction: {mnemonic} - {description}
//  Opcode: {opcode}, Function: {function}
//  Execution Box: {box}
//  Format: {get_instruction_format(opcode, mnemonic, box)}
//  Latency: {latency} cycles, Throughput: {throughput}/cycle
//
//  Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
//
// ============================================================================

#ifndef {header_guard}
#define {header_guard}

#include "coreLib/Axp_Attributes_core.h"
#include "{box_header}"
#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/InstructionGrainRegistry.h"
#include "grainFactoryLib/executionBoxDecoder_inl.h"
#include "machineLib/PipeLineSlot.h"

// ============================================================================
// {mnemonic} Instruction Grain
// ============================================================================

class {class_name} : public InstructionGrain
{{
public:
    {class_name}()
        : InstructionGrain(
            0,           // rawBits (updated per-fetch)
            {flags},     // flags
            {latency},   // latency (cycles)
            {throughput} // throughput (instructions/cycle)
          )
        , m_mnemonic("{mnemonic}")
        , m_opcode({opcode})
        , m_functionCode({function})
        , m_platform(GrainPlatform::Alpha)
    {{
    }}

    // ========================================================================
    // Virtual Method Implementations
    // ========================================================================
    
    AXP_HOT AXP_ALWAYS_INLINE
    void execute(PipelineSlot& slot) const noexcept override
    {{
        // Delegate to execution box via slot member
        slot.{box_member}->{execute_method}(slot);
    }}

    AXP_HOT AXP_ALWAYS_INLINE
    ExecutionBox executionBox() const noexcept
    {{
        return ExecutionBox::{box};
    }}

    AXP_HOT AXP_ALWAYS_INLINE
    GrainType grainType() const noexcept override
    {{
        return GrainType::{grain_type};
    }}

    // ========================================================================
    // Pure Virtual Accessor Implementations
    // ========================================================================
    
    AXP_HOT AXP_ALWAYS_INLINE
    QString mnemonic() const noexcept override
    {{
        return m_mnemonic;
    }}

    AXP_HOT AXP_ALWAYS_INLINE
    quint8 opcode() const noexcept override
    {{
        return m_opcode;
    }}

    AXP_HOT AXP_ALWAYS_INLINE
    quint16 functionCode() const noexcept override
    {{
        return m_functionCode;
    }}

    AXP_HOT AXP_ALWAYS_INLINE
    GrainPlatform platform() const noexcept override
    {{
        return m_platform;
    }}

private:
    QString m_mnemonic;
    quint8 m_opcode;
    quint16 m_functionCode;
    GrainPlatform m_platform;
}};

// ============================================================================
// Auto-registration
// ============================================================================

namespace {{
    static GrainAutoRegistrar<{class_name}> s_{sanitize_mnemonic(mnemonic).lower()}_registrar(
        {opcode}, {function}
    );
}}

#endif // {header_guard}
"""
    
    return template

# ============================================================================
# RegisterAllGrains.cpp Generator
# ============================================================================

def generate_registration_file(grains, output_dir):
    """Generate RegisterAllGrains.cpp that includes all grain headers."""
    
    # Group by box
    grains_by_box = {}
    for grain in grains:
        box = grain['Box']
        if box not in grains_by_box:
            grains_by_box[box] = []
        grains_by_box[box].append(grain)
    
    # Generate copyright
    copyright = get_copyright_header("RegisterAllGrains.cpp", "Auto-Generated Grain Registration")
    
    content = f"""{copyright}
//
//  Purpose: Include all instruction grain headers to force registration
//  Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
//
//  DO NOT EDIT MANUALLY - changes will be overwritten
//
// ============================================================================

#include "grainFactoryLib/InstructionGrainRegistry.h"
#include <QDebug>

// ============================================================================
// Include all grain headers (triggers auto-registration)
// Total grains: {len(grains)}
// ============================================================================

"""
    
    # Add includes organized by box
    for box in sorted(grains_by_box.keys()):
        box_dir = BOX_DIRECTORIES[box]
        box_grains = grains_by_box[box]
        
        content += f"\n// {box} Instructions ({len(box_grains)} grains)\n"
        
        for grain in sorted(box_grains, key=lambda g: g['Mnemonic']):
            mnemonic = grain['Mnemonic']
            filename = get_grain_filename(mnemonic)
            include = f"{BASE_INCLUDE_PATH}/{box_dir}/{filename}"
            content += f'#include "{include}"\n'
    
    # Add footer
    content += f"""
// ============================================================================
// Force linker to include this translation unit
// ============================================================================

namespace {{
    struct GrainRegistrationForcer {{
        GrainRegistrationForcer() {{
            auto& registry = InstructionGrainRegistry::instance();
            qDebug() << "RegisterAllGrains: Loaded" 
                     << registry.grainCount() << "instruction grains";
        }}
    }};
    
    // Static object instantiated before main()
    static GrainRegistrationForcer s_forcer;
}}

// End of auto-generated file
"""
    
    output_file = output_dir / "grainFactoryLib" / "RegisterAllGrains.cpp"
    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_text(content, encoding='utf-8')
    
    print(f"? Generated: {output_file}")
    return output_file

# ============================================================================
# Main Generator
# ============================================================================

def read_tsv(tsv_file):
    """Read GrainMaster.tsv and parse grain definitions."""
    grains = []
    
    with open(tsv_file, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f, delimiter='\t')
        
        for row in reader:
            # Skip comment lines
            if row['Opcode'].startswith('#'):
                continue
            
            # Parse opcode and function
            try:
                opcode = row['Opcode'].strip()
                function = row['Function'].strip()
                
                grains.append({
                    'Opcode': opcode,
                    'Function': function,
                    'Mnemonic': row['Mnemonic'].strip(),
                    'Description': row['Description'].strip(),
                    'Type': row['Type'].strip(),
                    'Box': row['Box'].strip(),
                })
            except (ValueError, KeyError) as e:
                print(f"Warning: Skipping invalid row: {row} ({e})")
                continue
    
    return grains

def generate_all_grains(tsv_file, output_dir):
    """Main entry point - generate all grain files."""
    
    print("=" * 80)
    print("Alpha AXP Grain Code Generator (with Base Constructor Initialization)")
    print("=" * 80)
    
    # Read TSV
    print(f"\nReading: {tsv_file}")
    grains = read_tsv(tsv_file)
    print(f"Loaded {len(grains)} grain definitions")
    
    # Create output directories
    output_dir = Path(output_dir)
    for box, box_dir in BOX_DIRECTORIES.items():
        full_dir = output_dir / BASE_INCLUDE_PATH / box_dir
        full_dir.mkdir(parents=True, exist_ok=True)
    
    # Generate individual grain headers
    print(f"\nGenerating grain headers with proper initialization...")
    generated_count = 0
    
    for grain in grains:
        box = grain['Box']
        box_dir = BOX_DIRECTORIES[box]
        mnemonic = grain['Mnemonic']
        
        # Generate content
        content = generate_grain_template(grain, box_dir)
        
        # Write file
        filename = get_grain_filename(mnemonic)
        output_file = output_dir / BASE_INCLUDE_PATH / box_dir / filename
        output_file.write_text(content, encoding='utf-8')
        
        generated_count += 1
        if generated_count % 50 == 0:
            print(f"  Generated {generated_count}/{len(grains)} grains...")
    
    print(f"? Generated {generated_count} grain headers")
    
    # Generate RegisterAllGrains.cpp
    print(f"\nGenerating registration file...")
    generate_registration_file(grains, output_dir)
    
    print("\n" + "=" * 80)
    print("GENERATION COMPLETE")
    print("=" * 80)
    print(f"\nGenerated files:")
    print(f"  - {generated_count} grain headers in {BASE_INCLUDE_PATH}/generated/")
    print(f"  - RegisterAllGrains.cpp")
    print(f"\nKey changes:")
    print(f"  ? Added InstructionGrain base constructor calls")
    print(f"  ? Automatic format detection (Operate/Memory/Branch/PAL)")
    print(f"  ? Execution flags (CanDualIssue, NeedsStall)")
    print(f"  ? EV6/21264 latency timings")
    print(f"  ? Throughput specifications")
    print(f"\nNext steps:")
    print(f"  1. Add format flags to InstructionGrain.h enum")
    print(f"  2. Add isOperateFormat() helpers to InstructionGrain class")
    print(f"  3. Regenerate all grains with this script")
    print(f"  4. Update destRegister() to use grain->isOperateFormat()")
    print(f"  5. Rebuild project")
    print(f"\n?? All grains will have proper initialization! ??")

# ============================================================================
# Entry Point
# ============================================================================

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python generate_all_grains.py GrainMaster.tsv [output_dir]")
        sys.exit(1)
    
    tsv_file = Path(sys.argv[1])
    output_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else Path(".")
    
    # Validate TSV file exists and is a file
    if not tsv_file.exists():
        print(f"Error: TSV file not found: {tsv_file}")
        sys.exit(1)
    
    if not tsv_file.is_file():
        print(f"Error: {tsv_file} is not a file")
        sys.exit(1)
    
    # Validate output directory exists and is a directory
    if not output_dir.exists():
        print(f"Error: Output directory not found: {output_dir}")
        sys.exit(1)
    
    if not output_dir.is_dir():
        print(f"Error: {output_dir} is not a directory")
        sys.exit(1)
    
    generate_all_grains(str(tsv_file), str(output_dir))