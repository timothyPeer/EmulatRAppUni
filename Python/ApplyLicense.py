#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
from pathlib import Path

# Unique marker used to detect whether the header is already present.
MARKER = "Licensed under eNVy Systems Non-Commercial License v1.1"

LICENSE_BLOCK_LINES = [
    "// ============================================================================",
    "// MBoxBase.h - Memory Box (MBox) Implementation",
    "// ============================================================================",
    "// Project: ASA-EMulatR - Alpha AXP Architecture Emulator",
    "// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.",
    "// Licensed under eNVy Systems Non-Commercial License v1.1",
    "// ",
    "// Project Architect: Timothy Peer",
    "// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)",
    "// ",
    "// Description:",
    "//   Header-only Memory Box implementation with integrated TLB management.",
    "//   Handles all memory operations: loads, stores, translations, and TLB",
    "//   staging for both PAL IPR updates and hardware miss handling.",
    "//",
    "// Commercial use prohibited without separate license.",
    "// Contact: peert@envysys.com | https://envysys.com",
    "// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/",
    "// ============================================================================",
    "",
]

HEADER_EXTS = {".h", ".hpp", ".hh", ".hxx", ".inl"}
SOURCE_EXTS = {".c", ".cc", ".cpp", ".cxx"}
ALL_EXTS = HEADER_EXTS | SOURCE_EXTS


def detect_eol(text: str) -> str:
    # Prefer CRLF if it's present anywhere; otherwise LF.
    return "\r\n" if "\r\n" in text else "\n"


def already_has_license(text: str) -> bool:
    return MARKER in text


def insert_after_header_guard(lines: list[str], eol: str) -> tuple[list[str], bool]:
    """
    For headers: insert after #pragma once, or after the first consecutive
    #ifndef ... then #define ... pair.
    """
    # Find first non-empty line index (keep BOM-safe: Python already read it into text)
    n = len(lines)

    # Helper to normalize (no trailing newline in line strings)
    def strip(line: str) -> str:
        return line.strip()

    # 1) #pragma once
    for i in range(min(n, 50)):  # look near the top only
        if strip(lines[i]) == "#pragma once":
            insert_at = i + 1
            # Ensure one blank line before block only if needed
            if insert_at < n and strip(lines[insert_at]) != "":
                block = [""] + LICENSE_BLOCK_LINES
            else:
                block = LICENSE_BLOCK_LINES
            new_lines = lines[:insert_at] + block + lines[insert_at:]
            return new_lines, True

    # 2) Header guard pair: #ifndef ... followed by #define ...
    for i in range(min(n - 1, 80)):
        if strip(lines[i]).startswith("#ifndef "):
            # allow blank/comment lines between? user wants line 3 under guard, so require immediate #define next line
            j = i + 1
            if strip(lines[j]).startswith("#define "):
                insert_at = j + 1
                # Insert a blank line then block if next line isn't blank
                if insert_at < n and strip(lines[insert_at]) != "":
                    block = [""] + LICENSE_BLOCK_LINES
                else:
                    block = LICENSE_BLOCK_LINES
                new_lines = lines[:insert_at] + block + lines[insert_at:]
                return new_lines, True

    return lines, False


def insert_at_top(lines: list[str]) -> tuple[list[str], bool]:
    # Insert at very top (but keep an initial shebang if present—rare for C/C++ but harmless)
    if lines and lines[0].startswith("#!"):
        insert_at = 1
        block = [""] + LICENSE_BLOCK_LINES
        return lines[:insert_at] + block + lines[insert_at:], True

    # Normal: at top
    return LICENSE_BLOCK_LINES + lines, True


def process_file(path: Path, dry_run: bool) -> bool:
    try:
        raw = path.read_text(encoding="utf-8", errors="surrogateescape")
    except Exception as e:
        print(f"[SKIP] {path} (read error: {e})")
        return False

    if already_has_license(raw):
        return False

    eol = detect_eol(raw)
    # Split into lines without keeping line endings
    lines = raw.splitlines()

    changed = False
    if path.suffix.lower() in HEADER_EXTS:
        new_lines, changed = insert_after_header_guard(lines, eol)
        if not changed:
            # No recognizable guard/pragma once; fall back to top-of-file
            new_lines, changed = insert_at_top(lines)
    else:
        new_lines, changed = insert_at_top(lines)

    if not changed:
        return False

    new_text = eol.join(new_lines) + (eol if raw.endswith(("\n", "\r\n")) else "")
    if dry_run:
        print(f"[WOULD UPDATE] {path}")
        return True

    try:
        path.write_text(new_text, encoding="utf-8", errors="surrogateescape", newline="")
    except Exception as e:
        print(f"[FAIL] {path} (write error: {e})")
        return False

    print(f"[UPDATED] {path}")
    return True


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("root", nargs="?", default=".", help="Repo root directory")
    ap.add_argument("--dry-run", action="store_true", help="Show what would change without writing")
    ap.add_argument("--exclude-dir", action="append", default=[".git", "build", "out", ".vs"],
                    help="Directory names to skip (repeatable)")
    args = ap.parse_args()

    root = Path(args.root).resolve()
    exclude = set(args.exclude_dir)

    updated = 0
    scanned = 0

    for dirpath, dirnames, filenames in os.walk(root):
        # prune excluded dirs in-place
        dirnames[:] = [d for d in dirnames if d not in exclude]

        for fn in filenames:
            p = Path(dirpath) / fn
            if p.suffix.lower() not in ALL_EXTS:
                continue
            scanned += 1
            if process_file(p, args.dry_run):
                updated += 1

    print(f"\nScanned: {scanned}  |  Updated: {updated}  |  Dry-run: {args.dry_run}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())