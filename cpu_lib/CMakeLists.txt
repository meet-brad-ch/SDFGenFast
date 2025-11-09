cmake_minimum_required(VERSION 3.20)

# CPU implementation library
add_library(sdfgen_cpu STATIC
    makelevelset3.cpp
)

# Include common headers (vec.h, array3.h) - no linking to avoid circular dependency
target_include_directories(sdfgen_cpu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

# Threading support (used in fast sweeping)
find_package(Threads REQUIRED)
target_link_libraries(sdfgen_cpu PUBLIC Threads::Threads)

# Set C++ standard
set_target_properties(sdfgen_cpu PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# Make headers available
target_include_directories(sdfgen_cpu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
