#!/usr/bin/env python3
import os
import sys

# ------------------------------------------------------------
# Configuration
# ------------------------------------------------------------
HEADER_EXTS = {".h", ".hpp", ".hh", ".cpp"}
EXCLUDE_DIRS = {
    ".git",
    ".vs",
    ".idea",
    "out",
    "build",
    "cmake-build-debug",
    "cmake-build-release"
}

# ------------------------------------------------------------
# Tree builder
# ------------------------------------------------------------
def collect_headers(root):
    tree = {}

    for dirpath, dirnames, filenames in os.walk(root):
        # Prune excluded directories in-place
        dirnames[:] = [
            d for d in dirnames
            if d not in EXCLUDE_DIRS
        ]

        headers = sorted(
            f for f in filenames
            if os.path.splitext(f)[1] in HEADER_EXTS
        )

        if headers:
            rel = os.path.relpath(dirpath, root)
            tree[rel] = headers

    return dict(sorted(tree.items()))

# ------------------------------------------------------------
# Pretty printer
# ------------------------------------------------------------
def print_tree(tree):
    for dirpath, headers in tree.items():
        print(f"{dirpath}/")
        for h in headers:
            print(f"  ├─ {h}")
        print()

# ------------------------------------------------------------
# Entry point
# ------------------------------------------------------------
def main():
    if len(sys.argv) != 2:
        print("Usage: python header_tree.py <project_root>")
        sys.exit(1)

    root = os.path.abspath(sys.argv[1])
    if not os.path.isdir(root):
        print(f"Error: not a directory: {root}")
        sys.exit(1)

    tree = collect_headers(root)
    print_tree(tree)


if __name__ == "__main__":
    main()
