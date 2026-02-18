#!/usr/bin/env python3
"""
Alpha AXP Grain Validator and Generator

Validates the GrainMaster.tsv file and generates grain header files
matching the InstructionGrain base class architecture.
"""

import sys
import csv
import re
from pathlib import Path
from collections import defaultdict
from typing import Dict, List, Tuple, Set

class GrainValidator:
    def __init__(self, tsv_path: str):
        self.tsv_path = Path(tsv_path)
        self.grains = []
        self.errors = []
        self.warnings = []
        
    def load_data(self) -> bool:
        """Load and parse the TSV file."""
        try:
            with open(self.tsv_path, 'r', encoding='utf-8') as f:
                reader = csv.DictReader(f, delimiter='\t')
                for row_num, row in enumerate(reader, start=2):
                    self.grains.append((row_num, row))
            print(f"✓ Loaded {len(self.grains)} grain definitions")
            return True
        except Exception as e:
            print(f"✗ Failed to load {self.tsv_path}: {e}")
            return False
    
    def validate_hex_format(self) -> None:
        """Validate hex format consistency."""
        print("\n[1] Checking hex format consistency...")
        
        hex_pattern = re.compile(r'^0x[0-9A-F]{2,4}$', re.IGNORECASE)
        
        for row_num, grain in self.grains:
            opcode_hex = grain['OPCodeHex'].strip()
            fc_formatted = grain['FC - Formatted'].strip()
            
            if opcode_hex and not hex_pattern.match(opcode_hex):
                self.errors.append(f"Row {row_num}: Invalid OPCodeHex format '{opcode_hex}'")
            
            if fc_formatted and not (hex_pattern.match(fc_formatted) or fc_formatted == ''):
                self.errors.append(f"Row {row_num}: Invalid FC format '{fc_formatted}'")
        
        if not self.errors:
            print("  ✓ All hex formats valid")
    
    def validate_opcode_consistency(self) -> None:
        """Validate OpCode-Dec matches Opcode hex conversion."""
        print("\n[2] Checking opcode decimal/hex consistency...")
        
        for row_num, grain in self.grains:
            opcode_dec = grain['OpCode-Dec'].strip()
            opcode_hex_str = grain['Opcode'].strip()
            
            if opcode_dec and opcode_hex_str:
                try:
                    dec_val = int(opcode_dec)
                    hex_val = int(opcode_hex_str, 16)
                    
                    if dec_val != hex_val:
                        self.errors.append(f"Row {row_num}: OpCode mismatch")
                except ValueError as e:
                    self.errors.append(f"Row {row_num}: Invalid number - {e}")
        
        if not any('OpCode mismatch' in e for e in self.errors):
            print("  ✓ All opcode conversions valid")
    
    def check_collisions(self) -> None:
        """Check for opcode/function code collisions."""
        print("\n[3] Checking for collisions...")
        
        collision_map: Dict[Tuple, List] = defaultdict(list)
        
        for row_num, grain in self.grains:
            opcode_hex = grain['OPCodeHex'].strip()
            fc_formatted = grain['FC - Formatted'].strip()
            mnemonic = grain['Mneumonic/Qualifier'].strip()
            qualifier = grain['qualifier'].strip()
            arch = grain['Architecture'].strip()
            format_type = grain['Format'].strip()
            
            if opcode_hex and fc_formatted:
                key = (opcode_hex, fc_formatted, qualifier, arch, format_type)
                collision_map[key].append((row_num, mnemonic))
        
        collisions_found = False
        for key, entries in collision_map.items():
            if len(entries) > 1:
                unique_mnemonics = set(m for _, m in entries)
                if len(unique_mnemonics) > 1:
                    collisions_found = True
                    self.errors.append(f"Collision: {', '.join(unique_mnemonics)}")
        
        if not collisions_found:
            print("  ✓ No collisions found")
        
        # Report variant statistics
        variant_map: Dict[Tuple, Set] = defaultdict(set)
        for row_num, grain in self.grains:
            opcode_hex = grain['OPCodeHex'].strip()
            fc_formatted = grain['FC - Formatted'].strip()
            arch = grain['Architecture'].strip()
            
            if opcode_hex and fc_formatted:
                key = (opcode_hex, fc_formatted)
                variant_map[key].add(arch)
        
        multi_variant_count = sum(1 for variants in variant_map.values() if len(variants) > 1)
        if multi_variant_count > 0:
            print(f"  ℹ️  {multi_variant_count} instructions with architecture variants")
    
    def check_duplicates(self) -> None:
        """Check for exact duplicate rows."""
        print("\n[4] Checking for duplicates...")
        
        seen = {}
        for row_num, grain in self.grains:
            sig = (
                grain['OPCodeHex'].strip(),
                grain['FC - Formatted'].strip(),
                grain['Mneumonic/Qualifier'].strip(),
                grain['Architecture'].strip()
            )
            
            if sig in seen:
                self.warnings.append(f"Row {row_num}: Duplicate of row {seen[sig]}")
            else:
                seen[sig] = row_num
        
        if not self.warnings:
            print("  ✓ No duplicates found")
    
    def check_required_fields(self) -> None:
        """Ensure all required fields are present."""
        print("\n[5] Checking required fields...")
        
        required = ['OPCodeHex', 'Mnemonic', 'Architecture', 'Format']
        
        for row_num, grain in self.grains:
            for field in required:
                if not grain.get(field, '').strip():
                    self.errors.append(f"Row {row_num}: Missing '{field}'")
        
        if not any('Missing' in e for e in self.errors):
            print("  ✓ All required fields present")
    
    def print_summary(self) -> bool:
        """Print validation summary."""
        print("\n" + "="*70)
        print("VALIDATION SUMMARY")
        print("="*70)
        
        if self.errors:
            print(f"\n❌ ERRORS: {len(self.errors)}")
            for err in self.errors[:5]:
                print(f"  • {err}")
            if len(self.errors) > 5:
                print(f"  ... and {len(self.errors) - 5} more")
        
        if self.warnings:
            print(f"\n⚠️  WARNINGS: {len(self.warnings)}")
            for warn in self.warnings[:3]:
                print(f"  • {warn}")
        
        if not self.errors and not self.warnings:
            print("\n✅ ALL CHECKS PASSED!")
            return True
        elif not self.errors:
            print("\n✅ NO ERRORS")
            return True
        else:
            print(f"\n❌ FAILED - {len(self.errors)} errors")
            return False
    
    def run(self) -> bool:
        """Run all validation checks."""
        if not self.load_data():
            return False
        
        self.validate_hex_format()
        self.validate_opcode_consistency()
        self.check_collisions()
        self.check_duplicates()
        self.check_required_fields()
        
        return self.print_summary()


class GrainGenerator:
    def __init__(self, tsv_path: str, output_base: str):
        self.tsv_path = Path(tsv_path)
        self.output_base = Path(output_base)
        self.grains = []
        
        # Category to execution box mapping
        self.box_map = {
            'Integer': 'm_eBox',
            'FloatingPoint': 'm_fBox',
            'Branch': 'm_cBox',
            'Memory': 'm_mBox',
            'Misc': 'm_fBox',
            'PAL': 'm_palBox',
            'Reserved': None  # No box assigned
        }
        
        # Category to box header include mapping
        self.header_map = {
            'Integer': '#include "EBoxLib/EBoxBase.h"',
            'FloatingPoint': '#include "FBoxLib/FBoxBase.h"',
            'Branch': '#include "CBoxLib/CBoxBase.h"',
            'Memory': '#include "MBoxLib_EV6/MBoxBase.h"',
            'Misc': '#include "FBoxLib/FBoxBase.h"',
            'PAL': '#include "PalBoxLib/PalBoxBase.h"',
            'Reserved': None  # No box assigned
        }
        
    def load_data(self) -> bool:
        """Load grain definitions."""
        try:
            with open(self.tsv_path, 'r', encoding='utf-8') as f:
                reader = csv.DictReader(f, delimiter='\t')
                self.grains = list(reader)
            print(f"✓ Loaded {len(self.grains)} grains for generation")
            return True
        except Exception as e:
            print(f"✗ Failed to load: {e}")
            return False
    
    def determine_category(self, grain: dict) -> str:
        """Determine subdirectory category."""
        format_type = grain['Format'].strip()
        
        if 'PAL' in format_type:
            return 'PAL'
        elif 'Floating-Point' in format_type or 'floating-point' in format_type:
            return 'FloatingPoint'
        elif format_type in ['Opr', 'opr', 'Vector']:
            return 'Integer'
        elif format_type in ['Mem', 'Mem-Barrier', 'Mem-Func-Code']:
            return 'Memory'
        elif 'Branch' in format_type:
            return 'Branch'
        elif format_type in ['Reserved', 'PALCode']:
            return 'Reserved'
        else:
            return 'Misc'
    
    def sanitize_filename(self, mnemonic: str) -> str:
        """Convert mnemonic to safe filename."""
        safe = mnemonic.replace('/', '_').replace('.', '_').replace('-', '_')
        return safe
    
    def generate_header(self, grain: dict) -> str:
        """Generate grain header content."""
        mnemonic_qual = grain['Mneumonic/Qualifier'].strip()
        opcode_hex = grain['OPCodeHex'].strip()
        opcode_dec = grain['OpCode-Dec'].strip()
        fc_hex = grain['FC - Formatted'].strip() or '0x0000'
        architecture = grain['Architecture'].strip()
        format_type = grain['Format'].strip()
        
        safe_name = self.sanitize_filename(mnemonic_qual)
        class_name = f"{safe_name}_InstructionGrain"
        
        # Generate header guard
        header_guard = f"{safe_name.upper()}_INSTRUCTIONGRAIN_H"
        
        # Map Format to GrainType
        graintype_map = {
            'Opr': 'GrainType::IntegerOperate',
            'opr': 'GrainType::IntegerOperate',
            'Vector': 'GrainType::Vector',
            'Floating-Point': 'GrainType::FloatOperate',
            'Mem': 'GrainType::IntegerMemory',
            'Mem-Barrier': 'GrainType::MemoryMB',
            'Mem-Func-Code': 'GrainType::MemoryMB',
            'Branch': 'GrainType::IntegerBranch',
            'PAL': 'GrainType::Pal',
            'PAL-Unprivileged': 'GrainType::Pal',
            'PAL-Tru64': 'GrainType::Pal',
            'PALCode': 'GrainType::Pal',
            'Reserved': 'GrainType::Unknown'
        }
        graintype_enum = graintype_map.get(format_type, 'GrainType::Unknown')
        
        # Map Architecture to GrainPlatform (all use Alpha platform)
        platform_enum = 'GrainPlatform::Alpha'
        
        # Determine execution box and generate execute body
        category = self.determine_category(grain)
        box_name = self.box_map.get(category)
        box_include = self.header_map.get(category)
        
        if box_name:
            execute_body = f"        slot.{box_name}->execute{safe_name}(slot);"
        else:
            execute_body = "        // Reserved - no execution box assigned"
        
        # Build box include line (only if box is assigned)
        box_include_line = f"{box_include}\n" if box_include else ""
        
        header = f"""#ifndef {header_guard}
#define {header_guard}
//
// Auto-generated grain: {mnemonic_qual}
// Opcode: {opcode_hex} ({opcode_dec} decimal)
// Function Code: {fc_hex}
// Format: {format_type}
// Architecture: {architecture}
//

#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/InstructionGrainRegistry.h"
#include "machineLib/PipeLineSlot.h"
{box_include_line}#include <QtGlobal>
#include <QString>

// ============================================================================
// {mnemonic_qual} - {format_type}
// ============================================================================

class {class_name} : public InstructionGrain
{{
public:
    {class_name}()
        : InstructionGrain(0, GF_None, 1, 1)  // rawBits=0, flags, latency=1, throughput=1
    {{
    }}
    
    ~{class_name}() override = default;
    
    quint8 opcode() const override
    {{
        return {opcode_hex};
    }}
    
    quint16 functionCode() const override
    {{
        return {fc_hex};
    }}
    
    GrainType grainType() const override
    {{
        return {graintype_enum};
    }}
    
    GrainPlatform platform() const override
    {{
        return {platform_enum};
    }}
    
    QString mnemonic() const override
    {{
        return "{mnemonic_qual}";
    }}
    
    void execute(PipelineSlot& slot) const noexcept override
    {{
{execute_body}
    }}
}};

// ============================================================================
// Registration
// ============================================================================

inline void register_{safe_name}_InstructionGrain()
{{
    {class_name}* grain = new {class_name}();
    InstructionGrainRegistry::instance().add(grain);
}}

#endif // {header_guard}
"""
        return header
    
    def generate_all(self, force_rebuild: bool = False) -> int:
        """Generate all grain headers."""
        if not self.load_data():
            return 0
        
        mode = "Rebuilding" if force_rebuild else "Generating"
        print(f"\n{mode} grains in {self.output_base}...")
        
        category_counts = defaultdict(int)
        generated = 0
        skipped = 0
        all_includes = []
        all_registrations = []
        
        for grain in self.grains:
            category = self.determine_category(grain)
            category_dir = self.output_base / category
            category_dir.mkdir(parents=True, exist_ok=True)
            
            mnemonic_qual = grain['Mneumonic/Qualifier'].strip()
            safe_name = self.sanitize_filename(mnemonic_qual)
            filename = f"{safe_name}_InstructionGrain.h"
            filepath = category_dir / filename
            
            if force_rebuild or not filepath.exists():
                header_content = self.generate_header(grain)
                
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(header_content)
                
                generated += 1
            else:
                skipped += 1
            
            include_path = f"#include \"generated/{category}/{filename}\""
            all_includes.append(include_path)
            all_registrations.append(f"    register_{safe_name}_InstructionGrain();")
            
            category_counts[category] += 1
        
        self._generate_register_all(all_includes, all_registrations)
        
        if force_rebuild:
            print(f"\n✓ Rebuilt {generated} grain headers:")
        else:
            print(f"\n✓ Generated {generated} new grains, skipped {skipped} existing:")
        
        for category, count in sorted(category_counts.items()):
            print(f"  • {category:20s}: {count:4d} grains")
        
        return generated
    
    def _generate_register_all(self, includes: List[str], registrations: List[str]) -> None:
        """Generate RegisterAllGrains.cpp."""
        
        content = f"""//
// Auto-generated grain registration
// Total grains: {len(registrations)}
//

#include "InstructionGrainRegistry.h"

// Include all grain headers
{chr(10).join(sorted(set(includes)))}

namespace {{

void registerAllGrains() {{
{chr(10).join(registrations)}
}}

}} // namespace

// Call from Phase2_InstructionSet():
//   registerAllGrains();
"""
        
        output_file = self.output_base.parent / "RegisterAllGrains.cpp"
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(content)
        
        print(f"\n✓ Generated RegisterAllGrains.cpp ({len(registrations)} grains)")


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Alpha AXP grain validator/generator')
    parser.add_argument('--tsv', default='GrainMaster.tsv', help='Grain table TSV')
    parser.add_argument('--validate-only', action='store_true', help='Validate only')
    parser.add_argument('--generate', action='store_true', help='Generate grains')
    parser.add_argument('--rebuild', action='store_true', 
                       help='Force rebuild all grains (ignore existing files)')
    parser.add_argument('--output', default='grainFactoryLib/grains/generated',
                       help='Output directory')
    
    args = parser.parse_args()
    
    print("="*70)
    print("ALPHA AXP GRAIN VALIDATOR")
    print("="*70)
    
    validator = GrainValidator(args.tsv)
    validation_passed = validator.run()
    
    if not validation_passed:
        print("\n⛔ Fix errors before generating")
        return 1
    
    if args.generate and not args.validate_only:
        print("\n" + "="*70)
        print("ALPHA AXP GRAIN GENERATOR")
        print("="*70)
        
        generator = GrainGenerator(args.tsv, args.output)
        count = generator.generate_all(force_rebuild=args.rebuild)
        
        if count > 0:
            mode = "Rebuilt" if args.rebuild else "Generated"
            print(f"\n✅ {mode} {count} grains")
            print(f"📁 Output: {args.output}")
            return 0
        else:
            print("\n❌ Generation failed")
            return 1
    
    return 0


if __name__ == '__main__':
    sys.exit(main())