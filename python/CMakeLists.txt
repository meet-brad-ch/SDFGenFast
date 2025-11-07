# Python bindings for SDFGen using nanobind
cmake_minimum_required(VERSION 3.20)

# Find Python and nanobind
find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)

# Try to find nanobind
# First check if nanobind is installed via pip (find_package)
# If not, fall back to FetchContent
find_package(nanobind CONFIG QUIET)

if(NOT nanobind_FOUND)
    message(STATUS "nanobind not found via find_package, using FetchContent")

    include(FetchContent)
    FetchContent_Declare(
        nanobind
        GIT_REPOSITORY https://github.com/wjakob/nanobind.git
        GIT_TAG v2.0.0
    )
    FetchContent_MakeAvailable(nanobind)
else()
    message(STATUS "Found nanobind via find_package")
endif()

# Create the Python extension module
nanobind_add_module(sdfgen_ext sdfgen_py.cpp)

# Link against sdfgen libraries
target_link_libraries(sdfgen_ext PRIVATE
    sdfgen_common
    sdfgen_cpu
)

# Link GPU library if available
if(HAVE_CUDA)
    target_link_libraries(sdfgen_ext PRIVATE sdfgen_gpu)
endif()

# Set C++ standard
target_compile_features(sdfgen_ext PRIVATE cxx_std_17)

# Post-build: Create ready-to-use package structure at project root
# This allows pytest and Python imports to work immediately after build
set(SDFGEN_PACKAGE_DIR "${PROJECT_SOURCE_DIR}/sdfgen")

add_custom_command(TARGET sdfgen_ext POST_BUILD
    # Create package directory
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SDFGEN_PACKAGE_DIR}"

    # Copy Python wrapper
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/sdfgen.py"
            "${SDFGEN_PACKAGE_DIR}/__init__.py"

    # Copy extension module
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:sdfgen_ext>"
            "${SDFGEN_PACKAGE_DIR}/$<TARGET_FILE_NAME:sdfgen_ext>"

    COMMENT "Setting up sdfgen package at ${SDFGEN_PACKAGE_DIR}"
)

# On Windows with CUDA, copy CUDA runtime DLL
if(WIN32 AND HAVE_CUDA AND CUDAToolkit_FOUND)
    # Determine CUDA runtime DLL name based on CUDA version
    set(CUDA_VERSION_MAJOR ${CUDAToolkit_VERSION_MAJOR})
    set(CUDART_DLL_NAME "cudart64_${CUDA_VERSION_MAJOR}.dll")
    set(CUDART_DLL_PATH "${CUDAToolkit_BIN_DIR}/${CUDART_DLL_NAME}")

    if(EXISTS "${CUDART_DLL_PATH}")
        add_custom_command(TARGET sdfgen_ext POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CUDART_DLL_PATH}"
                    "${SDFGEN_PACKAGE_DIR}/${CUDART_DLL_NAME}"
            COMMENT "Copying CUDA runtime DLL to package"
        )
    else()
        message(WARNING "CUDA DLL not found at ${CUDART_DLL_PATH} - you may need to copy it manually or add CUDA bin to PATH")
    endif()
endif()

# Install the extension module
install(TARGETS sdfgen_ext LIBRARY DESTINATION .)
