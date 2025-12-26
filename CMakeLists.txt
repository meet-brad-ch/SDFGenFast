cmake_minimum_required(VERSION 3.20)

project(SDFGen VERSION 2.0 LANGUAGES CXX)

# ============================================================================
# Fetch CLI11 for command-line parsing
# ============================================================================
include(FetchContent)
FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
)
FetchContent_MakeAvailable(cli11)

# ============================================================================
# Build Options
# ============================================================================
option(SDFGEN_BUILD_GPU "Build GPU-accelerated implementation" ON)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" OFF)

# ============================================================================
# Package Discovery - Auto-detect CUDA
# ============================================================================

# Only try to find and enable CUDA if GPU build is requested
if(SDFGEN_BUILD_GPU)
    # Try to find CUDA quietly first (for auto-detection)
    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
        message(STATUS "SDFGen: CUDA Toolkit found - GPU support available")
        # Enable CUDA language
        enable_language(CUDA)
    else()
        message(STATUS "SDFGen: CUDA Toolkit not found - forcing CPU-only build")
        # Force GPU build option to OFF if CUDA not available
        set(SDFGEN_BUILD_GPU OFF CACHE BOOL "Build GPU-accelerated implementation" FORCE)
    endif()
else()
    message(STATUS "SDFGen: GPU build disabled by user")
endif()

# ============================================================================
# Package Discovery - VTK (optional)
# ============================================================================

# VTK (optional, for .vti output format)
find_package(VTK QUIET)
if(VTK_FOUND)
    set(HAVE_VTK 1)
    message(STATUS "SDFGen: VTK found (${VTK_VERSION}) - .vti output enabled")
else()
    set(HAVE_VTK 0)
    message(STATUS "SDFGen: Building without VTK - binary .sdf output only")
endif()

# Set HAVE_CUDA based on final SDFGEN_BUILD_GPU value
if(SDFGEN_BUILD_GPU)
    set(HAVE_CUDA 1)
    message(STATUS "SDFGen: GPU acceleration enabled")
    message(STATUS "SDFGen: CUDA Compiler: ${CMAKE_CUDA_COMPILER}")
else()
    set(HAVE_CUDA 0)
    message(STATUS "SDFGen: GPU build disabled")
endif()

# Generate config.h from config.h.in
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

# Make generated config.h available to all subdirectories
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# ============================================================================
# Output Directories
# ============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

# ============================================================================
# Subdirectories
# ============================================================================

# Always build common and CPU libraries
add_subdirectory(common)
add_subdirectory(cpu_lib)

# Conditionally build GPU library
if(HAVE_CUDA AND SDFGEN_BUILD_GPU)
    add_subdirectory(gpu_lib)
endif()

# Always build the main application
add_subdirectory(app)

# Build utility tools
add_subdirectory(sdf_to_mesh)

# Always build tests
enable_testing()
add_subdirectory(tests)

# Conditionally build Python bindings
if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()

# ============================================================================
# Installation
# ============================================================================
install(TARGETS SDFGen
    RUNTIME DESTINATION bin
)

# ============================================================================
# Build Summary
# ============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "SDFGen Configuration Summary")
message(STATUS "========================================")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  C++ Compiler:   ${CMAKE_CXX_COMPILER}")
message(STATUS "  Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  VTK Support:    ${HAVE_VTK}")
message(STATUS "  CUDA Support:   ${HAVE_CUDA}")
if(HAVE_CUDA)
message(STATUS "  CUDA Compiler:  ${CMAKE_CUDA_COMPILER}")
endif()
message(STATUS "========================================")
message(STATUS "")
