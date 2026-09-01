# CPU implementation library
add_library(sdfgen_cpu STATIC
    makelevelset3.cpp
)

target_include_directories(sdfgen_cpu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Threading support (used in fast sweeping). sdfgen_headers provides the
# common headers (vec.h, array3.h) without linking sdfgen_common, which
# would be circular.
find_package(Threads REQUIRED)
target_link_libraries(sdfgen_cpu
    PUBLIC
        Threads::Threads
        sdfgen_headers
    PRIVATE
        sdfgen_options
)

set_target_properties(sdfgen_cpu PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
