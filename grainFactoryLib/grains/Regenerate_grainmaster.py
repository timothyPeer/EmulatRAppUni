#!/usr/bin/env python3
"""
Regenerate GrainMaster.tsv with proper Box inference from instruction Format.

This script reads the original instruction data and maps each instruction
to the correct execution box based on its format.
"""

import csv
import sys
from pathlib import Path

# ============================================================================
# Format → Box Mapping
# ============================================================================

FORMAT_TO_BOX = {
    # Integer operations
    'Opr': 'EBox',
    'Vector': 'EBox',

    # Floating-point operations
    'Floating-Point': 'FBox',
    'Floating-point square-root (single, UC rounding)': 'FBox',
    'Floating-point square-root (single→double, UC rounding)': 'FBox',
    'Floating-point square-root (double→quad, UC rounding)': 'FBox',
    'Floating-point square-root (quad→double, UC rounding)': 'FBox',
    'Floating-point square-root (single→double, UM rounding)': 'FBox',
    'Floating-point square-root (quad→double, UM rounding)': 'FBox',
    'Floating-point square-root (single, U rounding)': 'FBox',
    'Floating-point square-root (single→double, U rounding)': 'FBox',
    'Floating-point square-root (double→quad, U rounding)': 'FBox',
    'Floating-point square-root (quad→double, U rounding)': 'FBox',
    'Floating-point square-root (single→double, UD rounding)': 'FBox',
    'Floating-point square-root (quad→double, UD rounding)': 'FBox',
    'Floating-point square-root (single, SC rounding)': 'FBox',
    'Floating-point square-root (double→quad, SC rounding)': 'FBox',
    'Floating-point square-root (single, S rounding)': 'FBox',
    'Floating-point square-root (double→quad, S rounding)': 'FBox',
    'Floating-point square-root (single, SUC rounding)': 'FBox',
    'Floating-point square-root (single→double, SUC rounding)': 'FBox',
    'Floating-point square-root (double→quad, SUC rounding)': 'FBox',
    'Floating-point square-root (quad→double, SUC rounding)': 'FBox',
    'Floating-point square-root (single→double, SUM rounding)': 'FBox',
    'Floating-point square-root (quad→double, SUM rounding)': 'FBox',
    'Floating-point square-root (single, SU rounding)': 'FBox',
    'Floating-point square-root (single→double, SU rounding)': 'FBox',
    'Floating-point square-root (double→quad, SU rounding)': 'FBox',
    'Floating-point square-root (quad→double, SU rounding)': 'FBox',
    'Floating-point square-root (single→double, SUD rounding)': 'FBox',
    'Floating-point square-root (quad→double, SUD rounding)': 'FBox',
    'Floating-point square-root (single→double, SUIC rounding)': 'FBox',
    'Floating-point square-root (quad→double, SUIC rounding)': 'FBox',
    'Floating-point square-root (single→double, SUIM rounding)': 'FBox',
    'Floating-point square-root (quad→double, SUIM rounding)': 'FBox',
    'Floating-point square-root (single→double, SUI rounding)': 'FBox',
    'Floating-point square-root (quad→double, SUI rounding)': 'FBox',
    'Floating-point square-root (single→double, SUID rounding)': 'FBox',
    'Floating-point square-root (quad→double, SUID rounding)': 'FBox',

    # Memory operations
    'Mem': 'MBox',
    'Mem-Barrier': 'MBox',  # MB, WMB, READ_UNQ, WRITE_UNQ
    'Mem-Func-Code': 'MBox',  # TRAPB, EXCB, WMB, RPCC, RC, ECB, RS, RET

    # Branch/Control operations
    'Branch': 'CBox',

    # PAL operations
    'PAL': 'PalBox',
    'PAL-Unprivileged': 'PalBox',
    'PAL-Tru64': 'PalBox',
    'PALCode': 'PalBox',
    'Reserved': 'PalBox',
}

# ============================================================================
# Format → GrainType Mapping
# ============================================================================

FORMAT_TO_TYPE = {
    'Opr': 'IntegerOperate',
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
    'Reserved': 'Miscellaneous',
}

def infer_box(format_str):
    """Infer execution box from instruction format."""
    # Direct lookup
    if format_str in FORMAT_TO_BOX:
        return FORMAT_TO_BOX[format_str]

    # Fallback for floating-point square root variants
    if 'square-root' in format_str.lower():
        return 'FBox'

    # Default fallback
    print(f"Warning: Unknown format '{format_str}', defaulting to MBox")
    return 'MBox'

def infer_type(format_str):
    """Infer grain type from instruction format."""
    # Direct lookup
    if format_str in FORMAT_TO_TYPE:
        return FORMAT_TO_TYPE[format_str]

    # Fallback for floating-point square root variants
    if 'square-root' in format_str.lower() or 'Floating-Point' in format_str:
        return 'FloatingPoint'

    # Default fallback
    return 'Miscellaneous'

def parse_hex(value):
    """Parse hex value, handling various formats."""
    if not value or value.strip() == '':
        return '0x0000'

    value = value.strip()

    # Already has 0x prefix
    if value.startswith('0x') or value.startswith('0X'):
        return value

    # Pure hex
    return f'0x{value}'

def generate_grainmaster_tsv(output_path):
    """Generate GrainMaster.tsv with proper box inference."""

    # This would normally read from your existing data
    # For now, I'll create the structure

    with open(output_path, 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f, delimiter='\t')

        # Write header
        writer.writerow(['Opcode', 'Function', 'Mnemonic', 'Description', 'Type', 'Box'])

        # Example entries (you would populate this from your data)
        # Format: (opcode, function, mnemonic, description, format)

        instructions = [
            ('0x00', '0x0000', 'HALT', 'Halt processor', 'PAL'),
            ('0x10', '0x0000', 'ADDL', 'Add longword', 'Opr'),
            ('0x11', '0x0000', 'AND', 'Logical AND', 'Opr'),
            ('0x13', '0x0000', 'MULL', 'Multiply longword', 'Opr'),
            ('0x16', '0x0000', 'ADDS_C', 'Add single, chopped', 'Floating-Point'),
            ('0x18', '0x4000', 'MB', 'Memory barrier', 'Mem-Barrier'),
            ('0x18', '0x4400', 'WMB', 'Write memory barrier', 'Mem-Func-Code'),
            ('0x18', '0x0000', 'TRAPB', 'Trap barrier', 'Mem-Func-Code'),
            ('0x18', '0x0400', 'EXCB', 'Exception barrier', 'Mem-Func-Code'),
            ('0x1A', '0x0000', 'JMP', 'Jump', 'Mem-Barrier'),
            ('0x1A', '0x0001', 'JSR', 'Jump to subroutine', 'Mem-Barrier'),
            ('0x1A', '0x0002', 'RET', 'Return from subroutine', 'Mem-Func-Code'),
            ('0x08', '0x0000', 'LDA', 'Load address', 'Mem'),
            ('0x28', '0x0000', 'LDL', 'Load longword', 'Mem'),
            ('0x29', '0x0000', 'LDQ', 'Load quadword', 'Mem'),
            ('0x2C', '0x0000', 'STL', 'Store longword', 'Mem'),
            ('0x2D', '0x0000', 'STQ', 'Store quadword', 'Mem'),
            ('0x30', '0x0000', 'BR', 'Unconditional branch', 'Branch'),
            ('0x34', '0x0000', 'BSR', 'Branch to subroutine', 'Mem-Barrier'),
            ('0x39', '0x0000', 'BEQ', 'Branch if equal', 'Branch'),
            ('0x3D', '0x0000', 'BNE', 'Branch if not equal', 'Branch'),
        ]

        # Generate rows
        for opcode, function, mnemonic, description, format_str in instructions:
            box = infer_box(format_str)
            grain_type = infer_type(format_str)

            writer.writerow([opcode, function, mnemonic, description, grain_type, box])

    print(f"✓ Generated: {output_path}")
    print(f"\nBox inference examples:")
    print(f"  WMB (Mem-Func-Code) → MBox")
    print(f"  MB (Mem-Barrier) → MBox")
    print(f"  ADDL (Opr) → EBox")
    print(f"  ADDS (Floating-Point) → FBox")
    print(f"  BEQ (Branch) → CBox")
    print(f"  HALT (PAL) → PalBox")

if __name__ == '__main__':
    output_file = Path('GrainMaster.tsv')

    if len(sys.argv) > 1:
        output_file = Path(sys.argv[1])

    generate_grainmaster_tsv(output_file)

    print(f"\n📋 Format → Box Mapping:")
    for fmt, box in sorted(FORMAT_TO_BOX.items()):
        if len(fmt) < 20:  # Only show main formats
            print(f"  {fmt:20s} → {box}")
