#!/usr/bin/env python3
# ============================================================================
# license_guard_tool.py - Header Guard Audit & License Insertion Tool
# ============================================================================
# Project: ASA-EMulatR - Alpha AXP Architecture Emulator
# Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
#
# Usage:
#   python license_guard_tool.py <project_root> [options]
#
# Modes:
#   --audit          Report only (no changes). Default mode.
#   --fix-license    Insert/replace license headers in all .h/.cpp files.
#   --fix-guards     Insert missing header guards in .h files.
#   --fix-all        Fix both license headers and guards.
#   --backup         Create a zip backup before any changes.
#   --git-snapshot   Git commit current state before changes.
#   --dry-run        Show what would change without writing files.
#   --exclude=DIR    Exclude directory (repeatable). Default: build,third_party,.git
#
# Examples:
#   python license_guard_tool.py D:\VS2022\EmulatR --audit
#   python license_guard_tool.py D:\VS2022\EmulatR --git-snapshot --fix-all
#   python license_guard_tool.py D:\VS2022\EmulatR --backup --fix-license --dry-run
# ============================================================================

import os
import sys
import re
import argparse
import subprocess
import shutil
import zipfile
from datetime import datetime
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Optional, Set

# ============================================================================
# Configuration
# ============================================================================

STANDARD_LICENSE = """\
// ============================================================================
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

# Regex to detect an existing license/copyright block at the top of a file
# Matches consecutive // comment lines at file start (with optional blank lines)
LICENSE_BLOCK_PATTERN = re.compile(
    r'\A(\s*//[^\n]*\n)+',
    re.MULTILINE
)

# Keywords that identify a line as part of a license/copyright block
LICENSE_KEYWORDS = [
    'copyright', 'license', 'licensed', 'all rights reserved',
    'project:', 'contact:', 'documentation:', 'commercial',
    'architect', 'ai code generation', '============',
]

# Default directories to exclude from scanning
DEFAULT_EXCLUDES = {
    'build', 'Build', '.git', 'third_party', 'external', 'deps',
    'node_modules', '__pycache__', '.vs', 'Debug', 'Release',
    'x64', 'x86', 'CMakeFiles', 'cmake-build-debug', 'cmake-build-release',
    'build-EmulatRAppUni-Desktop_Qt_6_9_1_MinGW_64_bit-Debug',
}

FILE_EXTENSIONS = {'.h', '.cpp', '.hpp', '.cxx', '.cc', '.hxx', '.inl'}


# ============================================================================
# Data Structures
# ============================================================================

@dataclass
class FileReport:
    filepath: str
    relative_path: str
    extension: str
    has_license: bool = False
    has_guard: bool = False          # .h only
    guard_name: str = ''             # detected guard name
    expected_guard: str = ''         # what guard name should be
    guard_mismatch: bool = False     # guard exists but wrong name
    missing_endif: bool = False      # #ifndef/#define but no #endif
    license_stale: bool = False      # has license but doesn't match standard
    description: str = ''            # extracted or inferred description
    issues: List[str] = field(default_factory=list)


@dataclass
class AuditSummary:
    total_files: int = 0
    header_files: int = 0
    source_files: int = 0
    missing_license: int = 0
    stale_license: int = 0
    missing_guard: int = 0
    guard_mismatch: int = 0
    files_fixed: int = 0
    reports: List[FileReport] = field(default_factory=list)


# ============================================================================
# Utility Functions
# ============================================================================

def should_exclude(path: Path, excludes: Set[str]) -> bool:
    """Check if any component of the path matches an exclusion."""
    parts = path.parts
    for part in parts:
        if part in excludes:
            return True
    return False


def compute_guard_name(relative_path: str) -> str:
    """
    Compute the expected header guard from the file path.
    Example: EBoxLib/EBoxBase.h -> EBOXLIB_EBOXBASE_H
    """
    # Use just the filename for the guard (common convention)
    name = Path(relative_path).name
    guard = name.upper().replace('.', '_').replace('-', '_').replace(' ', '_')
    # Remove any non-alphanumeric/underscore characters
    guard = re.sub(r'[^A-Z0-9_]', '_', guard)
    return guard


def extract_description(content: str, filename: str) -> str:
    """
    Try to extract a description from existing comments, or infer from filename.
    """
    # Look for a description pattern like "// filename - Description"
    pattern = rf'//\s*{re.escape(filename)}\s*-\s*(.+)'
    match = re.search(pattern, content)
    if match:
        return match.group(1).strip()

    # Look for @brief
    match = re.search(r'@brief\s+(.+)', content)
    if match:
        return match.group(1).strip()

    # Look for first meaningful comment after any guard
    lines = content.split('\n')
    for line in lines[:30]:
        line = line.strip()
        if line.startswith('//') and not line.startswith('//=') and not line.startswith('//#'):
            text = line.lstrip('/ ').strip()
            if text and len(text) > 10 and 'ifndef' not in text.lower():
                return text

    # Infer from filename
    base = Path(filename).stem
    # Split CamelCase
    words = re.sub(r'([A-Z])', r' \1', base).strip()
    # Split underscores
    words = words.replace('_', ' ').strip()
    return words


def is_license_block_line(line: str) -> bool:
    """Check if a line is part of a license/copyright header block."""
    stripped = line.strip().lower()
    if not stripped.startswith('//'):
        return False
    content = stripped.lstrip('/ ').strip()
    if not content:  # empty comment line
        return True
    if content.startswith('====') or content.startswith('----'):
        return True
    for keyword in LICENSE_KEYWORDS:
        if keyword in content:
            return True
    return False


def find_license_block_end(lines: List[str]) -> int:
    """
    Find where the license/copyright block ends.
    Returns the index of the first line that is NOT part of the license block.
    """
    i = 0
    # Skip any leading blank lines
    while i < len(lines) and lines[i].strip() == '':
        i += 1

    if i >= len(lines):
        return 0

    # Must start with a comment
    if not lines[i].strip().startswith('//'):
        return 0

    # Walk through contiguous comment block
    block_start = i
    in_block = True
    consecutive_non_license = 0

    while i < len(lines) and in_block:
        stripped = lines[i].strip()

        if stripped == '':
            # Blank line - could be separator within block or end of block
            # Look ahead to see if more license-like comments follow
            j = i + 1
            while j < len(lines) and lines[j].strip() == '':
                j += 1
            if j < len(lines) and is_license_block_line(lines[j]):
                i = j
                continue
            else:
                break
        elif stripped.startswith('//'):
            if is_license_block_line(lines[i]):
                consecutive_non_license = 0
            else:
                consecutive_non_license += 1
                if consecutive_non_license >= 3:
                    # Probably not license anymore, back up
                    i -= consecutive_non_license
                    break
        else:
            break
        i += 1

    # If we only found a tiny block (< 3 lines), it's probably not a license
    if i - block_start < 3:
        return 0

    return i


def detect_header_guard(content: str) -> tuple:
    """
    Detect if a header guard (#ifndef/#define/#endif) is present.
    Returns (has_guard, guard_name, has_matching_endif).
    """
    lines = content.split('\n')

    ifndef_name = None
    define_name = None
    has_endif = False

    for line in lines:
        stripped = line.strip()

        if ifndef_name is None:
            match = re.match(r'#\s*ifndef\s+(\w+)', stripped)
            if match:
                ifndef_name = match.group(1)
                continue

        if ifndef_name and define_name is None:
            match = re.match(r'#\s*define\s+(\w+)', stripped)
            if match:
                define_name = match.group(1)
                continue

    if ifndef_name:
        # Check for matching #endif (should be near end of file)
        for line in reversed(lines[-20:]):
            if re.match(r'#\s*endif', line.strip()):
                has_endif = True
                break

    if ifndef_name and define_name and ifndef_name == define_name:
        return True, ifndef_name, has_endif
    elif ifndef_name:
        return True, ifndef_name, has_endif

    # Also check for #pragma once
    for line in lines[:20]:
        if re.match(r'#\s*pragma\s+once', line.strip()):
            return True, '#pragma once', True

    return False, '', False


def check_license_present(content: str) -> tuple:
    """
    Check if a standard license block is present.
    Returns (has_license, is_stale).
    """
    lower = content[:2000].lower()

    has_copyright = 'copyright' in lower
    has_license_word = 'license' in lower or 'licensed' in lower
    has_project = 'asa-emulatr' in lower or 'emulatr' in lower

    if has_copyright and has_project:
        # Check if it matches current standard
        has_envy = 'envy systems' in lower
        has_contact = 'peert@envysys.com' in lower or 'envysys.com' in lower
        if has_envy and has_contact:
            return True, False  # present and current
        return True, True  # present but stale

    if has_copyright or has_license_word:
        return True, True  # some license exists but doesn't match

    return False, False


# ============================================================================
# Core Operations
# ============================================================================

def audit_file(filepath: Path, root: Path) -> FileReport:
    """Audit a single file for license header and header guard."""
    relative = filepath.relative_to(root)
    report = FileReport(
        filepath=str(filepath),
        relative_path=str(relative),
        extension=filepath.suffix.lower(),
    )

    try:
        content = filepath.read_text(encoding='utf-8', errors='replace')
    except Exception as e:
        report.issues.append(f'READ ERROR: {e}')
        return report

    # Extract description for potential license insertion
    report.description = extract_description(content, filepath.name)

    # Check license
    has_license, is_stale = check_license_present(content)
    report.has_license = has_license
    report.license_stale = is_stale
    if not has_license:
        report.issues.append('MISSING LICENSE HEADER')
    elif is_stale:
        report.issues.append('STALE/NON-STANDARD LICENSE HEADER')

    # Check header guard (.h files only)
    if report.extension in {'.h', '.hpp', '.hxx', '.inl'}:
        has_guard, guard_name, has_endif = detect_header_guard(content)
        expected_guard = compute_guard_name(str(relative))
        report.has_guard = has_guard
        report.guard_name = guard_name
        report.expected_guard = expected_guard

        if not has_guard:
            report.issues.append('MISSING HEADER GUARD (#ifndef/#define/#endif)')
        else:
            if guard_name != '#pragma once' and guard_name != expected_guard:
                # Don't flag as mismatch - existing guards are often intentional
                # Just note it for informational purposes
                pass
            if not has_endif and guard_name != '#pragma once':
                report.missing_endif = True
                report.issues.append('MISSING #endif FOR HEADER GUARD')

    return report


def build_license_header(filename: str, description: str) -> str:
    """Build the standard license header for a file."""
    return STANDARD_LICENSE.format(
        filename=filename,
        description=description or filename.replace('.', ' ').replace('_', ' ')
    )


def fix_license(filepath: Path, report: FileReport, dry_run: bool = False) -> bool:
    """Insert or replace the license header in a file."""
    try:
        content = filepath.read_text(encoding='utf-8', errors='replace')
    except Exception:
        return False

    lines = content.split('\n')
    new_license = build_license_header(filepath.name, report.description)

    if report.has_license:
        # Replace existing license block
        end_idx = find_license_block_end(lines)
        if end_idx > 0:
            # Skip any blank lines after the old license
            while end_idx < len(lines) and lines[end_idx].strip() == '':
                end_idx += 1
            remaining = '\n'.join(lines[end_idx:])
            new_content = new_license + '\n\n' + remaining
        else:
            # Couldn't isolate old block, prepend
            new_content = new_license + '\n\n' + content
    else:
        # No license - prepend
        # But respect #ifndef guard if it's the very first thing
        first_code_line = 0
        while first_code_line < len(lines) and lines[first_code_line].strip() == '':
            first_code_line += 1

        if first_code_line < len(lines):
            first = lines[first_code_line].strip()
            if first.startswith('#ifndef') or first.startswith('#pragma once'):
                # Insert before the guard
                new_content = new_license + '\n\n' + content.lstrip('\n')
            else:
                new_content = new_license + '\n\n' + content.lstrip('\n')
        else:
            new_content = new_license + '\n\n' + content

    if dry_run:
        return True

    try:
        filepath.write_text(new_content, encoding='utf-8')
        return True
    except Exception as e:
        print(f'  ERROR writing {filepath}: {e}')
        return False


def fix_guard(filepath: Path, report: FileReport, dry_run: bool = False) -> bool:
    """Insert a missing header guard into a .h file."""
    if report.has_guard:
        return False  # Already has one

    try:
        content = filepath.read_text(encoding='utf-8', errors='replace')
    except Exception:
        return False

    guard_name = report.expected_guard
    guard_top = f'#ifndef {guard_name}\n#define {guard_name}\n'
    guard_bottom = f'\n#endif // {guard_name}\n'

    # Find where to insert the guard (after any license block)
    lines = content.split('\n')
    license_end = find_license_block_end(lines)

    if license_end > 0:
        header_part = '\n'.join(lines[:license_end])
        body_part = '\n'.join(lines[license_end:])
        new_content = header_part + '\n\n' + guard_top + '\n' + body_part.lstrip('\n') + guard_bottom
    else:
        new_content = guard_top + '\n' + content + guard_bottom

    if dry_run:
        return True

    try:
        filepath.write_text(new_content, encoding='utf-8')
        return True
    except Exception as e:
        print(f'  ERROR writing {filepath}: {e}')
        return False


# ============================================================================
# Git & Backup Operations
# ============================================================================

def git_snapshot(root: Path, message: str = None) -> bool:
    """Create a git commit of the current state as a safety snapshot."""
    if message is None:
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        message = f'[SNAPSHOT] Pre-license-tool safety commit - {timestamp}'

    try:
        # Check if we're in a git repo
        result = subprocess.run(
            ['git', 'rev-parse', '--is-inside-work-tree'],
            cwd=str(root), capture_output=True, text=True
        )
        if result.returncode != 0:
            print(f'WARNING: {root} is not a git repository.')
            print('  Run "git init" first, or use --backup for zip backup.')
            return False

        # Check for uncommitted changes
        result = subprocess.run(
            ['git', 'status', '--porcelain'],
            cwd=str(root), capture_output=True, text=True
        )
        if result.stdout.strip():
            # Stage everything
            subprocess.run(
                ['git', 'add', '-A'],
                cwd=str(root), capture_output=True, text=True
            )
            # Commit
            result = subprocess.run(
                ['git', 'commit', '-m', message],
                cwd=str(root), capture_output=True, text=True
            )
            if result.returncode == 0:
                # Get short hash
                hash_result = subprocess.run(
                    ['git', 'rev-parse', '--short', 'HEAD'],
                    cwd=str(root), capture_output=True, text=True
                )
                short_hash = hash_result.stdout.strip()
                print(f'  Git snapshot created: {short_hash} - "{message}"')
                return True
            else:
                print(f'  Git commit failed: {result.stderr}')
                return False
        else:
            print('  Working tree clean, no snapshot needed.')
            return True

    except FileNotFoundError:
        print('WARNING: git not found in PATH. Use --backup for zip backup.')
        return False


def create_backup(root: Path, backup_dir: Path = None) -> Optional[Path]:
    """Create a zip backup of all source files."""
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    project_name = root.name

    if backup_dir is None:
        backup_dir = root.parent

    backup_path = backup_dir / f'{project_name}_backup_{timestamp}.zip'

    print(f'  Creating backup: {backup_path}')
    file_count = 0

    try:
        with zipfile.ZipFile(backup_path, 'w', zipfile.ZIP_DEFLATED) as zf:
            for ext in FILE_EXTENSIONS:
                for filepath in root.rglob(f'*{ext}'):
                    if should_exclude(filepath.relative_to(root), DEFAULT_EXCLUDES):
                        continue
                    arcname = filepath.relative_to(root)
                    zf.write(filepath, arcname)
                    file_count += 1

        size_mb = backup_path.stat().st_size / (1024 * 1024)
        print(f'  Backup complete: {file_count} files, {size_mb:.1f} MB')
        return backup_path

    except Exception as e:
        print(f'  Backup FAILED: {e}')
        return None


# ============================================================================
# Reporting
# ============================================================================

def print_audit_report(summary: AuditSummary):
    """Print a formatted audit report."""
    sep = '=' * 78

    print(f'\n{sep}')
    print(f' LICENSE & HEADER GUARD AUDIT REPORT')
    print(f' Generated: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}')
    print(f'{sep}\n')

    # Summary
    print(f' Total files scanned:      {summary.total_files:>5}')
    print(f'   Header files (.h):      {summary.header_files:>5}')
    print(f'   Source files (.cpp):     {summary.source_files:>5}')
    print()
    print(f' Missing license header:   {summary.missing_license:>5}', end='')
    print('  ✓' if summary.missing_license == 0 else '  ✗ ACTION NEEDED')
    print(f' Stale/outdated license:   {summary.stale_license:>5}', end='')
    print('  ✓' if summary.stale_license == 0 else '  ⚠ REVIEW')
    print(f' Missing header guard:     {summary.missing_guard:>5}', end='')
    print('  ✓' if summary.missing_guard == 0 else '  ✗ ACTION NEEDED')
    print()

    # Detailed issues
    problem_files = [r for r in summary.reports if r.issues]
    if problem_files:
        print(f'{"-" * 78}')
        print(f' FILES WITH ISSUES ({len(problem_files)})')
        print(f'{"-" * 78}\n')

        # Group by issue type
        missing_license = [r for r in problem_files if not r.has_license]
        stale_license = [r for r in problem_files if r.license_stale]
        missing_guard = [r for r in problem_files if not r.has_guard and r.extension in {'.h', '.hpp'}]
        missing_endif = [r for r in problem_files if r.missing_endif]

        if missing_license:
            print(f' ── MISSING LICENSE HEADER ({len(missing_license)} files) ──')
            for r in sorted(missing_license, key=lambda x: x.relative_path):
                print(f'   {r.relative_path}')
            print()

        if stale_license:
            print(f' ── STALE/NON-STANDARD LICENSE ({len(stale_license)} files) ──')
            for r in sorted(stale_license, key=lambda x: x.relative_path):
                print(f'   {r.relative_path}')
            print()

        if missing_guard:
            print(f' ── MISSING HEADER GUARD ({len(missing_guard)} files) ──')
            for r in sorted(missing_guard, key=lambda x: x.relative_path):
                print(f'   {r.relative_path}')
                print(f'     Expected: #ifndef {r.expected_guard}')
            print()

        if missing_endif:
            print(f' ── MISSING #endif ({len(missing_endif)} files) ──')
            for r in sorted(missing_endif, key=lambda x: x.relative_path):
                print(f'   {r.relative_path} (guard: {r.guard_name})')
            print()
    else:
        print(' All files passed audit. ✓\n')

    print(f'{sep}')
    if summary.files_fixed > 0:
        print(f' Files modified: {summary.files_fixed}')
    print()


def write_csv_report(summary: AuditSummary, output_path: Path):
    """Write a machine-readable CSV report."""
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write('relative_path,extension,has_license,license_stale,has_guard,guard_name,expected_guard,issues\n')
        for r in sorted(summary.reports, key=lambda x: x.relative_path):
            issues = '; '.join(r.issues) if r.issues else ''
            f.write(f'"{r.relative_path}",{r.extension},{r.has_license},{r.license_stale},'
                    f'{r.has_guard},"{r.guard_name}","{r.expected_guard}","{issues}"\n')
    print(f'  CSV report written: {output_path}')


# ============================================================================
# Main Entry Point
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='Audit and fix license headers and header guards in C/C++ projects.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s D:\\VS2022\\EmulatR --audit
  %(prog)s D:\\VS2022\\EmulatR --git-snapshot --fix-all
  %(prog)s D:\\VS2022\\EmulatR --backup --fix-license --dry-run
  %(prog)s D:\\VS2022\\EmulatR --audit --csv-report report.csv
        """
    )

    parser.add_argument('project_root', type=str, help='Root directory of the project')
    parser.add_argument('--audit', action='store_true', default=True,
                        help='Report issues (default mode, always runs)')
    parser.add_argument('--fix-license', action='store_true',
                        help='Insert/replace license headers')
    parser.add_argument('--fix-guards', action='store_true',
                        help='Insert missing header guards')
    parser.add_argument('--fix-all', action='store_true',
                        help='Fix both license headers and guards')
    parser.add_argument('--backup', action='store_true',
                        help='Create zip backup before changes')
    parser.add_argument('--backup-dir', type=str, default=None,
                        help='Directory for backup zip (default: parent of project)')
    parser.add_argument('--git-snapshot', action='store_true',
                        help='Git commit current state before changes')
    parser.add_argument('--dry-run', action='store_true',
                        help='Show what would change without writing')
    parser.add_argument('--exclude', action='append', default=[],
                        help='Additional directories to exclude (repeatable)')
    parser.add_argument('--csv-report', type=str, default=None,
                        help='Write CSV report to file')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Show per-file details during scan')

    args = parser.parse_args()

    root = Path(args.project_root).resolve()
    if not root.is_dir():
        print(f'ERROR: {root} is not a directory.')
        sys.exit(1)

    if args.fix_all:
        args.fix_license = True
        args.fix_guards = True

    do_fix = args.fix_license or args.fix_guards
    excludes = DEFAULT_EXCLUDES | set(args.exclude)

    print(f'\n{"=" * 78}')
    print(f' ASA-EMulatR License & Header Guard Tool')
    print(f' Root: {root}')
    print(f' Mode: {"FIX" if do_fix else "AUDIT"}{" (DRY RUN)" if args.dry_run else ""}')
    print(f'{"=" * 78}\n')

    # ── Safety: Git snapshot ──
    if args.git_snapshot:
        print('[GIT SNAPSHOT]')
        if not git_snapshot(root):
            if do_fix and not args.dry_run:
                print('  Cannot proceed with fixes without git safety net.')
                print('  Use --backup instead, or --dry-run to preview.')
                sys.exit(1)

    # ── Safety: Zip backup ──
    if args.backup:
        print('[BACKUP]')
        backup_dir = Path(args.backup_dir) if args.backup_dir else None
        backup_path = create_backup(root, backup_dir)
        if backup_path is None and do_fix and not args.dry_run:
            print('  Cannot proceed with fixes without backup.')
            sys.exit(1)

    # ── Scan ──
    print('[SCANNING]')
    summary = AuditSummary()
    file_list = []

    for ext in sorted(FILE_EXTENSIONS):
        for filepath in sorted(root.rglob(f'*{ext}')):
            rel = filepath.relative_to(root)
            if should_exclude(rel, excludes):
                continue
            file_list.append(filepath)

    print(f'  Found {len(file_list)} source files to audit.\n')

    print('[AUDITING]')
    for filepath in file_list:
        report = audit_file(filepath, root)
        summary.reports.append(report)
        summary.total_files += 1

        if report.extension in {'.h', '.hpp', '.hxx', '.inl'}:
            summary.header_files += 1
        else:
            summary.source_files += 1

        if not report.has_license:
            summary.missing_license += 1
        if report.license_stale:
            summary.stale_license += 1
        if not report.has_guard and report.extension in {'.h', '.hpp', '.hxx', '.inl'}:
            summary.missing_guard += 1
        if report.guard_mismatch:
            summary.guard_mismatch += 1

        if args.verbose and report.issues:
            print(f'  ✗ {report.relative_path}: {", ".join(report.issues)}')
        elif args.verbose:
            print(f'  ✓ {report.relative_path}')

    # ── Fix ──
    if do_fix:
        action = 'DRY RUN' if args.dry_run else 'FIXING'
        print(f'\n[{action}]')

        for report in summary.reports:
            fixed = False

            if args.fix_license and (not report.has_license or report.license_stale):
                filepath = Path(report.filepath)
                if fix_license(filepath, report, dry_run=args.dry_run):
                    tag = 'WOULD FIX' if args.dry_run else 'FIXED'
                    action_desc = 'replaced stale' if report.license_stale else 'inserted'
                    print(f'  [{tag}] {action_desc} license: {report.relative_path}')
                    fixed = True

            if args.fix_guards and not report.has_guard and report.extension in {'.h', '.hpp'}:
                filepath = Path(report.filepath)
                if fix_guard(filepath, report, dry_run=args.dry_run):
                    tag = 'WOULD FIX' if args.dry_run else 'FIXED'
                    print(f'  [{tag}] inserted guard: {report.relative_path} ({report.expected_guard})')
                    fixed = True

            if fixed:
                summary.files_fixed += 1

    # ── Report ──
    print_audit_report(summary)

    if args.csv_report:
        write_csv_report(summary, Path(args.csv_report))

    # ── Post-fix git commit ──
    if do_fix and not args.dry_run and summary.files_fixed > 0:
        print('[POST-FIX]')
        if args.git_snapshot:
            git_snapshot(root, f'[LICENSE-TOOL] Updated {summary.files_fixed} files - '
                              f'license headers and/or header guards')
        print(f'  Done. {summary.files_fixed} files modified.')
        print(f'  To revert: git reset --hard HEAD~1')
    elif do_fix and args.dry_run:
        print(f'  Dry run complete. {summary.files_fixed} files would be modified.')
        print(f'  Rerun without --dry-run to apply changes.')


if __name__ == '__main__':
    main()
