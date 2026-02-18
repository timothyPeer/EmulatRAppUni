#!/usr/bin/env python3
"""
Directory Scanner - Find files by name patterns
===============================================
Flexible script for scanning directory structures and finding files
based on various naming patterns and criteria.

Usage Examples:
    python scan_files.py --pattern "*.h" --dir /path/to/project
    python scan_files.py --regex ".*_impl\.h$" --recursive
    python scan_files.py --exact "SRMEnvStore.cpp" --dir . --summary
    python scan_files.py --contains "Logging" --ext cpp,h --size-min 1000
"""

import os
import re
import argparse
import fnmatch
import sys
from pathlib import Path
from datetime import datetime
from typing import List, Optional, Dict, Any


class FileScanner:
    """Enhanced file scanner with multiple search criteria"""
    
    def __init__(self, base_dir: str = "."):
        self.base_dir = Path(base_dir).resolve()
        self.results: List[Dict[str, Any]] = []
        
    def scan(self, 
             pattern: Optional[str] = None,
             regex: Optional[str] = None, 
             exact: Optional[str] = None,
             contains: Optional[str] = None,
             extensions: Optional[List[str]] = None,
             recursive: bool = True,
             case_sensitive: bool = False,
             size_min: Optional[int] = None,
             size_max: Optional[int] = None,
             modified_days: Optional[int] = None) -> List[Dict[str, Any]]:
        """
        Scan directory for files matching criteria
        
        Args:
            pattern: Glob pattern (*.cpp, *_impl.h, etc.)
            regex: Regular expression pattern  
            exact: Exact filename match
            contains: Filename contains substring
            extensions: List of file extensions to include
            recursive: Search subdirectories
            case_sensitive: Case-sensitive matching
            size_min: Minimum file size in bytes
            size_max: Maximum file size in bytes
            modified_days: Files modified within N days
            
        Returns:
            List of file info dictionaries
        """
        self.results.clear()
        
        # Compile regex if provided
        compiled_regex = None
        if regex:
            flags = 0 if case_sensitive else re.IGNORECASE
            try:
                compiled_regex = re.compile(regex, flags)
            except re.error as e:
                print(f"Error: Invalid regex '{regex}': {e}")
                return []
        
        # Set up directory iterator
        if recursive:
            file_iterator = self.base_dir.rglob("*")
        else:
            file_iterator = self.base_dir.glob("*")
            
        # Scan files
        for file_path in file_iterator:
            if not file_path.is_file():
                continue
                
            if self._matches_criteria(file_path, pattern, compiled_regex, exact, 
                                    contains, extensions, case_sensitive,
                                    size_min, size_max, modified_days):
                
                file_info = self._get_file_info(file_path)
                self.results.append(file_info)
        
        return self.results
    
    def _matches_criteria(self, file_path: Path, pattern: Optional[str], 
                         compiled_regex: Optional[re.Pattern], exact: Optional[str],
                         contains: Optional[str], extensions: Optional[List[str]],
                         case_sensitive: bool, size_min: Optional[int], 
                         size_max: Optional[int], modified_days: Optional[int]) -> bool:
        """Check if file matches all specified criteria"""
        
        filename = file_path.name
        
        # Case sensitivity handling
        if not case_sensitive:
            test_filename = filename.lower()
            test_exact = exact.lower() if exact else None
            test_contains = contains.lower() if contains else None
        else:
            test_filename = filename
            test_exact = exact
            test_contains = contains
        
        # Pattern matching
        if pattern and not fnmatch.fnmatch(test_filename, pattern.lower() if not case_sensitive else pattern):
            return False
            
        # Regex matching
        if compiled_regex and not compiled_regex.search(filename):
            return False
            
        # Exact name matching
        if exact and test_filename != test_exact:
            return False
            
        # Contains substring
        if contains and test_contains not in test_filename:
            return False
            
        # Extension filtering
        if extensions:
            file_ext = file_path.suffix.lstrip('.').lower()
            if file_ext not in [ext.lower() for ext in extensions]:
                return False
        
        # Size filtering
        try:
            file_size = file_path.stat().st_size
            if size_min and file_size < size_min:
                return False
            if size_max and file_size > size_max:
                return False
        except OSError:
            return False
            
        # Modification time filtering
        if modified_days:
            try:
                file_mtime = file_path.stat().st_mtime
                days_since_modified = (datetime.now().timestamp() - file_mtime) / (24 * 3600)
                if days_since_modified > modified_days:
                    return False
            except OSError:
                return False
                
        return True
    
    def _get_file_info(self, file_path: Path) -> Dict[str, Any]:
        """Get comprehensive file information"""
        try:
            stat = file_path.stat()
            relative_path = file_path.relative_to(self.base_dir)
            
            return {
                'name': file_path.name,
                'path': str(file_path),
                'relative_path': str(relative_path),
                'directory': str(file_path.parent),
                'extension': file_path.suffix,
                'size': stat.st_size,
                'modified': datetime.fromtimestamp(stat.st_mtime),
                'created': datetime.fromtimestamp(stat.st_ctime),
            }
        except OSError as e:
            return {
                'name': file_path.name,
                'path': str(file_path),
                'error': str(e)
            }


def format_size(size_bytes: int) -> str:
    """Format file size in human-readable format"""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size_bytes < 1024:
            return f"{size_bytes:.1f} {unit}"
        size_bytes /= 1024
    return f"{size_bytes:.1f} TB"


def print_results(results: List[Dict[str, Any]], detailed: bool = False, 
                 summary: bool = False) -> None:
    """Print scan results in various formats"""
    
    if not results:
        print("No files found matching criteria.")
        return
    
    if summary:
        print(f"\nSummary: Found {len(results)} files")
        
        # Group by extension
        ext_counts = {}
        total_size = 0
        
        for file_info in results:
            ext = file_info.get('extension', 'no extension')
            ext_counts[ext] = ext_counts.get(ext, 0) + 1
            total_size += file_info.get('size', 0)
        
        print(f"Total size: {format_size(total_size)}")
        print("\nBy extension:")
        for ext, count in sorted(ext_counts.items()):
            print(f"  {ext}: {count} files")
        print()
    
    # Print file list
    for i, file_info in enumerate(results, 1):
        if 'error' in file_info:
            print(f"{i:3d}. {file_info['path']} (ERROR: {file_info['error']})")
            continue
            
        if detailed:
            print(f"{i:3d}. {file_info['relative_path']}")
            print(f"     Size: {format_size(file_info['size'])}")
            print(f"     Modified: {file_info['modified'].strftime('%Y-%m-%d %H:%M:%S')}")
            print(f"     Full path: {file_info['path']}")
            print()
        else:
            size_str = f" ({format_size(file_info['size'])})" if file_info.get('size') else ""
            print(f"{i:3d}. {file_info['relative_path']}{size_str}")


def main():
    parser = argparse.ArgumentParser(
        description="Scan directory structure for files matching patterns",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --pattern "*.h" --dir /path/to/project
  %(prog)s --regex ".*_impl\.h$" --recursive
  %(prog)s --exact "SRMEnvStore.cpp" 
  %(prog)s --contains "Logging" --ext cpp,h --size-min 1000
  %(prog)s --pattern "*.cpp" --modified-days 7 --summary
        """)
    
    # Search criteria
    parser.add_argument('--pattern', '-p', 
                       help='Glob pattern (e.g., *.cpp, *_impl.h)')
    parser.add_argument('--regex', '-r',
                       help='Regular expression pattern')
    parser.add_argument('--exact', '-e',
                       help='Exact filename match')
    parser.add_argument('--contains', '-c',
                       help='Filename contains substring')
    parser.add_argument('--ext', '--extensions',
                       help='File extensions (comma-separated: cpp,h,txt)')
    
    # Search options
    parser.add_argument('--dir', '-d', default='.',
                       help='Directory to search (default: current)')
    parser.add_argument('--recursive', action='store_true', default=True,
                       help='Search subdirectories (default: True)')
    parser.add_argument('--no-recursive', dest='recursive', action='store_false',
                       help='Don\'t search subdirectories')
    parser.add_argument('--case-sensitive', action='store_true',
                       help='Case-sensitive matching')
    
    # Size filtering
    parser.add_argument('--size-min', type=int,
                       help='Minimum file size in bytes')
    parser.add_argument('--size-max', type=int, 
                       help='Maximum file size in bytes')
    
    # Time filtering
    parser.add_argument('--modified-days', type=int,
                       help='Files modified within N days')
    
    # Output options
    parser.add_argument('--detailed', action='store_true',
                       help='Show detailed file information')
    parser.add_argument('--summary', action='store_true',
                       help='Show summary statistics')
    parser.add_argument('--count-only', action='store_true',
                       help='Only show count of matching files')
    
    args = parser.parse_args()
    
    # Validate arguments
    if not any([args.pattern, args.regex, args.exact, args.contains, args.ext]):
        parser.error("Must specify at least one search criterion")
    
    # Parse extensions
    extensions = None
    if args.ext:
        extensions = [ext.strip() for ext in args.ext.split(',')]
    
    # Check directory
    if not os.path.isdir(args.dir):
        print(f"Error: Directory '{args.dir}' does not exist")
        sys.exit(1)
    
    # Perform scan
    scanner = FileScanner(args.dir)
    results = scanner.scan(
        pattern=args.pattern,
        regex=args.regex,
        exact=args.exact,
        contains=args.contains,
        extensions=extensions,
        recursive=args.recursive,
        case_sensitive=args.case_sensitive,
        size_min=args.size_min,
        size_max=args.size_max,
        modified_days=args.modified_days
    )
    
    # Output results
    if args.count_only:
        print(len(results))
    else:
        print_results(results, detailed=args.detailed, summary=args.summary)


if __name__ == '__main__':
    main()