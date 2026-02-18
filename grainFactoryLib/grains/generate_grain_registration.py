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
    'PalBox': 'palLib_EV6/PalBoxBase.h',
}

# ============================================================================
# Template Generator
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
    
    # Generate copyright with actual filename and description
    copyright = get_copyright_header(filename, f"{mnemonic} Instruction Grain")
    
    template = f"""{copyright}
//
//  Instruction: {mnemonic} - {description}
//  Opcode: {opcode}, Function: {function}
//  Execution Box: {box}
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
        : m_mnemonic("{mnemonic}")
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
    print("Alpha AXP Grain Code Generator")
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
    print(f"\nGenerating grain headers...")
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
    print(f"\nNext steps:")
    print(f"  1. Review generated grain headers")
    print(f"  2. Implement execute methods in box implementation files")
    print(f"  3. Add RegisterAllGrains.cpp to CMakeLists.txt")
    print(f"  4. Rebuild project")
    print(f"\n?? Hopefully no more regeneration needed! ??")

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