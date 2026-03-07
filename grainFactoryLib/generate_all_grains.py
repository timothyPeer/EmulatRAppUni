#!/usr/bin/env python3
"""
generate_all_grains.py - Generate Alpha AXP instruction grain files from GrainMaster.tsv
with integrated CPU trace hooks (DEC ASM style).
Copyright (c) 2025, 2026 Timothy Peer / eNVy Systems, Inc.
Non-Commercial Use Only.

Usage:
    python generate_all_grains.py GrainMaster.tsv [output_dir]

    output_dir defaults to ./grains (relative to script location)
"""
import csv
import sys
from pathlib import Path
from datetime import datetime

# ============================================================================
# Configuration
# ============================================================================
ENABLE_CPU_TRACE = True   # Set True to add CpuTrace hooks to all grains
BASE_INCLUDE_PATH = "grainFactoryLib/grains"

# Box-specific output subdirectories
BOX_DIRECTORIES = {
    'EBox':   'generated/Integer',
    'FBox':   'generated/FloatingPoint',
    'MBox':   'generated/Memory',
    'CBox':   'generated/Branch',
    'PalBox': 'generated/PAL',
}

# Box-specific base class headers
BOX_HEADERS = {
    'EBox':   'EBoxLib/EBoxBase.h',
    'FBox':   'FBoxLib/FBoxBase.h',
    'MBox':   'MBoxLib_EV6/MBoxBase.h',
    'CBox':   'CBoxLib/CBoxBase.h',
    'PalBox': 'palBoxLib/PalBoxBase.h',
}

# ============================================================================
# Copyright Header
# ============================================================================
def get_copyright_header(filename, description):
    return f"""// ============================================================================
// {filename} - {description}
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025, 2026 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================"""

# ============================================================================
# Name / identifier helpers
# ============================================================================
def sanitize_mnemonic(mnemonic):
    """Make mnemonic safe for use in C++ identifiers and filenames."""
    return mnemonic.replace('/', '_').replace('-', '_').replace('.', '_')

def generate_header_guard(mnemonic):
    return f"{sanitize_mnemonic(mnemonic).upper()}_INSTRUCTIONGRAIN_H"

def get_grain_class_name(mnemonic):
    return f"{sanitize_mnemonic(mnemonic)}_InstructionGrain"

def get_grain_filename(mnemonic):
    return f"{sanitize_mnemonic(mnemonic)}_InstructionGrain.h"

def get_execute_method_name(mnemonic):
    return f"execute{sanitize_mnemonic(mnemonic)}"

def get_box_member(box):
    """Return the PipelineSlot member name for the given box."""
    members = {
        'EBox':   'm_eBox',
        'FBox':   'm_fBox',
        'MBox':   'm_mBox',
        'CBox':   'm_cBox',
        'PalBox': 'm_palBox',
    }
    return members.get(box, f"m_{box[0].lower()}{box[1:]}")

# ============================================================================
# Instruction format flags
# Matches enum GrainFlags : quint8 in InstructionGrain.h:
#   GF_None          = 0
#   GF_OperateFormat = 1 << 0   integer and FP operate (Rc = Ra op Rb)
#   GF_MemoryFormat  = 1 << 1   memory load/store
#   GF_BranchFormat  = 1 << 2   branch and jump
#   GF_PALFormat     = 1 << 3   PAL / privileged instructions
#   GF_CanDualIssue  = 1 << 4   eligible for dual issue pairing
#   GF_NeedsStall    = 1 << 5   serializing, cannot pair
# ============================================================================
def get_instruction_format(opcode_hex, mnemonic, box):
    """Return the primary GrainFlags format for this instruction."""
    try:
        op = int(opcode_hex, 16)
    except (ValueError, TypeError):
        return 'GF_None'

    mne = mnemonic.upper()

    # PAL box — all privileged / HW instructions
    if box == 'PalBox':
        return 'GF_PALFormat'

    # Memory format: LDx/STx opcodes 0x08-0x0F, 0x20-0x2F
    if op in range(0x08, 0x10) or op in range(0x20, 0x30):
        return 'GF_MemoryFormat'

    # Branch / jump format: 0x1A, 0x30-0x3F
    if op == 0x1A or op in range(0x30, 0x40):
        return 'GF_BranchFormat'

    # Integer and FP operate: 0x10-0x17
    if op in range(0x10, 0x18):
        return 'GF_OperateFormat'

    # Miscellaneous operate (MISC opcode 0x18)
    if op == 0x18:
        return 'GF_OperateFormat'

    return 'GF_None'


def get_execution_flags(mnemonic, opcode_hex, box):
    """
    Return list of additional GrainFlags for scheduling hints.
    Only uses flags that exist in the enum.
    """
    flags = []
    mne = mnemonic.upper()

    try:
        op = int(opcode_hex, 16)
    except (ValueError, TypeError):
        op = 0

    # Serializing instructions — cannot pair, require pipeline drain
    stall_mnemonics = {
        'MB', 'WMB', 'TRAPB', 'EXCB', 'FETCH', 'FETCH_M',
        'CALL_PAL', 'HW_REI', 'HW_MTPR', 'HW_MFPR',
        'RPCC', 'RC', 'RS',
    }
    if mne in stall_mnemonics:
        flags.append('GF_NeedsStall')
        return flags  # stall implies no dual issue

    # Dual-issue eligible — simple integer operates and most branches
    dual_issue_boxes = {'EBox', 'CBox'}
    no_dual = {
        'MULL', 'MULQ', 'MULL_V', 'MULQ_V', 'UMULH',
        'DIVF', 'DIVG', 'DIVS', 'DIVT',
        'SQRTF', 'SQRTG', 'SQRTS', 'SQRTT',
    }
    if box in dual_issue_boxes and mne not in no_dual:
        flags.append('GF_CanDualIssue')

    return flags if flags else ['GF_None']

# ============================================================================
# Latency
# ============================================================================
def get_latency(mnemonic, opcode_hex, box):
    """
    Return the execution latency in cycles for this instruction.
    Based on EV6 microarchitecture latencies.
    """
    mne = mnemonic.upper()

    try:
        op = int(opcode_hex, 16)
    except (ValueError, TypeError):
        op = 0

    # FP divide — very high latency
    if mne in ('DIVF', 'DIVG', 'DIVS', 'DIVT'):
        return 15

    # FP sqrt
    if mne in ('SQRTF', 'SQRTG', 'SQRTS', 'SQRTT'):
        return 12

    # FP multiply
    if mne in ('MULF', 'MULG', 'MULS', 'MULT', 'FMUL'):
        return 4

    # Integer multiply
    if mne in ('MULL', 'MULQ', 'MULL_V', 'MULQ_V', 'UMULH'):
        return 7

    # Memory loads
    if mne in ('LDL', 'LDQ', 'LDB', 'LDW', 'LDL_L', 'LDQ_L', 'LDQ_U',
               'LDF', 'LDG', 'LDS', 'LDT'):
        return 3

    # FP general operate
    if box == 'FBox':
        return 4

    # Everything else — 1 cycle
    return 1

# ============================================================================
# Throughput
# ============================================================================
def get_throughput(mnemonic, opcode_hex):
    """
    Return the throughput (instructions per cycle) for this instruction.
    """
    mne = mnemonic.upper()

    # Divides and sqrt — not pipelined
    if mne in ('DIVF', 'DIVG', 'DIVS', 'DIVT',
               'SQRTF', 'SQRTG', 'SQRTS', 'SQRTT'):
        return 1

    return 1  # EV6 issue rate is 1 per pipe per cycle

# ============================================================================
# Grain flags string
# ============================================================================
def generate_grain_flags(grain):
    opcode   = grain['Opcode']
    mnemonic = grain['Mnemonic']
    box      = grain['Box']

    fmt   = get_instruction_format(opcode, mnemonic, box)
    extra = get_execution_flags(mnemonic, opcode, box)

    # Build combined list, removing GF_None placeholders
    combined = [fmt] + [f for f in extra if f != 'GF_None']
    real = [f for f in combined if f != 'GF_None']

    if not real:
        return 'GF_None'

    # Deduplicate preserving order
    seen = set()
    deduped = []
    for f in real:
        if f not in seen:
            seen.add(f)
            deduped.append(f)

    return ' | '.join(deduped)

# ============================================================================
# Grain template generator
# ============================================================================
def generate_grain_template(grain):
    opcode      = grain['Opcode']
    function    = grain['Function']
    mnemonic    = grain['Mnemonic']
    description = grain['Description']
    box         = grain['Box']
    grain_type  = grain['Type']

    class_name     = get_grain_class_name(mnemonic)
    header_guard   = generate_header_guard(mnemonic)
    filename       = get_grain_filename(mnemonic)
    box_header     = BOX_HEADERS[box]
    execute_method = get_execute_method_name(mnemonic)
    box_member     = get_box_member(box)
    flags          = generate_grain_flags(grain)
    latency        = get_latency(mnemonic, opcode, box)
    throughput     = get_throughput(mnemonic, opcode)
    fmt            = get_instruction_format(opcode, mnemonic, box)
    registrar_name = sanitize_mnemonic(mnemonic).lower()
    copyright      = get_copyright_header(filename, f"{mnemonic} - {description}")
    timestamp      = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    if ENABLE_CPU_TRACE:
        execute_body = f"""        // Delegate to execution box via slot member

        slot.{box_member}->{execute_method}(slot);
#ifdef AXP_EXEC_TRACE
        {{
            QString operands = slot.getOperandsString();
            QString result   = slot.getResultString();
            CpuTrace::instruction(
                slot.cycle,
                slot.di.pc,
                slot.di.rawBits(),
                "{mnemonic}",
                operands,
                result
            );
        }}
#endif // AXP_EXEC_TRACE"""
    else:
        execute_body = f"""        // Delegate to execution box via slot member
        slot.{box_member}->{execute_method}(slot);"""

    content = f"""{copyright}
//
//  Instruction: {mnemonic} - {description}
//  Opcode: {opcode}, Function: {function}
//  Execution Box: {box}
//  Format: {fmt}
//  Latency: {latency} cycles, Throughput: {throughput}/cycle
//
//  Generated: {timestamp}
//
// ============================================================================

#ifndef {header_guard}
#define {header_guard}

#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/cpuTrace.h"
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
{execute_body}
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
    QString     m_mnemonic;
    quint8      m_opcode;
    quint16     m_functionCode;
    GrainPlatform m_platform;
}};

// ============================================================================
// Auto-registration
// ============================================================================

namespace {{
    static GrainAutoRegistrar<{class_name}> s_{registrar_name}_registrar(
        {opcode}, {function}
    );
}}

#endif // {header_guard}
"""
    return content

# ============================================================================
# TSV reader
# ============================================================================
def read_tsv(tsv_file):
    """Read GrainMaster.tsv and return list of grain dicts."""
    grains = []
    path = Path(tsv_file)
    if not path.exists():
        print(f"ERROR: TSV file not found: {tsv_file}", file=sys.stderr)
        sys.exit(1)

    with open(path, newline='', encoding='utf-8') as f:
        reader = csv.DictReader(f, delimiter='\t')
        for row in reader:
            # Skip blank rows
            if not row.get('Mnemonic', '').strip():
                continue
            # Validate required columns
            for col in ('Opcode', 'Function', 'Mnemonic', 'Description', 'Type', 'Box'):
                if col not in row:
                    print(f"WARNING: Missing column '{col}' in row: {row}", file=sys.stderr)
                    continue
            # Skip unknown boxes
            if row['Box'] not in BOX_DIRECTORIES:
                print(f"WARNING: Unknown box '{row['Box']}' for {row['Mnemonic']} -- skipping",
                      file=sys.stderr)
                continue
            grains.append(row)

    return grains

# ============================================================================
# Registration file generator
# ============================================================================
def generate_registration_file(grains, output_dir):
    """Generate RegisterAllGrains.cpp that #includes every grain header."""
    lines = []
    lines.append(get_copyright_header('RegisterAllGrains.cpp',
                                       'Auto-generated grain registration'))
    lines.append('')
    lines.append('// Auto-generated — do not edit manually.')
    lines.append(f'// Generated: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}')
    lines.append('')
    lines.append('#include "grainFactoryLib/InstructionGrainRegistry.h"')
    lines.append('')
    lines.append('// ============================================================================')
    lines.append('// Grain header includes — triggers static auto-registration')
    lines.append('// ============================================================================')
    lines.append('')

    for grain in grains:
        mnemonic = grain['Mnemonic']
        box      = grain['Box']
        box_dir  = BOX_DIRECTORIES[box]
        filename = get_grain_filename(mnemonic)
        lines.append(f'#include "{BASE_INCLUDE_PATH}/{box_dir}/{filename}"')

    lines.append('')
    lines.append('// ============================================================================')
    lines.append('// RegisterAllGrains — called once at startup')
    lines.append('// ============================================================================')
    lines.append('')
    lines.append('void RegisterAllGrains()')
    lines.append('{')
    lines.append('    // Registration is automatic via static GrainAutoRegistrar<T> instances.')
    lines.append('    // This function exists as an explicit call site to force TU instantiation.')
    lines.append('}')
    lines.append('')

    reg_path = output_dir / 'RegisterAllGrains.cpp'
    reg_path.write_text('\n'.join(lines), encoding='utf-8')
    print(f"  -> Written: {reg_path}")

# ============================================================================
# Main generation loop
# ============================================================================
def generate_all_grains(tsv_file, output_dir):
    output_dir = Path(output_dir)
    grains = read_tsv(tsv_file)

    if not grains:
        print("ERROR: No grains found in TSV.", file=sys.stderr)
        sys.exit(1)

    print(f"Loaded {len(grains)} grains from {tsv_file}")

    # Create box output directories
    box_dirs = {}
    for box, subdir in BOX_DIRECTORIES.items():
        d = output_dir / subdir
        d.mkdir(parents=True, exist_ok=True)
        box_dirs[box] = d

    written  = 0
    skipped  = 0
    errors   = 0

    for grain in grains:
        mnemonic = grain['Mnemonic']
        box      = grain['Box']
        filename = get_grain_filename(mnemonic)
        dest     = box_dirs[box] / filename

        try:
            content = generate_grain_template(grain)
            dest.write_text(content, encoding='utf-8')
            written += 1
        except Exception as e:
            print(f"ERROR generating {mnemonic}: {e}", file=sys.stderr)
            errors += 1

    # Generate registration file at output root
    try:
        generate_registration_file(grains, output_dir)
    except Exception as e:
        print(f"ERROR generating RegisterAllGrains.cpp: {e}", file=sys.stderr)
        errors += 1

    print(f"\nDone.")
    print(f"  Written : {written}")
    print(f"  Skipped : {skipped}")
    print(f"  Errors  : {errors}")
    print(f"  Output  : {output_dir.resolve()}")

# ============================================================================
# Entry point
# ============================================================================
if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} GrainMaster.tsv [output_dir]")
        print(f"  output_dir defaults to ./grains")
        sys.exit(1)

    tsv_file   = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else 'grains'

    generate_all_grains(tsv_file, output_dir)
