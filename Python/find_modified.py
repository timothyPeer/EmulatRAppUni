#!/usr/bin/env python3
"""
Find files modified within a specified time range.
"""

import os
import sys
import argparse
from datetime import datetime, timedelta
from pathlib import Path


def find_modified_files(root_dir, start_time, end_time=None, extensions=None):
    """
    Find files modified within a time range.
    
    Args:
        root_dir: Root directory to search
        start_time: Start datetime
        end_time: End datetime (default: now)
        extensions: List of file extensions to include
    
    Returns:
        List of (filepath, modification_time) tuples
    """
    if end_time is None:
        end_time = datetime.now()
    
    modified_files = []
    root_path = Path(root_dir).resolve()
    
    for root, dirs, files in os.walk(root_path):
        # Skip common directories
        dirs[:] = [d for d in dirs if d not in {
            '.git', '.svn', '__pycache__', 'node_modules',
            'build', 'dist', 'out', '.vs', '.vscode', 'bin', 'obj'
        }]
        
        for filename in files:
            filepath = Path(root) / filename
            
            # Extension filter
            if extensions and filepath.suffix.lower() not in extensions:
                continue
            
            try:
                mod_time = datetime.fromtimestamp(filepath.stat().st_mtime)
                
                if start_time <= mod_time <= end_time:
                    modified_files.append((str(filepath), mod_time))
            
            except (OSError, PermissionError):
                continue
    
    return modified_files


def main():
    parser = argparse.ArgumentParser(
        description='Find files modified within a time range'
    )
    
    parser.add_argument(
        'directory',
        nargs='?',
        default='.',
        help='Directory to search (default: current directory)'
    )
    
    parser.add_argument(
        '--hours',
        type=int,
        help='Find files modified in last N hours'
    )
    
    parser.add_argument(
        '--days',
        type=int,
        default=0,
        help='Find files modified in last N days (default: today only)'
    )
    
    parser.add_argument(
        '--ext',
        nargs='+',
        help='File extensions to include (e.g., .cpp .h)'
    )
    
    parser.add_argument(
        '--output',
        '-o',
        help='Output file for results'
    )
    
    parser.add_argument(
        '--relative',
        action='store_true',
        help='Show relative paths instead of absolute'
    )
    
    args = parser.parse_args()
    
    # Calculate time range
    now = datetime.now()
    
    if args.hours:
        start_time = now - timedelta(hours=args.hours)
    elif args.days:
        start_time = now - timedelta(days=args.days)
    else:
        # Default: today (since midnight)
        start_time = now.replace(hour=0, minute=0, second=0, microsecond=0)
    
    # Prepare extensions
    extensions = None
    if args.ext:
        extensions = [ext if ext.startswith('.') else f'.{ext}' 
                     for ext in args.ext]
    
    # Find files
    print(f"Searching: {Path(args.directory).resolve()}")
    print(f"Time range: {start_time.strftime('%Y-%m-%d %H:%M:%S')} to {now.strftime('%Y-%m-%d %H:%M:%S')}")
    if extensions:
        print(f"Extensions: {', '.join(extensions)}")
    print("-" * 80)
    
    files = find_modified_files(args.directory, start_time, now, extensions)
    
    if not files:
        print("No files found.")
        return
    
    # Sort by modification time (newest first)
    files.sort(key=lambda x: x[1], reverse=True)
    
    print(f"\nFound {len(files)} file(s):\n")
    
    # Prepare output
    root_path = Path(args.directory).resolve()
    lines = []
    
    for filepath, mod_time in files:
        if args.relative:
            try:
                display_path = Path(filepath).relative_to(root_path)
            except ValueError:
                display_path = filepath
        else:
            display_path = filepath
        
        time_str = mod_time.strftime("%Y-%m-%d %H:%M:%S")
        line = f"{time_str}  {display_path}"
        lines.append(line)
        print(line)
    
    # Save to file if requested
    if args.output:
        with open(args.output, 'w') as f:
            f.write(f"Files modified: {start_time.strftime('%Y-%m-%d %H:%M:%S')} to {now.strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write("=" * 80 + "\n\n")
            f.write('\n'.join(lines))
        print(f"\nSaved to: {args.output}")


if __name__ == "__main__":
    main()