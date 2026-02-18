
#!/usr/bin/env python3
"""
Circular Include Dependency Checker for C++ Projects

Usage:
    python check_circular_includes.py /path/to/project
    python check_circular_includes.py /path/to/project --verbose
    python check_circular_includes.py /path/to/project --file FaultDispatcherBank.h
"""

import re
import sys
from pathlib import Path
from collections import defaultdict
from typing import Dict, Set, List, Optional, Tuple

class CircularIncludeChecker:
    def __init__(self, project_root: Path, verbose: bool = False):
        self.project_root = Path(project_root)
        self.verbose = verbose
        self.file_map: Dict[str, Path] = {}  # filename -> full path
        self.include_graph: Dict[Path, Set[Path]] = defaultdict(set)
        self.all_cycles: List[List[Path]] = []
        
    def scan_project(self):
        """Scan project directory and build file map."""
        print(f"Scanning project directory: {self.project_root}")
        
        # Find all header and source files
        patterns = ['**/*.h', '**/*.hpp', '**/*.hxx', '**/*.cpp', '**/*.cc', '**/*.cxx']
        
        for pattern in patterns:
            for filepath in self.project_root.glob(pattern):
                if filepath.is_file():
                    filename = filepath.name
                    
                    # Handle duplicate filenames (use relative path as key)
                    rel_path = filepath.relative_to(self.project_root)
                    self.file_map[str(rel_path)] = filepath
                    
                    # Also store by basename for quick lookup
                    if filename not in self.file_map:
                        self.file_map[filename] = filepath
        
        print(f"Found {len(self.file_map)} files")
        
    def extract_includes(self, filepath: Path) -> List[str]:
        """Extract all #include directives from a file."""
        includes = []
        
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                in_comment_block = False
                
                for line in f:
                    stripped = line.strip()
                    
                    # Skip multi-line comments
                    if '/*' in stripped:
                        in_comment_block = True
                    if '*/' in stripped:
                        in_comment_block = False
                        continue
                    if in_comment_block:
                        continue
                    
                    # Skip single-line comments
                    if stripped.startswith('//'):
                        continue
                    
                    # Match #include "..." or #include <...>
                    match = re.match(r'#include\s+[<"]([^>"]+)[>"]', stripped)
                    if match:
                        include_path = match.group(1)
                        includes.append(include_path)
                        
        except Exception as e:
            if self.verbose:
                print(f"Warning: Could not read {filepath}: {e}")
        
        return includes
    
    def resolve_include_path(self, include_path: str, from_file: Path) -> Optional[Path]:
        """Resolve an include path to an actual file."""
        
        # Try exact match in file map (basename)
        basename = Path(include_path).name
        if basename in self.file_map:
            return self.file_map[basename]
        
        # Try relative path from current file's directory
        relative_to_file = (from_file.parent / include_path).resolve()
        if relative_to_file.exists():
            return relative_to_file
        
        # Try relative to project root
        relative_to_root = (self.project_root / include_path).resolve()
        if relative_to_root.exists():
            return relative_to_root
        
        # Try stripping leading ../ and searching
        cleaned_path = include_path.lstrip('../').lstrip('./')
        if cleaned_path in self.file_map:
            return self.file_map[cleaned_path]
        
        # Last resort: search by basename
        search_name = Path(include_path).name
        for key, path in self.file_map.items():
            if path.name == search_name:
                return path
        
        return None
    
    def build_dependency_graph(self):
        """Build the include dependency graph."""
        print("Building dependency graph...")
        
        for filepath in set(self.file_map.values()):
            includes = self.extract_includes(filepath)
            
            for include_path in includes:
                resolved = self.resolve_include_path(include_path, filepath)
                if resolved:
                    self.include_graph[filepath].add(resolved)
                elif self.verbose:
                    print(f"  Could not resolve: {include_path} (from {filepath.name})")
        
        print(f"Built graph with {len(self.include_graph)} nodes")
    
    def find_cycles_from_node(self, start: Path, visited: Set[Path], 
                              path: List[Path]) -> List[List[Path]]:
        """Find all cycles starting from a given node using DFS."""
        cycles = []
        
        # Check if we've found a cycle
        if start in visited:
            # Extract the cycle
            try:
                cycle_start_idx = path.index(start)
                cycle = path[cycle_start_idx:] + [start]
                return [cycle]
            except ValueError:
                return []
        
        visited.add(start)
        path.append(start)
        
        # Explore neighbors
        for neighbor in self.include_graph.get(start, []):
            cycles.extend(self.find_cycles_from_node(neighbor, visited.copy(), path.copy()))
        
        return cycles
    
    def find_all_cycles(self):
        """Find all cycles in the include graph."""
        print("Searching for circular dependencies...")
        
        all_cycles = []
        checked_cycles = set()
        
        for node in self.include_graph.keys():
            cycles = self.find_cycles_from_node(node, set(), [])
            
            for cycle in cycles:
                # Normalize cycle (start from smallest element to avoid duplicates)
                if not cycle:
                    continue
                    
                # Create a canonical representation
                min_idx = cycle.index(min(cycle, key=lambda p: str(p)))
                normalized = tuple(cycle[min_idx:-1] + cycle[:min_idx])
                
                if normalized not in checked_cycles:
                    checked_cycles.add(normalized)
                    all_cycles.append(list(normalized))
        
        self.all_cycles = all_cycles
        return all_cycles
    
    def print_cycles(self):
        """Print all found cycles in a readable format."""
        if not self.all_cycles:
            print("\n✅ No circular dependencies found!")
            return
        
        print(f"\n❌ Found {len(self.all_cycles)} circular dependencies:\n")
        
        for idx, cycle in enumerate(self.all_cycles, 1):
            print(f"Cycle {idx}:")
            print("  " + cycle[0].name)
            
            for i in range(len(cycle)):
                current = cycle[i]
                next_file = cycle[(i + 1) % len(cycle)]
                
                # Show the include that causes the dependency
                includes = self.extract_includes(current)
                for inc in includes:
                    resolved = self.resolve_include_path(inc, current)
                    if resolved == next_file:
                        print(f"    → includes: {inc}")
                        print(f"    → {next_file.name}")
                        break
            
            print()
    
    def check_specific_file(self, filename: str):
        """Check for cycles involving a specific file."""
        # Find the file
        target_file = None
        for key, path in self.file_map.items():
            if filename in str(path):
                target_file = path
                break
        
        if not target_file:
            print(f"Error: File '{filename}' not found in project")
            return
        
        print(f"\nChecking cycles involving: {target_file.name}")
        print(f"Full path: {target_file}\n")
        
        # Find cycles involving this file
        relevant_cycles = [c for c in self.all_cycles if target_file in c]
        
        if not relevant_cycles:
            print(f"✅ No circular dependencies found involving {filename}")
        else:
            print(f"❌ Found {len(relevant_cycles)} circular dependencies involving {filename}:\n")
            
            for idx, cycle in enumerate(relevant_cycles, 1):
                # Start cycle from the target file
                if target_file in cycle:
                    start_idx = cycle.index(target_file)
                    rotated = cycle[start_idx:] + cycle[:start_idx]
                    
                    print(f"Cycle {idx}:")
                    print(f"  {rotated[0].name}")
                    
                    for i in range(len(rotated)):
                        current = rotated[i]
                        next_file = rotated[(i + 1) % len(rotated)]
                        
                        includes = self.extract_includes(current)
                        for inc in includes:
                            resolved = self.resolve_include_path(inc, current)
                            if resolved == next_file:
                                print(f"    → includes: {inc}")
                                print(f"    → {next_file.name}")
                                break
                    print()
    
    def generate_dot_graph(self, output_file: str = "dependencies.dot"):
        """Generate a GraphViz .dot file of the dependency graph."""
        print(f"\nGenerating dependency graph: {output_file}")
        
        with open(output_file, 'w') as f:
            f.write("digraph includes {\n")
            f.write("  rankdir=LR;\n")
            f.write("  node [shape=box];\n\n")
            
            # Mark cycle nodes in red
            cycle_nodes = set()
            for cycle in self.all_cycles:
                cycle_nodes.update(cycle)
            
            for node in cycle_nodes:
                f.write(f'  "{node.name}" [color=red,style=filled,fillcolor=lightpink];\n')
            
            f.write("\n")
            
            # Add edges
            for source, targets in self.include_graph.items():
                for target in targets:
                    # Check if this edge is part of a cycle
                    is_cycle_edge = any(
                        source in cycle and target in cycle
                        for cycle in self.all_cycles
                    )
                    
                    color = "red" if is_cycle_edge else "black"
                    f.write(f'  "{source.name}" -> "{target.name}" [color={color}];\n')
            
            f.write("}\n")
        
        print(f"  Generated {output_file}")
        print(f"  View with: dot -Tpng {output_file} -o dependencies.png")


def main():
    import argparse
    
    parser = argparse.ArgumentParser(
        description="Check for circular include dependencies in C++ projects"
    )
    parser.add_argument(
        "project_root",
        help="Root directory of the C++ project"
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Enable verbose output"
    )
    parser.add_argument(
        "--file", "-f",
        help="Check cycles involving a specific file"
    )
    parser.add_argument(
        "--dot",
        action="store_true",
        help="Generate GraphViz .dot file"
    )
    
    args = parser.parse_args()
    
    # Create checker
    checker = CircularIncludeChecker(args.project_root, verbose=args.verbose)
    
    # Scan and analyze
    checker.scan_project()
    checker.build_dependency_graph()
    checker.find_all_cycles()
    
    # Print results
    if args.file:
        checker.check_specific_file(args.file)
    else:
        checker.print_cycles()
    
    # Generate dot graph if requested
    if args.dot:
        checker.generate_dot_graph()
    
    # Return exit code
    sys.exit(1 if checker.all_cycles else 0)


if __name__ == "__main__":
    main()