cmake_minimum_required(VERSION 3.10)
project(Multistage2026)

# ==============================================================================
# Global Settings
# ==============================================================================

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Required for Linux .so

# Output Directory: Put the .so files in the main Modules folder
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Modules)

# ==============================================================================
# Subdirectories
# ==============================================================================

# This tells CMake to go look inside the sources folder for more instructions
add_subdirectory(sources)

