#!/usr/bin/env python3
# ============================================================================
# dead_file_finder.py - Orphaned File Detection via Include Graph Reachability
# ============================================================================
# Project: ASA-EMulatR - Alpha AXP Architecture Emulator
# Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
#
# Strategy:
#   Start from main.cpp. Recursively follow every #include "...".
#   Any .h or .cpp on disk that is NEVER reached is an orphan.
#
# The include graph from main.cpp IS the project.
# Everything else is dead weight.
#
# Usage:
#   python dead_file_finder.py <project_root> [options]
#
# Modes:
#   (default)            Audit only — report orphans, change nothing.
#   --move-to-hive DIR   Move orphans into a parallel directory tree.
#                         Structure is preserved: if orphan is at
#                           <root>/fooLib/bar.h
#                         it moves to:
#                           <hive>/fooLib/bar.h
#   --suggest-remove      Print a .bat removal script (no changes).
#   --dry-run             Show what --move-to-hive would do, without moving.
#
# Options:
#   --entry FILE          Entry point (default: auto-detect main.cpp)
#   --extra-entry FILE    Additional entry points (repeatable)
#   --verbose, -v         Show BFS walk trace and detail
#   --csv-report FILE     Write CSV report
#   --show-graph          Print the include dependency graph
#   --show-reachable      Print all reachable files by depth
#   --exclude DIR         Exclude directory from scan (repeatable)
#
# Examples:
#   python dead_file_finder.py D:\VS2022\EmulatR\EmulatRAppUni
#   python dead_file_finder.py D:\VS2022\EmulatR\EmulatRAppUni --move-to-hive D:\VS2022\deadFileHive --dry-run
#   python dead_file_finder.py D:\VS2022\EmulatR\EmulatRAppUni --move-to-hive D:\VS2022\deadFileHive
#   python dead_file_finder.py D:\VS2022\EmulatR\EmulatRAppUni --suggest-remove > cleanup.bat
#   python dead_file_finder.py D:\VS2022\EmulatR\EmulatRAppUni -v --csv-report orphans.csv
# ============================================================================

import os
import sys
import re
import shutil
import argparse
from pathlib import Path
from collections import defaultdict, deque
from dataclasses import dataclass, field
from typing import Dict, List, Set, Optional, Tuple
from datetime import datetime

# ============================================================================
# Configuration
# ============================================================================

SOURCE_EXTENSIONS = {'.cpp', '.cxx', '.cc', '.c'}
HEADER_EXTENSIONS = {'.h', '.hpp', '.hxx', '.inl'}
ALL_EXTENSIONS    = SOURCE_EXTENSIONS | HEADER_EXTENSIONS

DEFAULT_EXCLUDES = {
    'build', 'Build', '.git', 'third_party', 'external', 'deps',
    'node_modules', '__pycache__', '.vs', 'Debug', 'Release',
    'x64', 'x86', 'CMakeFiles', 'cmake-build-debug', 'cmake-build-release',
    '.moc', '.rcc', '.uic', 'autogen', 'GeneratedFiles',
}

INCLUDE_PATTERN = re.compile(r'#\s*include\s+"([^"]+)"')
MAIN_CANDIDATES = ['main.cpp', 'Main.cpp', 'main.cxx', 'main.cc']


# ============================================================================
# Data Structures
# ============================================================================

@dataclass
class FileNode:
    abs_path: Path
    rel_path: str                                        # forward-slash normalized
    extension: str
    size_bytes: int = 0
    raw_includes: List[str] = field(default_factory=list)
    resolved_deps: List[str] = field(default_factory=list)
    included_by: List[str] = field(default_factory=list)
    reachable: bool = False
    depth: int = -1                                      # -1 = unreachable
    is_entry: bool = False
    is_grain: bool = False


@dataclass
class BrokenInclude:
    includer: str
    raw_include: str
    line_number: int


# ============================================================================
# Phase 1 — File Discovery
# ============================================================================

def should_exclude(rel_path: Path, excludes: Set[str]) -> bool:
    for part in rel_path.parts:
        if part in excludes:
            return True
    return False


def discover_files(root: Path, excludes: Set[str]) -> Dict[str, FileNode]:
    files: Dict[str, FileNode] = {}
    for filepath in root.rglob('*'):
        if not filepath.is_file():
            continue
        if filepath.suffix.lower() not in ALL_EXTENSIONS:
            continue
        rel = filepath.relative_to(root)
        if should_exclude(rel, excludes):
            continue

        rel_str = str(rel).replace('\\', '/')
        files[rel_str] = FileNode(
            abs_path=filepath.resolve(),
            rel_path=rel_str,
            extension=filepath.suffix.lower(),
            size_bytes=filepath.stat().st_size,
            is_grain='_InstructionGrain' in filepath.name,
        )
    return files


# ============================================================================
# Phase 2 — Entry Point Detection
# ============================================================================

def find_entry_point(files: Dict[str, FileNode],
                     hint: Optional[str] = None) -> Optional[str]:
    if hint:
        hint_norm = hint.replace('\\', '/')
        if hint_norm in files:
            return hint_norm
        hint_base = Path(hint_norm).name
        matches = [r for r in files if Path(r).name == hint_base]
        if len(matches) == 1:
            return matches[0]
        if matches:
            return min(matches, key=len)

    for candidate_name in MAIN_CANDIDATES:
        matches = [r for r in files if Path(r).name == candidate_name]
        if len(matches) == 1:
            return matches[0]
        if matches:
            return min(matches, key=len)
    return None


# ============================================================================
# Phase 3 — Include Graph Construction
# ============================================================================

def parse_includes(filepath: Path) -> List[Tuple[str, int]]:
    results = []
    try:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            for lineno, line in enumerate(f, 1):
                m = INCLUDE_PATTERN.search(line)
                if m:
                    results.append((m.group(1).replace('\\', '/'), lineno))
    except Exception:
        pass
    return results


def resolve_include(raw: str, includer_rel: str,
                    files: Dict[str, FileNode], root: Path) -> Optional[str]:
    """
    Resolution order:
      1. Relative to includer's directory
      2. Direct match
      3. Strip leading ../
      4. Trailing path-component match
      5. Unique basename match
    """
    includer_dir = str(Path(includer_rel).parent).replace('\\', '/')
    if includer_dir == '.':
        includer_dir = ''

    # 1 — relative to includer
    if includer_dir:
        candidate = f'{includer_dir}/{raw}'
    else:
        candidate = raw
    try:
        normed = str(Path(candidate)).replace('\\', '/')
    except Exception:
        normed = candidate
    if normed in files:
        return normed

    # 2 — direct
    if raw in files:
        return raw

    # 3 — strip ../
    stripped = raw
    while stripped.startswith('../'):
        stripped = stripped[3:]
    if stripped in files:
        return stripped

    # 4 — trailing components
    raw_parts = Path(raw).parts
    for n in range(len(raw_parts), 0, -1):
        suffix = '/'.join(raw_parts[-n:])
        matches = [r for r in files if r.endswith(suffix)]
        if len(matches) == 1:
            return matches[0]
        if len(matches) > 1 and n >= 2:
            includer_top = (Path(includer_rel).parts[0]
                           if Path(includer_rel).parts else '')
            same_tree = [m for m in matches
                        if Path(m).parts and Path(m).parts[0] == includer_top]
            if len(same_tree) == 1:
                return same_tree[0]

    # 5 — unique basename
    basename = Path(raw).name
    matches = [r for r in files if Path(r).name == basename]
    if len(matches) == 1:
        return matches[0]

    return None


def build_dependency_graph(files: Dict[str, FileNode],
                           root: Path) -> List[BrokenInclude]:
    broken: List[BrokenInclude] = []
    for rel_path, node in files.items():
        raw_includes = parse_includes(node.abs_path)
        node.raw_includes = [r for r, _ in raw_includes]
        for raw, lineno in raw_includes:
            resolved = resolve_include(raw, rel_path, files, root)
            if resolved:
                node.resolved_deps.append(resolved)
                files[resolved].included_by.append(rel_path)
            else:
                broken.append(BrokenInclude(rel_path, raw, lineno))
    return broken


# ============================================================================
# Phase 4 — BFS Reachability Walk
# ============================================================================

def walk_reachability(files: Dict[str, FileNode],
                      entry_points: List[str],
                      verbose: bool = False) -> int:
    queue: deque[Tuple[str, int]] = deque()
    visited: Set[str] = set()

    for entry in entry_points:
        if entry in files:
            files[entry].is_entry = True
            files[entry].reachable = True
            files[entry].depth = 0
            queue.append((entry, 0))
            visited.add(entry)

    while queue:
        current, depth = queue.popleft()
        node = files[current]

        if verbose:
            indent = '  ' * min(depth, 8)
            print(f'  {indent}[{depth}] {current} '
                  f'({len(node.resolved_deps)} deps)')

        # Follow explicit #include edges
        for dep in node.resolved_deps:
            if dep not in visited:
                visited.add(dep)
                files[dep].reachable = True
                files[dep].depth = depth + 1
                queue.append((dep, depth + 1))

        # Implicit .cpp <-> .h pairing
        # Reaching Foo.h means Foo.cpp is compiled too, and vice versa.
        stem = Path(current).stem
        dir_prefix = str(Path(current).parent).replace('\\', '/')
        for ext in ALL_EXTENSIONS:
            if dir_prefix and dir_prefix != '.':
                sibling = f'{dir_prefix}/{stem}{ext}'
            else:
                sibling = f'{stem}{ext}'
            if sibling != current and sibling in files and sibling not in visited:
                visited.add(sibling)
                files[sibling].reachable = True
                files[sibling].depth = depth + 1
                queue.append((sibling, depth + 1))

    return len(visited)


def detect_aggregators(files: Dict[str, FileNode]) -> List[str]:
    """Find files that include 5+ grains (e.g. RegisterAllGrains.cpp)."""
    results = []
    for rel_path, node in files.items():
        grain_count = sum(1 for d in node.resolved_deps
                         if '_InstructionGrain' in d)
        if grain_count >= 5:
            results.append(rel_path)
    return results


# ============================================================================
# Phase 5 — Move to Dead File Hive
# ============================================================================

def move_to_hive(files: Dict[str, FileNode], root: Path,
                 hive_dir: Path, dry_run: bool = False) -> int:
    """
    Move orphaned files from <root>/rel/path/file.h  to  <hive>/rel/path/file.h.
    Preserves the full relative directory structure.
    Returns count of files moved.
    """
    orphans = sorted(
        [f for f in files.values() if not f.reachable],
        key=lambda f: f.rel_path
    )

    if not orphans:
        print('  No orphans to move.')
        return 0

    moved = 0
    errors = 0

    for node in orphans:
        src = node.abs_path
        # Build destination preserving directory structure
        dest = hive_dir / node.rel_path.replace('/', os.sep)
        dest_dir = dest.parent

        if dry_run:
            print(f'  [DRY RUN] {node.rel_path}')
            print(f'            -> {dest}')
            moved += 1
            continue

        try:
            dest_dir.mkdir(parents=True, exist_ok=True)
            shutil.move(str(src), str(dest))
            moved += 1
            print(f'  MOVED: {node.rel_path}')
        except Exception as e:
            errors += 1
            print(f'  ERROR: {node.rel_path} -> {e}')

    print()
    if dry_run:
        print(f'  Dry run complete. {moved} files would be moved to:')
        print(f'    {hive_dir}')
    else:
        print(f'  Moved {moved} files to: {hive_dir}')
        if errors:
            print(f'  Errors: {errors}')

    return moved


# ============================================================================
# Reporting
# ============================================================================

def print_report(files: Dict[str, FileNode], broken: List[BrokenInclude],
                 entry_points: List[str], verbose: bool = False):
    sep = '=' * 78
    total = len(files)
    headers = sum(1 for f in files.values() if f.extension in HEADER_EXTENSIONS)
    sources = sum(1 for f in files.values() if f.extension in SOURCE_EXTENSIONS)
    reachable = sum(1 for f in files.values() if f.reachable)
    orphans = [f for f in files.values() if not f.reachable]
    orphan_headers = [f for f in orphans if f.extension in HEADER_EXTENSIONS]
    orphan_sources = [f for f in orphans if f.extension in SOURCE_EXTENSIONS]
    orphan_grains = [f for f in orphans if f.is_grain]

    print(f'\n{sep}')
    print(f' DEAD FILE ANALYSIS - #include Reachability from main.cpp')
    print(f' Generated: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}')
    print(f'{sep}\n')

    print(f' Entry point(s):')
    for e in entry_points:
        print(f'   -> {e}')
    print()

    print(f' Files on disk:             {total:>5}')
    print(f'   Headers (.h/.hpp/.inl):  {headers:>5}')
    print(f'   Sources (.cpp/.cc):      {sources:>5}')
    print()
    pct_live = 100 * reachable / total if total else 0
    pct_dead = 100 * len(orphans) / total if total else 0
    print(f' REACHABLE from entry:      {reachable:>5}  ({pct_live:.0f}%)')
    print(f' ORPHANED  (unreachable):   {len(orphans):>5}  ({pct_dead:.0f}%)')
    print()
    print(f'   Orphaned headers:        {len(orphan_headers):>5}', end='')
    print('  OK' if not orphan_headers else '  <-- REVIEW')
    print(f'   Orphaned sources:        {len(orphan_sources):>5}', end='')
    print('  OK' if not orphan_sources else '  <-- REVIEW')
    print(f'   Orphaned grains:         {len(orphan_grains):>5}', end='')
    print('  OK' if not orphan_grains else '  <-- old pre-generated?')
    print(f'   Broken #includes:        {len(broken):>5}', end='')
    print('  OK' if not broken else '  <-- FIX')
    print()

    # ── Orphaned files grouped by directory ──
    if orphans:
        total_kb = sum(f.size_bytes for f in orphans) / 1024
        print(f'{"-" * 78}')
        print(f' ORPHANED FILES ({len(orphans)} files, {total_kb:.0f} KB)')
        print(f' Not reachable from main.cpp via #include graph.')
        print(f'{"-" * 78}\n')

        by_dir: Dict[str, List[FileNode]] = defaultdict(list)
        for node in sorted(orphans, key=lambda n: n.rel_path):
            d = str(Path(node.rel_path).parent).replace('\\', '/')
            by_dir[d].append(node)

        for d in sorted(by_dir):
            print(f'  [{d}/]' if d != '.' else '  [(root)]')
            for node in by_dir[d]:
                name = Path(node.rel_path).name
                size_kb = node.size_bytes / 1024
                tags = []
                if node.is_grain:
                    tags.append('GRAIN')
                if node.extension in SOURCE_EXTENSIONS:
                    tags.append('SOURCE')
                if node.included_by:
                    tags.append(f'incl. by {len(node.included_by)} orphan(s)')
                tag_str = f'  [{", ".join(tags)}]' if tags else ''
                print(f'     {name:<45} {size_kb:>7.1f} KB{tag_str}')
            print()

    # ── Broken includes in ACTIVE files only ──
    if broken:
        reachable_broken = [b for b in broken if files[b.includer].reachable]
        orphan_broken_count = len(broken) - len(reachable_broken)

        if reachable_broken:
            print(f'{"-" * 78}')
            print(f' BROKEN #INCLUDES IN ACTIVE FILES ({len(reachable_broken)})')
            print(f'{"-" * 78}\n')

            by_file: Dict[str, List[BrokenInclude]] = defaultdict(list)
            for b in reachable_broken:
                by_file[b.includer].append(b)

            for includer in sorted(by_file):
                print(f'  {includer}:')
                for b in sorted(by_file[includer], key=lambda x: x.line_number):
                    print(f'    L{b.line_number:>4}: #include "{b.raw_include}"')
                print()

        if orphan_broken_count > 0 and verbose:
            print(f'  ({orphan_broken_count} additional broken includes in '
                  f'orphaned files — expected, not shown unless -v)\n')
            if verbose:
                orphan_broken = [b for b in broken
                                if not files[b.includer].reachable]
                by_file2: Dict[str, int] = defaultdict(int)
                for b in orphan_broken:
                    by_file2[b.includer] += 1
                for inc in sorted(by_file2):
                    print(f'    {inc}: {by_file2[inc]} broken')
                print()

    # ── Summary ──
    print(f'{sep}')
    if orphans:
        total_kb = sum(f.size_bytes for f in orphans) / 1024
        print(f' {len(orphans)} orphaned files found ({total_kb:.0f} KB).')
        print(f'   --move-to-hive DIR   Move them to a parallel tree (safe).')
        print(f'   --suggest-remove     Generate a .bat removal script.')
        print(f'   --show-reachable     See what IS reachable.')
    else:
        print(f' All {total} files are reachable from main.cpp. No orphans.')
    print(f'{sep}\n')


def print_reachable_tree(files: Dict[str, FileNode]):
    reachable = sorted(
        [f for f in files.values() if f.reachable],
        key=lambda f: (f.depth, f.rel_path)
    )
    print(f'\n{"=" * 78}')
    print(f' REACHABLE FILES ({len(reachable)}) — sorted by depth from main.cpp')
    print(f'{"=" * 78}\n')

    current_depth = -1
    for node in reachable:
        if node.depth != current_depth:
            current_depth = node.depth
            label = 'ENTRY' if current_depth == 0 else f'depth {current_depth}'
            print(f'  -- {label} --')
        tag = ' [GRAIN]' if node.is_grain else ''
        print(f'    {node.rel_path}{tag}')
    print()


def print_include_graph(files: Dict[str, FileNode],
                        reachable_only: bool = True):
    print(f'\n{"=" * 78}')
    scope = 'reachable only' if reachable_only else 'all'
    print(f' INCLUDE GRAPH ({scope})')
    print(f'{"=" * 78}\n')

    for rel_path in sorted(files):
        node = files[rel_path]
        if reachable_only and not node.reachable:
            continue
        if not node.resolved_deps:
            continue
        tag = 'OK' if node.reachable else 'DEAD'
        entry = ' *ENTRY*' if node.is_entry else ''
        print(f'  [{tag}] {rel_path}{entry}')
        for dep in sorted(node.resolved_deps):
            dep_tag = 'OK' if files[dep].reachable else 'DEAD'
            print(f'         -> [{dep_tag}] {dep}')
        print()


def print_removal_script(files: Dict[str, FileNode]):
    orphans = sorted(
        [f for f in files.values() if not f.reachable],
        key=lambda f: f.rel_path
    )
    if not orphans:
        print('REM No orphaned files detected.')
        return

    total_kb = sum(f.size_bytes for f in orphans) / 1024
    print(f'@echo off')
    print(f'REM {"=" * 66}')
    print(f'REM Dead File Cleanup Script')
    print(f'REM Generated: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}')
    print(f'REM Files: {len(orphans)} ({total_kb:.0f} KB)')
    print(f'REM')
    print(f'REM REVIEW CAREFULLY. Prefer "git rm" for tracked files.')
    print(f'REM {"=" * 66}')
    print()
    print(f'echo WARNING: This will delete {len(orphans)} files ({total_kb:.0f} KB).')
    print(f'echo Press Ctrl+C to cancel, or')
    print(f'pause')
    print()

    by_dir: Dict[str, List[FileNode]] = defaultdict(list)
    for node in orphans:
        d = str(Path(node.rel_path).parent).replace('\\', '/')
        by_dir[d].append(node)

    for d in sorted(by_dir):
        print(f'REM --- {d}/ ---')
        for node in by_dir[d]:
            win_path = node.rel_path.replace('/', '\\')
            print(f'del "{win_path}"')
        print()

    print(f'echo Done. {len(orphans)} files removed.')


def write_csv_report(files: Dict[str, FileNode], output_path: Path):
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write('relative_path,extension,size_bytes,reachable,depth,'
                'is_entry,is_grain,included_by_count,includes_count\n')
        for rel_path in sorted(files):
            n = files[rel_path]
            f.write(f'"{n.rel_path}",{n.extension},{n.size_bytes},'
                    f'{n.reachable},{n.depth},{n.is_entry},{n.is_grain},'
                    f'{len(n.included_by)},{len(n.resolved_deps)}\n')
    print(f'  CSV report: {output_path}')


# ============================================================================
# Main
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='Find orphaned C/C++ files by walking #include graph from main.cpp.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
How it works:
  1. Discovers every .h/.cpp file under <project_root>
  2. Parses every #include "..." directive
  3. BFS from main.cpp through the include graph
  4. .cpp/.h pairs are implicitly linked (reaching one marks the other)
  5. Anything not reached is an orphan candidate

The --move-to-hive option safely relocates orphans to a parallel directory
tree. The relative path is preserved, so you can easily copy files back:

  Source:  D:\\VS2022\\EmulatR\\EmulatRAppUni\\fooLib\\bar.h
  Hive:    D:\\VS2022\\deadFileHive\\fooLib\\bar.h

Examples:
  %(prog)s D:\\VS2022\\EmulatR\\EmulatRAppUni
  %(prog)s D:\\VS2022\\EmulatR\\EmulatRAppUni --move-to-hive D:\\VS2022\\deadFileHive --dry-run
  %(prog)s D:\\VS2022\\EmulatR\\EmulatRAppUni --move-to-hive D:\\VS2022\\deadFileHive
  %(prog)s D:\\VS2022\\EmulatR\\EmulatRAppUni --suggest-remove > cleanup.bat
  %(prog)s D:\\VS2022\\EmulatR\\EmulatRAppUni --show-reachable
        """
    )

    parser.add_argument('project_root', help='Root directory (where main.cpp lives)')
    parser.add_argument('--entry', default=None,
                        help='Path to main.cpp (auto-detected if omitted)')
    parser.add_argument('--extra-entry', action='append', default=[],
                        help='Additional entry points (repeatable)')
    parser.add_argument('--move-to-hive', default=None, metavar='DIR',
                        help='Move orphans to this directory (preserves tree)')
    parser.add_argument('--dry-run', action='store_true',
                        help='Preview --move-to-hive without actually moving')
    parser.add_argument('--suggest-remove', action='store_true',
                        help='Print a .bat removal script to stdout')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Show BFS trace and extra detail')
    parser.add_argument('--csv-report', default=None, metavar='FILE',
                        help='Write CSV report')
    parser.add_argument('--show-graph', action='store_true',
                        help='Print include dependency graph')
    parser.add_argument('--show-reachable', action='store_true',
                        help='Print all reachable files by depth')
    parser.add_argument('--exclude', action='append', default=[],
                        help='Additional directories to exclude')

    args = parser.parse_args()
    root = Path(args.project_root).resolve()

    if not root.is_dir():
        print(f'ERROR: {root} is not a directory.', file=sys.stderr)
        sys.exit(1)

    excludes = DEFAULT_EXCLUDES | set(args.exclude)
    quiet = args.suggest_remove

    def log(msg=''):
        if not quiet:
            print(msg)

    log(f'\n{"=" * 78}')
    log(f' ASA-EMulatR Dead File Finder')
    log(f' Root: {root}')
    if args.move_to_hive:
        log(f' Hive: {args.move_to_hive}{"  (DRY RUN)" if args.dry_run else ""}')
    log(f'{"=" * 78}')

    # ── Phase 1: Discover ──
    log(f'\n[PHASE 1] Discovering files...')
    files = discover_files(root, excludes)
    log(f'  Found {len(files)} source/header files.')

    if not files:
        log('  No files found. Check root and exclusions.')
        sys.exit(1)

    # ── Phase 2: Entry points ──
    log(f'\n[PHASE 2] Locating entry point(s)...')
    entry_points: List[str] = []

    main_entry = find_entry_point(files, args.entry)
    if main_entry:
        entry_points.append(main_entry)
        log(f'  main.cpp -> {main_entry}')
    else:
        log(f'  WARNING: main.cpp not found!')
        if args.entry:
            log(f'  Tried: {args.entry}')
        log(f'  Use --entry path/to/main.cpp')

    for extra in args.extra_entry:
        found = find_entry_point(files, extra)
        if found:
            entry_points.append(found)
            log(f'  extra   -> {found}')
        else:
            log(f'  WARNING: not found: {extra}')

    if not entry_points:
        log(f'\n  ERROR: No entry points. Cannot compute reachability.')
        sys.exit(1)

    # ── Phase 3: Include graph ──
    log(f'\n[PHASE 3] Building #include dependency graph...')
    broken = build_dependency_graph(files, root)
    edges = sum(len(n.resolved_deps) for n in files.values())
    log(f'  {edges} include edges resolved.')
    log(f'  {len(broken)} unresolved includes.')

    # ── Phase 4: BFS walk ──
    log(f'\n[PHASE 4] Walking include graph from main.cpp...')
    if args.verbose:
        log()
    reached = walk_reachability(files, entry_points, verbose=args.verbose)
    log(f'  Reachable: {reached}/{len(files)} files.')

    # ── Aggregator check ──
    aggs = detect_aggregators(files)
    for a in aggs:
        if not files[a].reachable:
            log(f'\n  NOTE: {a} includes many grains but is itself unreachable.')
            log(f'        If it is compiled, add: --extra-entry {a}')

    orphan_count = sum(1 for f in files.values() if not f.reachable)
    log()

    # ── Output ──
    if args.suggest_remove:
        print_removal_script(files)
    else:
        print_report(files, broken, entry_points, verbose=args.verbose)

    if args.show_reachable:
        print_reachable_tree(files)

    if args.show_graph:
        print_include_graph(files, reachable_only=not args.verbose)

    if args.csv_report:
        write_csv_report(files, Path(args.csv_report))

    # ── Move to hive ──
    if args.move_to_hive and orphan_count > 0:
        hive_dir = Path(args.move_to_hive).resolve()
        print(f'\n{"=" * 78}')
        action = 'DRY RUN — MOVE TO HIVE' if args.dry_run else 'MOVING TO HIVE'
        print(f' [{action}]')
        print(f' Source: {root}')
        print(f' Dest:   {hive_dir}')
        print(f'{"=" * 78}\n')

        moved = move_to_hive(files, root, hive_dir, dry_run=args.dry_run)

        if not args.dry_run and moved > 0:
            print(f'\n  To restore any file:')
            print(f'    copy "{hive_dir}\\<relpath>" "{root}\\<relpath>"')
            print(f'\n  To restore everything:')
            print(f'    xcopy /E /Y "{hive_dir}\\*" "{root}\\"')
            print()

    elif args.move_to_hive and orphan_count == 0:
        print('  No orphans to move.')


if __name__ == '__main__':
    main()
