# SDF to Mesh converter (Marching Cubes)
# Converts SDF files back to OBJ mesh for visualization/debugging

add_executable(sdf_to_mesh
    sdf_to_mesh.cpp
)

target_link_libraries(sdf_to_mesh PRIVATE
    CLI11::CLI11
    sdfgen_options
)
