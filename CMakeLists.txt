cmake_minimum_required(VERSION 3.23)

project(SDFGen VERSION 2.2.0 LANGUAGES CXX)

# Default to a Release build for single-configuration generators.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release Debug RelWithDebInfo MinSizeRel)
endif()

# One C++ standard for every target (carried by sdfgen_options below).
# Mixing standards across statically linked targets is an ODR/ABI hazard.
set(CMAKE_CXX_EXTENSIONS OFF)

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
option(BUILD_TESTING "Build the test suite" ON)
option(SDFGEN_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
set(SDFGEN_CUDA_ARCHITECTURES "all-major" CACHE STRING
    "CUDA architectures to compile for (semicolon list, or all-major/native)")

# ============================================================================
# Project options target
# ============================================================================
# Every first-party target links sdfgen_options. It carries the C++ standard,
# strict warning flags, and the include path of the generated config.h.
# CUDA device code is unaffected: the flags are gated on COMPILE_LANGUAGE:CXX.
add_library(sdfgen_options INTERFACE)
target_compile_features(sdfgen_options INTERFACE cxx_std_17)
if(MSVC)
    target_compile_options(sdfgen_options INTERFACE
        $<$<COMPILE_LANGUAGE:CXX>:/W4 /permissive- /utf-8>)
    if(SDFGEN_WARNINGS_AS_ERRORS)
        target_compile_options(sdfgen_options INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/WX>)
    endif()
else()
    target_compile_options(sdfgen_options INTERFACE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic>)
    if(SDFGEN_WARNINGS_AS_ERRORS)
        target_compile_options(sdfgen_options INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-Werror>)
    endif()
endif()

# ============================================================================
# Package Discovery - Auto-detect CUDA
# ============================================================================

# Only try to find and enable CUDA if GPU build is requested
if(SDFGEN_BUILD_GPU)
    # Try to find CUDA quietly first (for auto-detection)
    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
        message(STATUS "SDFGen: CUDA Toolkit found - GPU support available")
        # nvcc rejects host compilers newer than it knows (e.g. CUDA 12.x
        # with Visual Studio 2026 / MSVC 19.50+). Pass the override so the
        # build proceeds; parity tests still validate the result.
        if(MSVC AND MSVC_VERSION GREATER 1949 AND CUDAToolkit_VERSION VERSION_LESS 13.0)
            message(WARNING "SDFGen: MSVC ${MSVC_VERSION} is newer than CUDA "
                "${CUDAToolkit_VERSION} officially supports - adding "
                "-allow-unsupported-compiler. Run the GPU tests to confirm "
                "the combination works, or install a CUDA release that "
                "supports this Visual Studio version.")
            string(APPEND CMAKE_CUDA_FLAGS " -allow-unsupported-compiler")
        endif()
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

# Generate config.h into a dedicated directory so a stale copy in the
# source tree can never shadow it.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/config.h"
)

# config.h is exposed only through sdfgen_options (target-based, no
# directory-scope include_directories).
target_include_directories(sdfgen_options INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/generated)

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

# Tests (skipped when embedding, e.g. pip install sets BUILD_TESTING=OFF)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Conditionally build Python bindings
if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()

# ============================================================================
# Installation
# ============================================================================
# Skip the CLI install when building a Python wheel (scikit-build-core
# defines SKBUILD); the wheel must contain only the Python package.
if(NOT SKBUILD)
    install(TARGETS SDFGen
        RUNTIME DESTINATION bin
    )
endif()

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
