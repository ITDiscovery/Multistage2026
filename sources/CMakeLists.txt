# ==============================================================================
# Multistage2026 Module Target
# ==============================================================================

add_library(Multistage2026 SHARED
    Multistage2026.cpp
    Multistage2026_MFD.cpp
    parser.cpp
    Guidance.cpp
    Particles.cpp
    peg.cpp
)

# ==============================================================================
# Include Directories
# ==============================================================================

# Point to header files
# ${CMAKE_SOURCE_DIR} refers to the root folder (~/Multistage2026)
target_include_directories(Multistage2026 PRIVATE
    .
    ../simpleini
    ${CMAKE_SOURCE_DIR}/Orbitersdk/include
)

# ==============================================================================
# Compiler Flags
# ==============================================================================

if(UNIX)
    target_compile_definitions(Multistage2026 PRIVATE
        ORBITER_MODULE
        _GNU_SOURCE
    )

    target_compile_options(Multistage2026 PRIVATE
        -w  # Suppress warnings for clean output
    )
endif()

# ==============================================================================
# Output Properties
# ==============================================================================

# Name the output "Multistage2026.so" (remove the "lib" prefix)
set_target_properties(Multistage2026 PROPERTIES
    PREFIX ""
    SUFFIX ".so"
)
