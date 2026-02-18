#!/usr/bin/env python3
"""
generate_cmakelists.py

Generate a single-project CMakeLists.txt for a Qt6 + C++20 codebase.
Recursively includes all .cpp, .h, and .inl files under the given root.

Usage:
    python generate_cmakelists.py <project_root> [executable_name]

Example:
    python generate_cmakelists.py Z:\\EmulatRAppUni ASAEmulatR
"""

import sys
import pathlib

def main():
    if len(sys.argv) < 2:
        print("Usage: python generate_cmakelists.py <project_root> [executable_name]")
        sys.exit(1)

    project_root = pathlib.Path(sys.argv[1]).resolve()
    exe_name = sys.argv[2] if len(sys.argv) >= 3 else project_root.name

    if not project_root.exists():
        print(f"Error: directory does not exist: {project_root}")
        sys.exit(1)

    cmake_path = project_root / "CMakeLists.txt"

    cmake_contents = f"""cmake_minimum_required(VERSION 3.22)

project({exe_name}
    LANGUAGES CXX
)

# ============================================================================
# Global Build Policy
# ============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# Qt6
# ============================================================================

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_compile_definitions(
    QT_NO_KEYWORDS
    QT_USE_QSTRINGBUILDER
)

# ============================================================================
# Source Collection
# ============================================================================

file(GLOB_RECURSE PROJECT_SOURCES
    CONFIGURE_DEPENDS
    "*.cpp"
)

file(GLOB_RECURSE PROJECT_HEADERS
    CONFIGURE_DEPENDS
    "*.h"
    "*.inl"
)

# ============================================================================
# Executable
# ============================================================================

add_executable({exe_name}
    ${{PROJECT_SOURCES}}
    ${{PROJECT_HEADERS}}
)

target_include_directories({exe_name}
    PRIVATE
        ${{CMAKE_SOURCE_DIR}}
)

target_link_libraries({exe_name}
    PRIVATE
        Qt6::Core
        Qt6::Widgets
)

set_target_properties({exe_name} PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# ============================================================================
# IDE Source Grouping
# ============================================================================

source_group(TREE ${{CMAKE_SOURCE_DIR}}
    FILES
        ${{PROJECT_SOURCES}}
        ${{PROJECT_HEADERS}}
)
"""

    cmake_path.write_text(cmake_contents, encoding="utf-8")
    print(f"Generated {cmake_path}")

if __name__ == "__main__":
    main()
