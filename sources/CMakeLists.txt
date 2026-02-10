add_library(Multistage2026 SHARED
    Multistage2026.cpp
    Multistage2026_MFD.cpp
    parser.cpp
    Guidance.cpp
    Particles.cpp
    peg.cpp
)

# Point to header files
target_include_directories(Multistage2026 PRIVATE
    .
    ../simpleini
    /home/peter/orbiter/Orbitersdk/include
)

if(UNIX)
    target_compile_definitions(Multistage2026 PRIVATE
        ORBITER_MODULE
        _GNU_SOURCE
    )
    target_compile_options(Multistage2026 PRIVATE -w)
endif()

set_target_properties(Multistage2026 PROPERTIES
    PREFIX ""
    SUFFIX ".so"
)
# ==============================================================================
# Installation Logic
# ==============================================================================

# Define Orbiter Root
if(NOT DEFINED ORBITER_ROOT)
    set(ORBITER_ROOT "/usr/local/Orbiter")
endif()

# 1. Backup existing module
install(CODE "
    set(TARGET_FILE \"${ORBITER_ROOT}/Modules/libMultistage2026.so\")
    if(EXISTS \"\${TARGET_FILE}\")
        message(STATUS \"Backing up existing module to \${TARGET_FILE}.bak\")
        file(RENAME \"\${TARGET_FILE}\" \"\${TARGET_FILE}.bak\")
    endif()
")

# 2. Install AND Rename
# We install the target, but we use RENAME to change it from Multistage2026.so to libMultistage2026.so
install(TARGETS Multistage2026
    LIBRARY DESTINATION "${ORBITER_ROOT}/Modules"
    NAMELINK_SKIP
)

# 3. Explicit Rename (CMake's install RENAME feature is sometimes tricky with targets, 
# so we use a post-install hook to guarantee the name change)
install(CODE "
    message(STATUS \"Renaming installed file to libMultistage2026.so\")
    file(RENAME \"${ORBITER_ROOT}/Modules/Multistage2026.so\" \"${ORBITER_ROOT}/Modules/libMultistage2026.so\")
")
