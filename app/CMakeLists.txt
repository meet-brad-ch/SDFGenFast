# Main executable
add_executable(SDFGen
    main.cpp
)

# sdfgen_common transitively provides sdfgen_cpu (and sdfgen_gpu when built).
target_link_libraries(SDFGen PRIVATE
    sdfgen_common
    CLI11::CLI11
    sdfgen_options
)

# VTK integration (optional)
if(VTK_FOUND)
    target_link_libraries(SDFGen PRIVATE ${VTK_LIBRARIES})
    target_include_directories(SDFGen PRIVATE ${VTK_INCLUDE_DIRS})
endif()
