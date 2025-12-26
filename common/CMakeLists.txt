cmake_minimum_required(VERSION 3.20)

# Common library: data structures, utilities, mesh I/O, and unified API
add_library(sdfgen_common STATIC
    sdfgen_unified.cpp
    sdf_io.cpp
    mesh_io.cpp
    mesh_io_obj.cpp
    mesh_io_stl.cpp
    mesh_repair.cpp
)

# Headers (vec.h, array3.h, etc.) are header-only
# Make headers available to any library that links against sdfgen_common
target_include_directories(sdfgen_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link against CPU implementation (always available)
target_link_libraries(sdfgen_common PUBLIC sdfgen_cpu)

# Conditionally link against GPU implementation
if(HAVE_CUDA AND SDFGEN_BUILD_GPU)
    target_link_libraries(sdfgen_common PUBLIC sdfgen_gpu)
endif()

# Provide config.h location
target_include_directories(sdfgen_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Set C++ and CUDA standards
set_target_properties(sdfgen_common PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED ON
    CUDA_SEPARABLE_COMPILATION ON
    POSITION_INDEPENDENT_CODE ON
)

# Note: Do NOT set CUDA_RESOLVE_DEVICE_SYMBOLS for libraries
# Device symbols should only be resolved at the final executable level
# to avoid duplicate symbol errors during linking
