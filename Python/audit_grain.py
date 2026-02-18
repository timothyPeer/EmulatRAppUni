#!/usr/bin/env python3
"""
Grain Header Audit Script
Scans all grain header files and checks for:
1. GrainRegistrationCore.h include
2. REGISTER_GRAIN() macro usage
3. Required methods (opcode, functionCode, execute)
"""

import os
import re
from pathlib import Path
from typing import List, Tuple, Set

class GrainAudit:
    def __init__(self, grains_dir: str = "grainLib/grains"):
        self.grains_dir = grains_dir
        self.issues = []
        self.grain_files = []
        
    def find_grain_files(self) -> List[Path]:
        """Find all grain header files"""
        grains_path = Path(self.grains_dir)
        if not grains_path.exists():
            print(f" ERROR: Directory not found: {self.grains_dir}")
            return []
        
        # Find all .h files containing "Grain" in name
        grain_files = list(grains_path.glob("*Grain*.h"))
        self.grain_files = sorted(grain_files)
        return self.grain_files
    
    def check_file(self, filepath: Path) -> dict:
        """Check a single grain file for required elements"""
        result = {
            'file': filepath.name,
            'has_registration_include': False,
            'has_register_macro': False,
            'has_opcode_method': False,
            'has_functioncode_method': False,
            'has_execute_method': False,
            'register_count': 0,
            'issues': []
        }
        
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # Check for GrainRegistrationCore.h include
            if re.search(r'#include\s+["\<].*GrainRegistrationCore\.h["\>]', content):
                result['has_registration_include'] = True
            
            # Check for REGISTER_GRAIN macro
            register_matches = re.findall(r'REGISTER_GRAIN\s*\(', content)
            result['register_count'] = len(register_matches)
            result['has_register_macro'] = result['register_count'] > 0
            
            # Check for required methods
            if re.search(r'\bopcode\s*\(\s*\)\s*const\s+override', content):
                result['has_opcode_method'] = True
            
            if re.search(r'\bfunctionCode\s*\(\s*\)\s*const\s+override', content):
                result['has_functioncode_method'] = True
            
            if re.search(r'\bexecute\s*\([^)]*PipelineSlot', content):
                result['has_execute_method'] = True
            
            # Identify issues
            if not result['has_registration_include']:
                result['issues'].append('Missing GrainRegistrationCore.h include')
            
            if not result['has_register_macro']:
                result['issues'].append('Missing REGISTER_GRAIN() macro')
            
            if not result['has_opcode_method']:
                result['issues'].append('Missing opcode() method')
            
            if not result['has_functioncode_method']:
                result['issues'].append('Missing functionCode() method')
            
            if not result['has_execute_method']:
                result['issues'].append('Missing execute() method')
            
        except Exception as e:
            result['issues'].append(f'Error reading file: {e}')
        
        return result
    
    def audit_all(self) -> List[dict]:
        """Audit all grain files"""
        results = []
        
        print(f" Scanning grain files in: {self.grains_dir}\n")
        
        grain_files = self.find_grain_files()
        if not grain_files:
            return results
        
        print(f" Found {len(grain_files)} grain files\n")
        
        for filepath in grain_files:
            result = self.check_file(filepath)
            results.append(result)
        
        return results
    
    def print_report(self, results: List[dict]):
        """Print audit report"""
        print("=" * 80)
        print("GRAIN AUDIT REPORT")
        print("=" * 80)
        print()
        
        # Files with issues
        files_with_issues = [r for r in results if r['issues']]
        files_ok = [r for r in results if not r['issues']]
        
        if files_with_issues:
            print(f"  FILES WITH ISSUES ({len(files_with_issues)}):")
            print("-" * 80)
            for result in files_with_issues:
                print(f"\n? {result['file']}")
                for issue in result['issues']:
                    print(f"   - {issue}")
                print(f"   Registrations found: {result['register_count']}")
            print()
        
        if files_ok:
            print(f"FILES OK ({len(files_ok)}):")
            print("-" * 80)
            for result in files_ok:
                print(f" {result['file']} ({result['register_count']} registration(s))")
            print()
        
        # Summary statistics
        print("=" * 80)
        print("SUMMARY")
        print("=" * 80)
        print(f"Total files scanned:           {len(results)}")
        print(f"Files OK:                      {len(files_ok)}")
        print(f"Files with issues:             {len(files_with_issues)}")
        print()
        
        total_registrations = sum(r['register_count'] for r in results)
        print(f"Total REGISTER_GRAIN() calls:  {total_registrations}")
        print()
        
        # Specific issue counts
        missing_include = sum(1 for r in results if not r['has_registration_include'])
        missing_register = sum(1 for r in results if not r['has_register_macro'])
        missing_opcode = sum(1 for r in results if not r['has_opcode_method'])
        missing_func = sum(1 for r in results if not r['has_functioncode_method'])
        missing_exec = sum(1 for r in results if not r['has_execute_method'])
        
        if any([missing_include, missing_register, missing_opcode, missing_func, missing_exec]):
            print("MISSING ELEMENTS:")
            if missing_include:
                print(f"  - GrainRegistrationCore.h include: {missing_include} files")
            if missing_register:
                print(f"  - REGISTER_GRAIN() macro:          {missing_register} files")
            if missing_opcode:
                print(f"  - opcode() method:                 {missing_opcode} files")
            if missing_func:
                print(f"  - functionCode() method:           {missing_func} files")
            if missing_exec:
                print(f"  - execute() method:                {missing_exec} files")
        
        print("=" * 80)
    
    def generate_fix_script(self, results: List[dict]):
        """Generate a script to add missing includes"""
        files_needing_include = [r for r in results if not r['has_registration_include']]
        
        if not files_needing_include:
            return
        
        print("\n" + "=" * 80)
        print("FIX SCRIPT - Add Missing Includes")
        print("=" * 80)
        print()
        print("# Run this to add missing GrainRegistrationCore.h includes")
        print()
        
        for result in files_needing_include:
            filepath = Path(self.grains_dir) / result['file']
            print(f"# Fix: {result['file']}")
            print(f"# Add after InstructionGrain.h include:")
            print(f'#   #include "../GrainRegistrationCore.h"')
            print()

def main():
    # Try common grain directory locations
    possible_dirs = [
        "grainLib/grains",
        "grainFactoryLib/grains",
        "../grainLib/grains",
        "../../grainLib/grains"
    ]
    
    grains_dir = None
    for dir_path in possible_dirs:
        if os.path.exists(dir_path):
            grains_dir = dir_path
            break
    
    if not grains_dir:
        print("? Could not find grains directory!")
        print("Please run from project root or specify path:")
        print("  python audit_grains.py /path/to/grainLib/grains")
        return
    
    auditor = GrainAudit(grains_dir)
    results = auditor.audit_all()
    
    if results:
        auditor.print_report(results)
        auditor.generate_fix_script(results)
    else:
        print("? No grain files found!")

if __name__ == "__main__":
    main()