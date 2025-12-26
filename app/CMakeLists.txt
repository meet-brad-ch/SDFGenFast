cmake_minimum_required(VERSION 3.20)

# Main executable
add_executable(SDFGen
    main.cpp
)

# Link against CPU library (always required)
target_link_libraries(SDFGen PRIVATE
    sdfgen_cpu
    sdfgen_common
    CLI11::CLI11
)

# Conditionally link against GPU library
if(HAVE_CUDA AND SDFGEN_BUILD_GPU)
    target_link_libraries(SDFGen PRIVATE sdfgen_gpu)
endif()

# VTK integration (optional)
if(VTK_FOUND)
    target_link_libraries(SDFGen PRIVATE ${VTK_LIBRARIES})
    target_include_directories(SDFGen PRIVATE ${VTK_INCLUDE_DIRS})
endif()

# Set C++ standard
set_target_properties(SDFGen PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include config.h from project root
target_include_directories(SDFGen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
