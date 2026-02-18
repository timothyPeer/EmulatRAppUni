#!/usr/bin/env python3
"""
CMake Project Consolidator for EmulatR
Scans a multi-library C++ project and generates a unified CMakeLists.txt
"""

import os
import re
import argparse
from pathlib import Path
from collections import defaultdict
from dataclasses import dataclass, field
from typing import List, Dict, Set, Optional

@dataclass
class LibraryInfo:
    """Information about a library subdirectory"""
    name: str
    path: Path
    sources: List[Path] = field(default_factory=list)
    headers: List[Path] = field(default_factory=list)
    dependencies: List[str] = field(default_factory=list)
    compile_definitions: List[str] = field(default_factory=list)
    is_qt_module: bool = False
    link_libraries: List[str] = field(default_factory=list)

class ProjectAnalyzer:
    """Analyzes a C++ project structure"""
    
    SOURCE_EXTENSIONS = {'.cpp', '.cxx', '.cc', '.c'}
    HEADER_EXTENSIONS = {'.h', '.hpp', '.hxx', '.inl'}
    
    # Directories to skip
    SKIP_DIRS = {'build', 'cmake-build-debug', 'cmake-build-release', 
                 '.git', '__pycache__', 'Debug', 'Release', 'x64'}
    
    def __init__(self, root_path: Path):
        self.root = root_path
        self.libraries: Dict[str, LibraryInfo] = {}
        self.root_sources: List[Path] = []
        self.root_headers: List[Path] = []
        self.qt_used = False
        self.project_name = root_path.name
        
    def analyze(self) -> None:
        """Main analysis entry point"""
        print(f"Analyzing project at: {self.root}")
        
        # First pass: identify library directories and collect files
        self._scan_directory_structure()
        
        # Second pass: parse existing CMakeLists.txt for dependencies
        self._parse_cmake_files()
        
        # Third pass: infer dependencies from includes
        self._infer_dependencies()
        
        print(f"\nFound {len(self.libraries)} library directories")
        print(f"Root sources: {len(self.root_sources)}")
        print(f"Root headers: {len(self.root_headers)}")
        
    def _scan_directory_structure(self) -> None:
        """Scan directory tree and categorize files"""
        for item in self.root.iterdir():
            if item.is_file():
                ext = item.suffix.lower()
                if ext in self.SOURCE_EXTENSIONS:
                    self.root_sources.append(item)
                elif ext in self.HEADER_EXTENSIONS:
                    self.root_headers.append(item)
            elif item.is_dir() and item.name not in self.SKIP_DIRS:
                self._scan_library_directory(item)
                
    def _scan_library_directory(self, lib_dir: Path, parent_prefix: str = "") -> None:
        """Scan a potential library directory"""
        lib_name = f"{parent_prefix}{lib_dir.name}" if parent_prefix else lib_dir.name
        
        sources = []
        headers = []
        subdirs = []
        
        for item in lib_dir.iterdir():
            if item.is_file():
                ext = item.suffix.lower()
                if ext in self.SOURCE_EXTENSIONS:
                    sources.append(item)
                elif ext in self.HEADER_EXTENSIONS:
                    headers.append(item)
            elif item.is_dir() and item.name not in self.SKIP_DIRS:
                subdirs.append(item)
        
        # If directory has source/header files, treat as library
        if sources or headers:
            lib_info = LibraryInfo(
                name=lib_name,
                path=lib_dir,
                sources=sources,
                headers=headers
            )
            
            # Check for Qt usage
            for h in headers:
                try:
                    content = h.read_text(encoding='utf-8', errors='ignore')
                    if 'Q_OBJECT' in content or '#include <Q' in content:
                        lib_info.is_qt_module = True
                        self.qt_used = True
                        break
                except:
                    pass
                    
            self.libraries[lib_name] = lib_info
            
        # Recursively scan subdirectories
        for subdir in subdirs:
            self._scan_library_directory(subdir, f"{lib_name}/")
            
    def _parse_cmake_files(self) -> None:
        """Parse existing CMakeLists.txt files for additional info"""
        for lib_name, lib_info in self.libraries.items():
            cmake_file = lib_info.path / "CMakeLists.txt"
            if cmake_file.exists():
                self._parse_cmake_file(cmake_file, lib_info)
                
    def _parse_cmake_file(self, cmake_path: Path, lib_info: LibraryInfo) -> None:
        """Extract useful information from a CMakeLists.txt"""
        try:
            content = cmake_path.read_text(encoding='utf-8', errors='ignore')
            
            # Find target_link_libraries
            link_pattern = r'target_link_libraries\s*\(\s*\w+\s+(.*?)\)'
            for match in re.finditer(link_pattern, content, re.DOTALL | re.IGNORECASE):
                libs = match.group(1).strip()
                # Extract library names (simple parsing)
                for lib in re.findall(r'(\w+(?:Lib|lib|CORE)?)', libs):
                    if lib not in ('PUBLIC', 'PRIVATE', 'INTERFACE'):
                        lib_info.link_libraries.append(lib)
                        
            # Find compile definitions
            def_pattern = r'target_compile_definitions\s*\(\s*\w+\s+(?:PUBLIC|PRIVATE|INTERFACE)\s+(.*?)\)'
            for match in re.finditer(def_pattern, content, re.DOTALL | re.IGNORECASE):
                defs = match.group(1).strip()
                for d in re.findall(r'(\w+(?:=\w+)?)', defs):
                    lib_info.compile_definitions.append(d)
                    
            # Check for Qt
            if 'Qt' in content or 'find_package(Qt' in content:
                lib_info.is_qt_module = True
                self.qt_used = True
                
        except Exception as e:
            print(f"Warning: Could not parse {cmake_path}: {e}")
            
    def _infer_dependencies(self) -> None:
        """Infer inter-library dependencies from #include statements"""
        lib_headers = {}  # header filename -> library name
        
        # Build index of which library owns which header
        for lib_name, lib_info in self.libraries.items():
            for h in lib_info.headers:
                lib_headers[h.name] = lib_name
                
        # Scan each library's files for includes
        for lib_name, lib_info in self.libraries.items():
            deps = set()
            all_files = lib_info.sources + lib_info.headers
            
            for f in all_files:
                try:
                    content = f.read_text(encoding='utf-8', errors='ignore')
                    # Find #include "..." statements
                    for match in re.finditer(r'#include\s*"([^"]+)"', content):
                        inc_file = match.group(1)
                        # Get just the filename
                        inc_name = Path(inc_file).name
                        if inc_name in lib_headers:
                            dep_lib = lib_headers[inc_name]
                            if dep_lib != lib_name:
                                deps.add(dep_lib)
                except:
                    pass
                    
            lib_info.dependencies = sorted(deps)

    def generate_cmake(self, output_path: Optional[Path] = None) -> str:
        """Generate unified CMakeLists.txt"""
        lines = []
        
        # Header
        lines.append("# Auto-generated unified CMakeLists.txt")
        lines.append(f"# Generated by consolidate_cmake.py")
        lines.append("")
        lines.append("cmake_minimum_required(VERSION 3.16)")
        lines.append(f"project({self.project_name} LANGUAGES CXX)")
        lines.append("")
        lines.append("set(CMAKE_CXX_STANDARD 17)")
        lines.append("set(CMAKE_CXX_STANDARD_REQUIRED ON)")
        lines.append("set(CMAKE_INCLUDE_CURRENT_DIR ON)")
        lines.append("")
        
        # Qt setup if needed
        if self.qt_used:
            lines.append("# Qt Configuration")
            lines.append("set(CMAKE_AUTOMOC ON)")
            lines.append("set(CMAKE_AUTOUIC ON)")
            lines.append("set(CMAKE_AUTORCC ON)")
            lines.append("")
            lines.append("find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)")
            lines.append("")
            
        # Include directories - add all library paths
        lines.append("# Include directories")
        lines.append("include_directories(")
        lines.append("    ${CMAKE_CURRENT_SOURCE_DIR}")
        for lib_name, lib_info in sorted(self.libraries.items()):
            rel_path = lib_info.path.relative_to(self.root)
            lines.append(f"    ${{CMAKE_CURRENT_SOURCE_DIR}}/{rel_path}")
        lines.append(")")
        lines.append("")
        
        # Source file groups
        lines.append("#" + "=" * 70)
        lines.append("# Source Files by Module")
        lines.append("#" + "=" * 70)
        lines.append("")
        
        # Root sources
        if self.root_sources:
            lines.append("# Root sources")
            lines.append("set(ROOT_SOURCES")
            for src in sorted(self.root_sources):
                lines.append(f"    {src.name}")
            lines.append(")")
            lines.append("")
            
        # Library sources
        for lib_name, lib_info in sorted(self.libraries.items()):
            var_name = self._make_var_name(lib_name)
            rel_path = lib_info.path.relative_to(self.root)
            
            if lib_info.sources:
                lines.append(f"# {lib_name} sources")
                lines.append(f"set({var_name}_SOURCES")
                for src in sorted(lib_info.sources):
                    src_rel = src.relative_to(self.root)
                    lines.append(f"    {src_rel}")
                lines.append(")")
                lines.append("")
                
        # Aggregate all sources
        lines.append("# All sources combined")
        lines.append("set(ALL_SOURCES")
        if self.root_sources:
            lines.append("    ${ROOT_SOURCES}")
        for lib_name, lib_info in sorted(self.libraries.items()):
            if lib_info.sources:
                var_name = self._make_var_name(lib_name)
                lines.append(f"    ${{{var_name}_SOURCES}}")
        lines.append(")")
        lines.append("")
        
        # Create executable or library
        lines.append("#" + "=" * 70)
        lines.append("# Build Target")
        lines.append("#" + "=" * 70)
        lines.append("")
        
        # Check if this looks like an executable (has main.cpp)
        has_main = any(s.name == 'main.cpp' for s in self.root_sources)
        
        if has_main:
            lines.append(f"add_executable({self.project_name}")
            lines.append("    ${ALL_SOURCES}")
            lines.append(")")
        else:
            lines.append(f"add_library({self.project_name} STATIC")
            lines.append("    ${ALL_SOURCES}")
            lines.append(")")
            
        lines.append("")
        
        # Link Qt if needed
        if self.qt_used:
            lines.append(f"target_link_libraries({self.project_name}")
            lines.append("    Qt6::Core")
            lines.append("    Qt6::Gui")
            lines.append("    Qt6::Widgets")
            lines.append(")")
            lines.append("")
            
        # Compile definitions
        all_defs = set()
        for lib_info in self.libraries.values():
            all_defs.update(lib_info.compile_definitions)
        if all_defs:
            lines.append(f"target_compile_definitions({self.project_name} PRIVATE")
            for d in sorted(all_defs):
                lines.append(f"    {d}")
            lines.append(")")
            lines.append("")
            
        # Source groups for IDE organization
        lines.append("#" + "=" * 70)
        lines.append("# IDE Source Groups (preserves folder structure)")
        lines.append("#" + "=" * 70)
        lines.append("")
        
        for lib_name, lib_info in sorted(self.libraries.items()):
            if lib_info.sources or lib_info.headers:
                var_name = self._make_var_name(lib_name)
                # Use forward slashes in group name for cross-platform
                group_name = lib_name.replace('/', '\\\\')
                
                if lib_info.sources:
                    files = " ".join(f'"{s.relative_to(self.root)}"' for s in sorted(lib_info.sources))
                    lines.append(f'source_group("{group_name}" FILES {files})')
                if lib_info.headers:
                    files = " ".join(f'"{h.relative_to(self.root)}"' for h in sorted(lib_info.headers))
                    lines.append(f'source_group("{group_name}" FILES {files})')
                    
        lines.append("")
        
        result = "\n".join(lines)
        
        if output_path:
            output_path.write_text(result, encoding='utf-8')
            print(f"\nGenerated: {output_path}")
            
        return result
    
    def _make_var_name(self, lib_name: str) -> str:
        """Convert library name to valid CMake variable name"""
        return lib_name.upper().replace('/', '_').replace('-', '_').replace('.', '_')
    
    def generate_report(self) -> str:
        """Generate analysis report"""
        lines = []
        lines.append("=" * 70)
        lines.append("PROJECT ANALYSIS REPORT")
        lines.append("=" * 70)
        lines.append(f"\nProject: {self.project_name}")
        lines.append(f"Root: {self.root}")
        lines.append(f"Qt Used: {self.qt_used}")
        lines.append("")
        
        lines.append("-" * 70)
        lines.append("LIBRARIES FOUND")
        lines.append("-" * 70)
        
        total_sources = len(self.root_sources)
        total_headers = len(self.root_headers)
        
        for lib_name, lib_info in sorted(self.libraries.items()):
            lines.append(f"\n{lib_name}:")
            lines.append(f"  Path: {lib_info.path.relative_to(self.root)}")
            lines.append(f"  Sources: {len(lib_info.sources)}")
            lines.append(f"  Headers: {len(lib_info.headers)}")
            if lib_info.dependencies:
                lines.append(f"  Dependencies: {', '.join(lib_info.dependencies[:5])}")
                if len(lib_info.dependencies) > 5:
                    lines.append(f"                ... and {len(lib_info.dependencies) - 5} more")
            if lib_info.is_qt_module:
                lines.append("  Qt Module: Yes")
            total_sources += len(lib_info.sources)
            total_headers += len(lib_info.headers)
            
        lines.append("")
        lines.append("-" * 70)
        lines.append("SUMMARY")
        lines.append("-" * 70)
        lines.append(f"Total Libraries: {len(self.libraries)}")
        lines.append(f"Total Source Files: {total_sources}")
        lines.append(f"Total Header Files: {total_headers}")
        
        # Dependency analysis
        lines.append("")
        lines.append("-" * 70)
        lines.append("DEPENDENCY GRAPH (inferred from includes)")
        lines.append("-" * 70)
        
        dep_count = defaultdict(int)
        for lib_info in self.libraries.values():
            for dep in lib_info.dependencies:
                dep_count[dep] += 1
                
        if dep_count:
            lines.append("\nMost depended-upon libraries:")
            for lib, count in sorted(dep_count.items(), key=lambda x: -x[1])[:10]:
                lines.append(f"  {lib}: {count} dependents")
                
        return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Consolidate multi-library CMake project into single CMakeLists.txt"
    )
    parser.add_argument(
        "project_path",
        type=Path,
        help="Path to the project root directory"
    )
    parser.add_argument(
        "-o", "--output",
        type=Path,
        default=None,
        help="Output path for generated CMakeLists.txt (default: project_path/CMakeLists_unified.txt)"
    )
    parser.add_argument(
        "-r", "--report",
        action="store_true",
        help="Generate analysis report"
    )
    parser.add_argument(
        "--report-file",
        type=Path,
        default=None,
        help="Save report to file"
    )
    
    args = parser.parse_args()
    
    if not args.project_path.exists():
        print(f"Error: Path does not exist: {args.project_path}")
        return 1
        
    analyzer = ProjectAnalyzer(args.project_path)
    analyzer.analyze()
    
    # Generate report if requested
    if args.report or args.report_file:
        report = analyzer.generate_report()
        print(report)
        if args.report_file:
            args.report_file.write_text(report, encoding='utf-8')
            print(f"\nReport saved to: {args.report_file}")
    
    # Generate CMake
    output_path = args.output or (args.project_path / "CMakeLists_unified.txt")
    cmake_content = analyzer.generate_cmake(output_path)
    
    print(f"\n✓ Unified CMakeLists.txt generated successfully!")
    print(f"\nNext steps:")
    print(f"  1. Review the generated file: {output_path}")
    print(f"  2. Rename to CMakeLists.txt (backup existing first)")
    print(f"  3. Remove or archive individual subdirectory CMakeLists.txt files")
    print(f"  4. Test build: mkdir build && cd build && cmake .. && cmake --build .")
    
    return 0


if __name__ == "__main__":
    exit(main())