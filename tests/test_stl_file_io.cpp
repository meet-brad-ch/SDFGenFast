// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// SDF file I/O round-trip test driven by an STL mesh
#include "test_utils.h"
#include "mesh_io.h"

int main() {
    return test_utils::run_mesh_file_io_test(
        "SDFGen STL File I/O Test", meshio::load_stl,
        "test_x3y4z5_bin.stl", "test_stl_cpu.sdf", "test_stl_gpu.sdf");
}
