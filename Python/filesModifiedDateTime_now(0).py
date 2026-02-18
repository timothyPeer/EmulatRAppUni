#!/usr/bin/env python3
"""
Find all files modified today in a directory tree.
"""

import os
import sys
from datetime import datetime
from pathlib import Path


def find_files_modified_today(root_dir='.', file_extensions=None):
    """
    Find all files modified today.
    
    Args:
        root_dir: Root directory to search (default: current directory)
        file_extensions: List of extensions to filter (e.g., ['.cpp', '.h'])
                        None = all files
    
    Returns:
        List of tuples: (filepath, modification_time)
    """
    today = datetime.now().date()
    modified_files = []
    
    root_path = Path(root_dir).resolve()
    
    print(f"Scanning: {root_path}")
    print(f"Today's date: {today}")
    print("-" * 80)
    
    for root, dirs, files in os.walk(root_path):
        # Skip common build/cache directories
        dirs[:] = [d for d in dirs if d not in {
            '.git', '.svn', '__pycache__', 'node_modules', 
            'build', 'dist', 'out', '.vs', '.vscode'
        }]
        
        for filename in files:
            filepath = Path(root) / filename
            
            # Filter by extension if specified
            if file_extensions and filepath.suffix not in file_extensions:
                continue
            
            try:
                # Get modification time
                mod_time = datetime.fromtimestamp(filepath.stat().st_mtime)
                
                # Check if modified today
                if mod_time.date() == today:
                    modified_files.append((str(filepath), mod_time))
            
            except (OSError, PermissionError) as e:
                # Skip files we can't access
                continue
    
    return modified_files


def format_file_list(files):
    """Format and display the list of modified files."""
    if not files:
        print("No files modified today.")
        return
    
    # Sort by modification time (newest first)
    files.sort(key=lambda x: x[1], reverse=True)
    
    print(f"\nFound {len(files)} file(s) modified today:\n")
    
    for filepath, mod_time in files:
        time_str = mod_time.strftime("%H:%M:%S")
        print(f"{time_str}  {filepath}")


def main():
    """Main entry point."""
    # Configuration
    root_directory = '.'  # Current directory
    
    # Filter for C++ files only (remove this line for all files)
    extensions = ['.cpp', '.h', '.hpp', '.c', '.cc', '.cxx', '.inl']
    
    # Or search all files:
    # extensions = None
    
    # Find files
    modified_files = find_files_modified_toda