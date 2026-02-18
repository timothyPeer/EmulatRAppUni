#!/usr/bin/env python3
"""
Convert original GrainMaster.tsv to corrected format with proper box inference.

This script:
1. Reads the original instruction data
2. Maps Format → Box (with mnemonic overrides)
3. Maps Format → GrainType
4. Generates corrected GrainMaster.tsv
5. Provides audit report
"""

import csv
import sys
from pathlib import Path
from collections import defaultdict

# ============================================================================
# Format → Box Mapping
# ============================================================================

FORMAT_TO_BOX = {
    # Integer operations
    'Opr': 'EBox',
    'opr': 'EBox',  # Handle lowercase
    'Vector': 'EBox',
    
    # Floating-point operations  
    'Floating-Point': 'FBox',
    
    # Memory operations
    'Mem': 'MBox',
    
    # Branch/Control operations
    'Branch': 'CBox',
    
    # PAL operations
    'PAL': 'PalBox',
    'PAL-Unprivileged': 'PalBox',
    'PAL-Tru64': 'PalBox',
    'PALCode': 'PalBox',
    'Pal': 'PalBox',
    'Reserved': 'PalBox',
}

# ============================================================================
# Mnemonic → Box Overrides (takes precedence over format)
# ============================================================================

MNEMONIC_TO_BOX_OVERRIDE = {
    # Memory barriers → MBox
    'MB': 'MBox',
    'WMB': 'MBox',
    'TRAPB': 'MBox',
    'EXCB': 'MBox',
    'FETCH': 'MBox',
    'FETCH_M': 'MBox',
    'RPCC': 'MBox',
    'RC': 'MBox',
    'ECB': 'MBox',
    'RS': 'MBox',
    'READ_UNQ': 'MBox',
    'WRITE_UNQ': 'MBox',
    'RDUNIQUE': 'MBox',
    'WRUNIQUE': 'MBox',
    
    # Branches (even with Mem-Barrier format) → CBox
    'JMP': 'CBox',
    'JSR': 'CBox',
    'RET': 'CBox',
    'JSR_COROUTINE': 'CBox',
    'BSR': 'CBox',
    'BR': 'CBox',
    
    # All conditional branches → CBox
    'BEQ': 'CBox',
    'BNE': 'CBox',
    'BLT': 'CBox',
    'BGE': 'CBox',
    'BLE': 'CBox',
    'BGT': 'CBox',
    'BLBC': 'CBox',
    'BLBS': 'CBox',
    'FBEQ': 'CBox',
    'FBNE': 'CBox',
    'FBLT': 'CBox',
    'FBGE': 'CBox',
    'FBLE': 'CBox',
    'FBGT': 'CBox',
}

# ============================================================================
# Format → GrainType Mapping
# ============================================================================

FORMAT_TO_TYPE = {
    'Opr': 'IntegerOperate',
    'opr': 'IntegerOperate',
    'Vector': 'IntegerOperate',
    'Floating-Point': 'FloatingPoint',
    'Mem': 'Memory',
    'Mem-Barrier': 'Memory',
    'Mem-Func-Code': 'Memory',
    'Branch': 'Branch',
    'PAL': 'PALcode',
    'PAL-Unprivileged': 'PALcode',
    'PAL-Tru64': 'PALcode',
    'PALCode': 'PALcode',
    'Pal': 'PALcode',
    'Reserved': 'Miscellaneous',
}

# ============================================================================
# Mnemonic → GrainType Overrides
# ============================================================================

MNEMONIC_TO_TYPE_OVERRIDE = {
    # Branches are always Branch type
    'JMP': 'Branch',
    'JSR': 'Branch',
    'RET': 'Branch',
    'JSR_COROUTINE': 'Branch',
    'BSR': 'Branch',
    'BR': 'Branch',
    'BEQ': 'Branch',
    'BNE': 'Branch',
    'BLT': 'Branch',
    'BGE': 'Branch',
    'BLE': 'Branch',
    'BGT': 'Branch',
    'BLBC': 'Branch',
    'BLBS': 'Branch',
    'FBEQ': 'Branch',
    'FBNE': 'Branch',
    'FBLT': 'Branch',
    'FBGE': 'Branch',
    'FBLE': 'Branch',
    'FBGT': 'Branch',
}

# ============================================================================
# Helper Functions
# ============================================================================

def infer_box(mnemonic, format_str):
    """Infer execution box from mnemonic and format."""
    # Check mnemonic override first
    if mnemonic in MNEMONIC_TO_BOX_OVERRIDE:
        return MNEMONIC_TO_BOX_OVERRIDE[mnemonic]
    
    # Check format mapping
    if format_str in FORMAT_TO_BOX:
        return FORMAT_TO_BOX[format_str]
    
    # Handle square-root (floating-point) instructions
    if 'square-root' in format_str.lower():
        return 'FBox'
    
    # Handle Mem-Barrier and Mem-Func-Code special cases
    if 'Mem-Barrier' in format_str or 'Mem-Func-Code' in format_str:
        # If it's a branch, it goes to CBox (handled by override above)
        # Otherwise it's memory barrier → MBox
        return 'MBox'
    
    # Default fallback
    print(f"Warning: Unknown format '{format_str}' for '{mnemonic}', defaulting to MBox")
    return 'MBox'

def infer_type(mnemonic, format_str):
    """Infer grain type from mnemonic and format."""
    # Check mnemonic override first
    if mnemonic in MNEMONIC_TO_TYPE_OVERRIDE:
        return MNEMONIC_TO_TYPE_OVERRIDE[mnemonic]
    
    # Check format mapping
    if format_str in FORMAT_TO_TYPE:
        return FORMAT_TO_TYPE[format_str]
    
    # Handle square-root (floating-point) instructions
    if 'square-root' in format_str.lower():
        return 'FloatingPoint'
    
    # Handle special cases
    if 'Mem-Barrier' in format_str or 'Mem-Func-Code' in format_str:
        return 'Memory'
    
    # Default fallback
    return 'Miscellaneous'

def make_description(mnemonic, qualifier):
    """Generate description from mnemonic and qualifier."""
    if qualifier and qualifier.strip() and qualifier.strip() != ' ':
        return f"{mnemonic} {qualifier.strip()}"
    return mnemonic

def normalize_function(func_str):
    """Normalize function code to 0x format."""
    if not func_str or func_str.strip() == '':
        return '0x0000'
    
    func_str = func_str.strip()
    
    # Already has 0x prefix
    if func_str.startswith('0x') or func_str.startswith('0X'):
        # Ensure 4 digits
        hex_part = func_str[2:].upper()
        return f"0x{hex_part.zfill(4)}"
    
    # Pure hex
    return f"0x{func_str.upper().zfill(4)}"

# ============================================================================
# Main Processing
# ============================================================================

def process_tsv(input_file, output_file):
    """Process the original TSV and generate corrected version."""
    
    grains = []
    stats = defaultdict(int)
    box_distribution = defaultdict(int)
    type_distribution = defaultdict(int)
    
    print("Reading original GrainMaster.tsv...")
    
    with open(input_file, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f, delimiter='\t')
        
        for row in reader:
            # Extract fields
            opcode_hex = row['OPCodeHex'].strip()
            mnemonic_qual = row['Mneumonic/Qualifier'].strip()
            function = row['FC - Formatted'].strip()
            format_str = row['Format'].strip()
            qualifier = row.get('qualifier', '').strip()
            
            # Skip empty rows
            if not opcode_hex or not mnemonic_qual:
                continue
            
            # Infer box and type
            box = infer_box(mnemonic_qual, format_str)
            grain_type = infer_type(mnemonic_qual, format_str)
            
            # Generate description
            description = make_description(mnemonic_qual, qualifier)
            
            # Normalize function code
            function = normalize_function(function)
            
            # Store grain
            grain = {
                'Opcode': opcode_hex,
                'Function': function,
                'Mnemonic': mnemonic_qual,
                'Description': description,
                'Type': grain_type,
                'Box': box,
                'OriginalFormat': format_str,  # For audit
            }
            
            grains.append(grain)
            
            # Statistics
            stats['total'] += 1
            box_distribution[box] += 1
            type_distribution[grain_type] += 1
    
    # Write output
    print(f"\nWriting corrected GrainMaster.tsv...")
    
    with open(output_file, 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f, delimiter='\t')
        
        # Write header
        writer.writerow(['Opcode', 'Function', 'Mnemonic', 'Description', 'Type', 'Box'])
        
        # Write grains
        for grain in grains:
            writer.writerow([
                grain['Opcode'],
                grain['Function'],
                grain['Mnemonic'],
                grain['Description'],
                grain['Type'],
                grain['Box'],
            ])
    
    # Print audit report
    print("\n" + "=" * 80)
    print("AUDIT REPORT")
    print("=" * 80)
    print(f"\nTotal Instructions: {stats['total']}")
    
    print(f"\nBox Distribution:")
    for box in sorted(box_distribution.keys()):
        print(f"  {box:15s}: {box_distribution[box]:3d} grains")
    
    print(f"\nGrain Type Distribution:")
    for grain_type in sorted(type_distribution.keys()):
        print(f"  {grain_type:20s}: {type_distribution[grain_type]:3d} grains")
    
    # Check for potential issues
    print(f"\nPotential Issues:")
    mem_barrier_count = sum(1 for g in grains if 'Mem-Barrier' in g['OriginalFormat'] or 'Mem-Func-Code' in g['OriginalFormat'])
    mbox_from_mem_barrier = sum(1 for g in grains if ('Mem-Barrier' in g['OriginalFormat'] or 'Mem-Func-Code' in g['OriginalFormat']) and g['Box'] == 'MBox')
    cbox_from_mem_barrier = sum(1 for g in grains if ('Mem-Barrier' in g['OriginalFormat'] or 'Mem-Func-Code' in g['OriginalFormat']) and g['Box'] == 'CBox')
    
    print(f"  Mem-Barrier/Mem-Func-Code format instructions: {mem_barrier_count}")
    print(f"    → MBox (barriers): {mbox_from_mem_barrier}")
    print(f"    → CBox (branches): {cbox_from_mem_barrier}")
    
    # List some examples
    print(f"\nSample Mem-Barrier → MBox (correct):")
    mbox_samples = [g for g in grains if ('Mem-Barrier' in g['OriginalFormat'] or 'Mem-Func-Code' in g['OriginalFormat']) and g['Box'] == 'MBox'][:5]
    for g in mbox_samples:
        print(f"  {g['Opcode']} {g['Function']}: {g['Mnemonic']:20s} → {g['Box']}")
    
    print(f"\nSample Mem-Barrier → CBox (branches, correct):")
    cbox_samples = [g for g in grains if ('Mem-Barrier' in g['OriginalFormat'] or 'Mem-Func-Code' in g['OriginalFormat']) and g['Box'] == 'CBox'][:5]
    for g in cbox_samples:
        print(f"  {g['Opcode']} {g['Function']}: {g['Mnemonic']:20s} → {g['Box']}")
    
    print("\n" + "=" * 80)
    print("✓ Complete!")
    print("=" * 80)
    
    return grains

# ============================================================================
# Entry Point
# ============================================================================

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python convert_grainmaster.py original_grainmaster.tsv [output.tsv]")
        sys.exit(1)
    
    input_file = Path(sys.argv[1])
    output_file = Path(sys.argv[2]) if len(sys.argv) > 2 else Path('GrainMaster_Corrected.tsv')
    
    if not input_file.exists():
        print(f"Error: {input_file} not found")
        sys.exit(1)
    
    grains = process_tsv(input_file, output_file)
    
    print(f"\n✓ Generated: {output_file}")
    print(f"✓ Total grains: {len(grains)}")
    print(f"\nReady to use with generate_all_grains.py!")