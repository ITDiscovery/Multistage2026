# Multistage2026 - Integration with Global Build Chain

# 1. Define the library target
add_library(Multistage2026 SHARED 
    Multistage2026.cpp
    parser.cpp
    peg.cpp
    Particles.cpp
    Guidance.cpp
)

# 2. Point to the global Include directories
target_include_directories(Multistage2026 PRIVATE 
    ${ORBITER_SOURCE_SDK_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/Extern/simpleini # Assuming the standard OpenOrbiter path
    ${VESSEL_COMMON_DIR}
)

# 3. Link against the SDK and core libraries
target_link_libraries(Multistage2026 PRIVATE 
    Orbitersdk 
    # Add any other core libs here if needed
)

# 4. Handle your Debug HUD toggle
if(DEBUG_HUD)
    target_compile_definitions(Multistage2026 PRIVATE _DEBUG_HUD)
endif()
