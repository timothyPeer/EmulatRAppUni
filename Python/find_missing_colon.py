#!/usr/bin/env python3
import os
import re
import sys
from pathlib import Path


def find_cpp_files(root_path):
    """Recursively find all C++ header and source files"""
    cpp_files = []
    for ext in ['.h', '.hpp', '.cpp', '.cc', '.cxx']:
        for filepath in Path(root_path).rglob(f'*{ext}'):
            cpp_files.append(filepath)
    return cpp_files


def check_file(filepath, search_pattern):
    """Check if file contains the pattern and look for missing semicolons"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except:
        return None
    
    include_line = None
    for i, line in enumerate(lines):
        if re.search(search_pattern, line):
            include_line = i
            break
    
    if include_line is None:
        return None
    
    start = max(0, include_line - 15)
    context = lines[start:include_line + 1]
    
    problems = []
    for i, line in enumerate(context):
        stripped = line.strip()
        actual_line_num = start + i + 1
        
        if re.match(r'^\}\s*$', stripped):
            problems.append((actual_line_num, line.rstrip(), "MISSING SEMICOLON?"))
        elif re.search(r'^(class|struct)\s+\w+', stripped):
            problems.append((actual_line_num, line.rstrip(), "CHECK DEFINITION"))
    
    if problems:
        return (include_line + 1, context, problems)
    
    return None


def main():
    if len(sys.argv) > 1:
        root = Path(sys.argv[1])
    else:
        root = Path.cwd()
    
    if not root.exists():
        print(f"Error: Path {root} does not exist")
        sys.exit(1)
    
    print("=" * 70)
    print("Missing Semicolon Finder")
    print("=" * 70)
    print(f"Searching in: {root}")
    print()
    
    print("Searching for files that include cpuCore_core.h...")
    pattern = r'#include.*cpuCore_core\.h'
    
    all_files = find_cpp_files(root)
    matching_files = []
    
    for f in all_files:
        try:
            with open(f, 'r', encoding='utf-8', errors='ignore') as file:
                if re.search(pattern, file.read()):
                    matching_files.append(f)
        except:
            pass
    
    if not matching_files:
        print("No files found that include cpuCore_core.h directly.")
        print()
        print("Searching for files that include AlphaPipeline.h instead...")
        pattern = r'#include.*AlphaPipeline\.h'
        
        for f in all_files:
            try:
                with open(f, 'r', encoding='utf-8', errors='ignore') as file:
                    if re.search(pattern, file.read()):
                        matching_files.append(f)
            except:
                pass
    
    if not matching_files:
        print("ERROR: No relevant files found!")
        return
    
    print(f"Found {len(matching_files)} file(s):")
    for f in matching_files:
        print(f"  - {f}")
    print()
    
    print("=" * 70)
    print("Checking for syntax errors...")
    print("=" * 70)
    print()
    
    found_issues = False
    
    for filepath in matching_files:
        result = check_file(filepath, pattern)
        
        if result:
            include_line, context, problems = result
            found_issues = True
            
            print(f"*** SUSPICIOUS: {filepath} ***")
            print(f"Include statement at line {include_line}")
            print()
            
            start_line = include_line - len(context) + 1
            for i, line in enumerate(context):
                line_num = start_line + i
                print(f"  {line_num:4d} : {line.rstrip()}")
            
            print()
            print("PROBLEMS FOUND:")
            for prob_line, prob_text, prob_type in problems:
                print(f"  Line {prob_line}: {prob_type}")
                print(f"    {prob_text}")
            
            print()
            print("-" * 70)
            print()
    
    print("=" * 70)
    print("SUMMARY")
    print("=" * 70)
    
    if found_issues:
        print("!!! FOUND SUSPICIOUS PATTERNS !!!")
        print()
        print("Look for lines marked 'MISSING SEMICOLON?'")
        print("These are closing braces '}' without a semicolon.")
        print()
        print("FIX: Add a semicolon after the closing brace:")
        print("  }  <- WRONG")
        print("  }; <- CORRECT")
    else:
        print("No obvious syntax errors found in the scanned files.")
        print()
        print("Try a clean rebuild:")
        print("  1. Close Visual Studio")
        print("  2. Delete: out\\build\\x64-debug\\")
        print("  3. Reopen Visual Studio and rebuild")
    
    print()


if __name__ == "__main__":
    main()