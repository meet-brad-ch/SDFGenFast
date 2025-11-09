// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// Test harness for validating SDF file I/O using STL files
#include "test_utils.h"
#include "mesh_io.h"
#include <iostream>
#include <cstdio>
#include <cstdint>

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "SDFGen STL File I/O Test\n";
    std::cout << "========================================\n\n";

    // STL file path (relative to tests directory - WORKING_DIRECTORY in CMakeLists.txt)
    const char* stl_path = argc > 1 ? argv[1] : "./resources/test_x3y4z5_bin.stl";

    // Test configuration
    const int32_t target_nx = argc > 2 ? atoi(argv[2]) : 32;  // Small grid for fast testing
    const int32_t padding = 1;

    std::cout << "Test Configuration:\n";
    std::cout << "  STL file:   " << stl_path << "\n";
    std::cout << "  Target Nx:  " << target_nx << "\n";
    std::cout << "  Padding:    " << padding << "\n\n";

    // Load STL file
    std::vector<Vec3f> vertList;
    std::vector<Vec3ui> faceList;
    Vec3f min_box, max_box;

    if (!meshio::load_stl(stl_path, vertList, faceList, min_box, max_box)) {
        std::cerr << "ERROR: Failed to load STL file\n";
        return 1;
    }

    test_utils::print_mesh_info(vertList, faceList, min_box, max_box);

    // Calculate grid parameters (proportional sizing)
    float dx;
    int32_t ny, nz;
    Vec3f origin;
    test_utils::calculate_grid_parameters(min_box, max_box, target_nx, padding, dx, ny, nz, origin);

    // File paths
    const char* cpu_filename = "test_stl_cpu.sdf";
    const char* gpu_filename = "test_stl_gpu.sdf";

    // Run test
    test_utils::SDFComparisonResult result;
    if (!test_utils::test_sdf_io_roundtrip(faceList, vertList, origin, dx,
                                           target_nx, ny, nz,
                                           cpu_filename, gpu_filename, result)) {
        return 1;
    }

    // Print results
    test_utils::print_test_summary("STL FILE I/O TEST", result);

    // Don't cleanup - leave files for inspection
    std::cout << "\nTest files saved (not deleted):\n";
    std::cout << "  " << cpu_filename << "\n";
    std::cout << "  " << gpu_filename << "\n\n";

    return result.passed() ? 0 : 1;
}
