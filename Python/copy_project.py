#!/usr/bin/env python3
"""
Project File Copier for EmulatR
Copies source files from old directory structure to new directory,
preserving the folder hierarchy.
"""

import os
import shutil
import argparse
from pathlib import Path
from datetime import datetime
from typing import Set, List, Tuple

class ProjectCopier:
    """Copies C++ project files while preserving directory structure"""
    
    # File extensions to copy
    SOURCE_EXTENSIONS = {'.cpp', '.cxx', '.cc', '.c'}
    HEADER_EXTENSIONS = {'.h', '.hpp', '.hxx', '.inl'}
    CONFIG_EXTENSIONS = {'.ui', '.qrc', '.rc', '.def', '.ico', '.png', '.json', '.xml', '.txt', '.md'}
    
    # Directories to skip entirely
    SKIP_DIRS = {
        'build', 'cmake-build-debug', 'cmake-build-release', 'cmake-build-relwithdebinfo',
        '.git', '.vs', '.vscode', '.idea',
        '__pycache__', '.mypy_cache',
        'Debug', 'Release', 'x64', 'x86',
        'out', 'bin', 'obj', 'lib',
        'GeneratedFiles', 'moc', 'uic', 'rcc'
    }
    
    # Files to skip
    SKIP_FILES = {
        '.gitignore', '.gitattributes', '.clang-format', '.editorconfig',
        'CMakeCache.txt', 'cmake_install.cmake', 'Makefile',
    }
    
    def __init__(self, source: Path, dest: Path, options: dict = None):
        self.source = source.resolve()
        self.dest = dest.resolve()
        self.options = options or {}
        
        # Tracking
        self.copied_files: List[Tuple[Path, Path]] = []
        self.skipped_files: List[Tuple[Path, str]] = []
        self.created_dirs: List[Path] = []
        self.errors: List[Tuple[Path, str]] = []
        
    def get_extensions_to_copy(self) -> Set[str]:
        """Get the set of file extensions to copy based on options"""
        extensions = set()
        extensions.update(self.SOURCE_EXTENSIONS)
        extensions.update(self.HEADER_EXTENSIONS)
        
        if self.options.get('include_config', True):
            extensions.update(self.CONFIG_EXTENSIONS)
            
        if self.options.get('include_cmake', False):
            pass  # CMakeLists.txt handled separately
            
        return extensions
        
    def should_copy_file(self, file_path: Path) -> Tuple[bool, str]:
        """Determine if a file should be copied"""
        name = file_path.name
        ext = file_path.suffix.lower()
        
        # Skip known skip files
        if name in self.SKIP_FILES:
            return False, "in skip list"
            
        # Handle CMakeLists.txt specially
        if name == 'CMakeLists.txt':
            if self.options.get('include_cmake', False):
                return True, ""
            return False, "CMake files excluded (use --include-cmake to copy)"
            
        # Check extension
        valid_extensions = self.get_extensions_to_copy()
        if ext in valid_extensions:
            return True, ""
            
        # Check for extensionless files we might want
        if not ext and name in {'Makefile', 'Doxyfile'}:
            return False, "build file excluded"
            
        return False, f"extension '{ext}' not in copy list"
        
    def should_skip_dir(self, dir_path: Path) -> bool:
        """Check if directory should be skipped"""
        return dir_path.name in self.SKIP_DIRS
        
    def copy(self, dry_run: bool = False) -> bool:
        """
        Perform the copy operation
        
        Args:
            dry_run: If True, only simulate the copy without actually copying
            
        Returns:
            True if successful (or dry run completed)
        """
        print(f"{'[DRY RUN] ' if dry_run else ''}Copying project files...")
        print(f"  Source: {self.source}")
        print(f"  Destination: {self.dest}")
        print()
        
        if not self.source.exists():
            print(f"ERROR: Source directory does not exist: {self.source}")
            return False
            
        if self.source == self.dest:
            print("ERROR: Source and destination cannot be the same!")
            return False
            
        # Check if dest is inside source or vice versa
        try:
            self.dest.relative_to(self.source)
            print("ERROR: Destination cannot be inside source directory!")
            return False
        except ValueError:
            pass  # Good, dest is not inside source
            
        try:
            self.source.relative_to(self.dest)
            print("ERROR: Source cannot be inside destination directory!")
            return False
        except ValueError:
            pass  # Good, source is not inside dest
            
        # Create destination if it doesn't exist
        if not dry_run:
            self.dest.mkdir(parents=True, exist_ok=True)
            
        # Walk the source directory
        self._copy_recursive(self.source, self.dest, dry_run)
        
        return len(self.errors) == 0
        
    def _copy_recursive(self, src_dir: Path, dest_dir: Path, dry_run: bool) -> None:
        """Recursively copy directory contents"""
        try:
            items = sorted(src_dir.iterdir())
        except PermissionError as e:
            self.errors.append((src_dir, f"Permission denied: {e}"))
            return
            
        for item in items:
            if item.is_dir():
                if self.should_skip_dir(item):
                    self.skipped_files.append((item, "directory in skip list"))
                    continue
                    
                # Create corresponding directory in destination
                new_dest = dest_dir / item.name
                if not dry_run:
                    new_dest.mkdir(exist_ok=True)
                self.created_dirs.append(new_dest)
                
                # Recurse
                self._copy_recursive(item, new_dest, dry_run)
                
            elif item.is_file():
                should_copy, reason = self.should_copy_file(item)
                
                if should_copy:
                    dest_file = dest_dir / item.name
                    
                    if not dry_run:
                        try:
                            shutil.copy2(item, dest_file)
                        except Exception as e:
                            self.errors.append((item, str(e)))
                            continue
                            
                    self.copied_files.append((item, dest_file))
                else:
                    self.skipped_files.append((item, reason))
                    
    def print_summary(self, verbose: bool = False) -> None:
        """Print summary of the copy operation"""
        print()
        print("=" * 70)
        print("COPY SUMMARY")
        print("=" * 70)
        
        # Count by type
        source_count = sum(1 for f, _ in self.copied_files if f.suffix.lower() in self.SOURCE_EXTENSIONS)
        header_count = sum(1 for f, _ in self.copied_files if f.suffix.lower() in self.HEADER_EXTENSIONS)
        other_count = len(self.copied_files) - source_count - header_count
        
        print(f"\nFiles copied: {len(self.copied_files)}")
        print(f"  - Source files (.cpp, .c, etc.): {source_count}")
        print(f"  - Header files (.h, .hpp, .inl): {header_count}")
        print(f"  - Other files: {other_count}")
        print(f"\nDirectories created: {len(self.created_dirs)}")
        print(f"Files/dirs skipped: {len(self.skipped_files)}")
        
        if self.errors:
            print(f"\n⚠ ERRORS: {len(self.errors)}")
            for path, error in self.errors:
                print(f"  {path}: {error}")
                
        if verbose:
            print("\n" + "-" * 70)
            print("COPIED FILES")
            print("-" * 70)
            
            # Group by directory
            by_dir = {}
            for src, dest in self.copied_files:
                rel_dir = src.parent.relative_to(self.source)
                if rel_dir not in by_dir:
                    by_dir[rel_dir] = []
                by_dir[rel_dir].append(src.name)
                
            for dir_path in sorted(by_dir.keys()):
                print(f"\n{dir_path}/")
                for filename in sorted(by_dir[dir_path]):
                    print(f"  {filename}")
                    
        if verbose and self.skipped_files:
            print("\n" + "-" * 70)
            print("SKIPPED (first 20)")
            print("-" * 70)
            for path, reason in self.skipped_files[:20]:
                try:
                    rel = path.relative_to(self.source)
                except ValueError:
                    rel = path
                print(f"  {rel}: {reason}")
            if len(self.skipped_files) > 20:
                print(f"  ... and {len(self.skipped_files) - 20} more")
                
    def generate_manifest(self, output_path: Path) -> None:
        """Generate a manifest file listing all copied files"""
        lines = []
        lines.append(f"# Copy Manifest - Generated {datetime.now().isoformat()}")
        lines.append(f"# Source: {self.source}")
        lines.append(f"# Destination: {self.dest}")
        lines.append(f"# Total files: {len(self.copied_files)}")
        lines.append("")
        
        for src, dest in sorted(self.copied_files):
            src_rel = src.relative_to(self.source)
            lines.append(str(src_rel))
            
        output_path.write_text("\n".join(lines), encoding='utf-8')
        print(f"\nManifest written to: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Copy C++ project files to new directory, preserving structure",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s Z:\\EmulatRApp Z:\\EmulatRApp_Consolidated
  %(prog)s ./old_project ./new_project --dry-run
  %(prog)s ./src ./dest --include-cmake --verbose
        """
    )
    
    parser.add_argument(
        "source",
        type=Path,
        help="Source directory to copy from"
    )
    
    parser.add_argument(
        "destination", 
        type=Path,
        help="Destination directory to copy to"
    )
    
    parser.add_argument(
        "-n", "--dry-run",
        action="store_true",
        help="Show what would be copied without actually copying"
    )
    
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Show detailed list of copied/skipped files"
    )
    
    parser.add_argument(
        "--include-cmake",
        action="store_true",
        help="Also copy CMakeLists.txt files (default: skip them)"
    )
    
    parser.add_argument(
        "--no-config",
        action="store_true",
        help="Skip config files (.ui, .qrc, .json, etc.)"
    )
    
    parser.add_argument(
        "--manifest",
        type=Path,
        default=None,
        help="Generate manifest file listing all copied files"
    )
    
    args = parser.parse_args()
    
    options = {
        'include_cmake': args.include_cmake,
        'include_config': not args.no_config,
    }
    
    copier = ProjectCopier(args.source, args.destination, options)
    
    success = copier.copy(dry_run=args.dry_run)
    copier.print_summary(verbose=args.verbose)
    
    if args.manifest:
        copier.generate_manifest(args.manifest)
        
    if args.dry_run:
        print("\n[DRY RUN] No files were actually copied.")
        print("Remove --dry-run flag to perform the actual copy.")
        
    return 0 if success else 1


if __name__ == "__main__":
    exit(main())