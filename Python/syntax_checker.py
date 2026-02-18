#!/usr/bin/env python3
import os
import re
import sys
from pathlib import Path
from collections import defaultdict


class SyntaxChecker:
    def __init__(self, filepath):
        self.filepath = filepath
        self.errors = []
        self.warnings = []
        
    def check(self):
        try:
            with open(self.filepath, 'r', encoding='utf-8', errors='ignore') as f:
                self.lines = f.readlines()
        except Exception as e:
            self.errors.append((0, f"Could not read file: {e}"))
            return False
        
        self.check_missing_semicolons()
        self.check_brace_matching()
        self.check_include_guards()
        
        return len(self.errors) == 0
    
    def check_missing_semicolons(self):
        """Check for missing semicolons after class, struct, enum, namespace"""
        in_block = False
        block_type = None
        block_start = 0
        brace_count = 0
        
        for i, line in enumerate(self.lines):
            stripped = line.strip()
            line_num = i + 1
            
            # Skip comments and preprocessor directives
            if stripped.startswith('//') or stripped.startswith('#'):
                continue
            
            # Track class/struct/enum/union declarations
            if not in_block:
                match = re.match(r'^(class|struct|enum|union)\s+(\w+)', stripped)
                if match:
                    block_type = match.group(1)
                    block_start = line_num
                    in_block = True
                    brace_count = 0
            
            # Count braces
            if in_block:
                brace_count += stripped.count('{') - stripped.count('}')
                
                # Check if block ends
                if brace_count == 0 and '{' in ''.join(self.lines[block_start-1:i+1]):
                    # Block closed, check for semicolon
                    if re.match(r'^\}\s*$', stripped):
                        # Closing brace without semicolon
                        self.errors.append((line_num, 
                            f"Missing semicolon after {block_type} definition that started at line {block_start}",
                            stripped))
                    elif re.match(r'^\};', stripped):
                        # Correct - has semicolon
                        pass
                    
                    in_block = False
                    block_type = None
    
    def check_brace_matching(self):
        """Check for matching braces, parentheses, and brackets"""
        # Remove string literals and comments first
        code = self.remove_strings_and_comments()
        
        brace_stack = []
        paren_stack = []
        bracket_stack = []
        angle_stack = []
        
        for i, line in enumerate(code):
            line_num = i + 1
            
            for j, char in enumerate(line):
                if char == '{':
                    brace_stack.append((line_num, j))
                elif char == '}':
                    if not brace_stack:
                        self.errors.append((line_num, "Unmatched closing brace '}'", line.strip()))
                    else:
                        brace_stack.pop()
                
                elif char == '(':
                    paren_stack.append((line_num, j))
                elif char == ')':
                    if not paren_stack:
                        self.errors.append((line_num, "Unmatched closing parenthesis ')'", line.strip()))
                    else:
                        paren_stack.pop()
                
                elif char == '[':
                    bracket_stack.append((line_num, j))
                elif char == ']':
                    if not bracket_stack:
                        self.errors.append((line_num, "Unmatched closing bracket ']'", line.strip()))
                    else:
                        bracket_stack.pop()
        
        # Report unclosed braces
        if brace_stack:
            for line_num, col in brace_stack:
                self.errors.append((line_num, "Unclosed brace '{'", self.lines[line_num-1].strip()))
        
        if paren_stack:
            for line_num, col in paren_stack:
                self.errors.append((line_num, "Unclosed parenthesis '('", self.lines[line_num-1].strip()))
        
        if bracket_stack:
            for line_num, col in bracket_stack:
                self.errors.append((line_num, "Unclosed bracket '['", self.lines[line_num-1].strip()))
    
    def check_include_guards(self):
        """Check for proper include guard or #pragma once in header files"""
        if not self.filepath.suffix in ['.h', '.hpp']:
            return
        
        # Check first 10 lines for #pragma once or #ifndef
        has_pragma_once = False
        has_ifndef = False
        
        for line in self.lines[:10]:
            if '#pragma once' in line:
                has_pragma_once = True
                break
            if re.match(r'#ifndef\s+\w+', line.strip()):
                has_ifndef = True
                break
        
        if not has_pragma_once and not has_ifndef:
            self.warnings.append((1, "Missing include guard (#pragma once or #ifndef)", ""))
    
    def remove_strings_and_comments(self):
        """Remove string literals and comments from code for bracket matching"""
        cleaned = []
        in_multiline_comment = False
        
        for line in self.lines:
            cleaned_line = []
            i = 0
            while i < len(line):
                # Check for multiline comment start
                if not in_multiline_comment and i < len(line) - 1 and line[i:i+2] == '/*':
                    in_multiline_comment = True
                    i += 2
                    continue
                
                # Check for multiline comment end
                if in_multiline_comment and i < len(line) - 1 and line[i:i+2] == '*/':
                    in_multiline_comment = False
                    i += 2
                    continue
                
                # Skip if in multiline comment
                if in_multiline_comment:
                    i += 1
                    continue
                
                # Check for single line comment
                if i < len(line) - 1 and line[i:i+2] == '//':
                    break
                
                # Check for string literal
                if line[i] in ['"', "'"]:
                    quote = line[i]
                    i += 1
                    while i < len(line) and line[i] != quote:
                        if line[i] == '\\' and i + 1 < len(line):
                            i += 2
                        else:
                            i += 1
                    i += 1
                    continue
                
                cleaned_line.append(line[i])
                i += 1
            
            cleaned.append(''.join(cleaned_line))
        
        return cleaned


def find_cpp_files(root_path):
    """Recursively find all C++ header and source files"""
    cpp_files = []
    for ext in ['.h', '.hpp', '.cpp', '.cc', '.cxx']:
        for filepath in Path(root_path).rglob(f'*{ext}'):
            # Skip build directories
            if 'build' in filepath.parts or 'out' in filepath.parts:
                continue
            cpp_files.append(filepath)
    return cpp_files


def main():
    if len(sys.argv) > 1:
        root = Path(sys.argv[1])
    else:
        root = Path.cwd()
    
    if not root.exists():
        print(f"Error: Path {root} does not exist")
        sys.exit(1)
    
    print("=" * 80)
    print("C++ Syntax Checker")
    print("=" * 80)
    print(f"Scanning: {root}")
    print()
    
    # Find all C++ files
    print("Finding C++ files...")
    cpp_files = find_cpp_files(root)
    print(f"Found {len(cpp_files)} files to check")
    print()
    
    # Check each file
    print("=" * 80)
    print("Checking files...")
    print("=" * 80)
    print()
    
    total_errors = 0
    total_warnings = 0
    files_with_errors = []
    
    for filepath in cpp_files:
        checker = SyntaxChecker(filepath)
        checker.check()
        
        if checker.errors or checker.warnings:
            files_with_errors.append(filepath)
            
            print(f"{'*' * 80}")
            print(f"FILE: {filepath}")
            print(f"{'*' * 80}")
            
            if checker.errors:
                print()
                print("ERRORS:")
                for line_num, msg, context in checker.errors:
                    total_errors += 1
                    print(f"  Line {line_num:4d}: {msg}")
                    if context:
                        print(f"               {context}")
            
            if checker.warnings:
                print()
                print("WARNINGS:")
                for line_num, msg, context in checker.warnings:
                    total_warnings += 1
                    print(f"  Line {line_num:4d}: {msg}")
                    if context:
                        print(f"               {context}")
            
            print()
    
    # Summary
    print("=" * 80)
    print("SUMMARY")
    print("=" * 80)
    print(f"Files checked: {len(cpp_files)}")
    print(f"Files with issues: {len(files_with_errors)}")
    print(f"Total errors: {total_errors}")
    print(f"Total warnings: {total_warnings}")
    print()
    
    if total_errors > 0:
        print("!!! ERRORS FOUND !!!")
        print()
        print("Common fixes:")
        print("  1. Add semicolon after class/struct/enum definition:")
        print("     };  <- not just }")
        print()
        print("  2. Check for unmatched braces, parentheses, brackets")
        print()
        print("Files with errors:")
        for f in files_with_errors[:10]:  # Show first 10
            print(f"  - {f}")
        if len(files_with_errors) > 10:
            print(f"  ... and {len(files_with_errors) - 10} more")
    else:
        print("No syntax errors found!")
        if total_warnings > 0:
            print(f"(But {total_warnings} warning(s) found - review above)")
    
    print()
    
    sys.exit(0 if total_errors == 0 else 1)


if __name__ == "__main__":
    main()