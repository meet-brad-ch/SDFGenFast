# Common library: data structures, utilities, mesh I/O, and unified API

# Header-only surface (vec.h, array1.h, array3.h, util.h, ...) shared by
# every module, including ones that must not link sdfgen_common itself.
add_library(sdfgen_headers INTERFACE)
target_include_directories(sdfgen_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(sdfgen_common STATIC
    sdfgen_unified.cpp
    sdf_io.cpp
    mesh_io.cpp
    mesh_io_obj.cpp
    mesh_io_stl.cpp
    mesh_repair.cpp
)

target_link_libraries(sdfgen_common
    PUBLIC
        sdfgen_headers
        sdfgen_cpu
    PRIVATE
        sdfgen_options
)

# Conditionally link against GPU implementation
if(HAVE_CUDA AND SDFGEN_BUILD_GPU)
    target_link_libraries(sdfgen_common PUBLIC sdfgen_gpu)
endif()

set_target_properties(sdfgen_common PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
