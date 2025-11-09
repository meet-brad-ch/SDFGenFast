// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// Library Test: ASCII STL Format Support
// Tests ASCII STL loading and SDF generation using mesh_io library

#include "test_utils.h"
#include "mesh_io.h"
#include <iostream>
#include <cstdio>
#include <cstdint>

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "ASCII STL Format Test\n";
    std::cout << "========================================\n\n";

    // ASCII STL file path (relative to tests directory - WORKING_DIRECTORY in CMakeLists.txt)
    const char* ascii_stl_path = argc > 1 ? argv[1] : "./resources/test_x3y4z5_ascii.stl";
    const char* binary_stl_path = "./resources/test_x3y4z5_bin.stl";

    // Test configuration
    const int32_t target_nx = argc > 2 ? atoi(argv[2]) : 32;
    const int32_t padding = 1;

    std::cout << "Test Configuration:\n";
    std::cout << "  ASCII STL:  " << ascii_stl_path << "\n";
    std::cout << "  Binary STL: " << binary_stl_path << "\n";
    std::cout << "  Target Nx:  " << target_nx << "\n";
    std::cout << "  Padding:    " << padding << "\n\n";

    // ========================================
    // Test 1: Load ASCII STL
    // ========================================
    std::cout << "[Test 1] Loading ASCII STL file...\n";
    std::vector<Vec3f> vertList_ascii;
    std::vector<Vec3ui> faceList_ascii;
    Vec3f min_box_ascii, max_box_ascii;

    if (!meshio::load_stl(ascii_stl_path, vertList_ascii, faceList_ascii, min_box_ascii, max_box_ascii)) {
        std::cerr << "ERROR: Failed to load ASCII STL file\n";
        return 1;
    }

    std::cout << "✓ ASCII STL loaded successfully\n";
    test_utils::print_mesh_info(vertList_ascii, faceList_ascii, min_box_ascii, max_box_ascii);

    // ========================================
    // Test 2: Load Binary STL for comparison
    // ========================================
    std::cout << "[Test 2] Loading Binary STL file for comparison...\n";
    std::vector<Vec3f> vertList_binary;
    std::vector<Vec3ui> faceList_binary;
    Vec3f min_box_binary, max_box_binary;

    if (!meshio::load_stl(binary_stl_path, vertList_binary, faceList_binary, min_box_binary, max_box_binary)) {
        std::cerr << "ERROR: Failed to load binary STL file\n";
        return 1;
    }

    std::cout << "✓ Binary STL loaded successfully\n";
    test_utils::print_mesh_info(vertList_binary, faceList_binary, min_box_binary, max_box_binary);

    // ========================================
    // Test 3: Compare mesh data
    // ========================================
    std::cout << "[Test 3] Comparing ASCII and Binary STL data...\n";

    bool meshes_match = true;

    // Compare triangle counts
    if (faceList_ascii.size() != faceList_binary.size()) {
        std::cerr << "✗ Triangle count mismatch: ASCII=" << faceList_ascii.size()
                  << ", Binary=" << faceList_binary.size() << "\n";
        meshes_match = false;
    }

    // Compare vertex counts
    if (vertList_ascii.size() != vertList_binary.size()) {
        std::cerr << "✗ Vertex count mismatch: ASCII=" << vertList_ascii.size()
                  << ", Binary=" << vertList_binary.size() << "\n";
        meshes_match = false;
    }

    // Compare bounding boxes (should be identical for same mesh)
    Vec3f bbox_diff_min = min_box_ascii - min_box_binary;
    Vec3f bbox_diff_max = max_box_ascii - max_box_binary;

    constexpr float BBOX_TOLERANCE = 1e-5f;
    if (std::abs(bbox_diff_min[0]) > BBOX_TOLERANCE ||
        std::abs(bbox_diff_min[1]) > BBOX_TOLERANCE ||
        std::abs(bbox_diff_min[2]) > BBOX_TOLERANCE ||
        std::abs(bbox_diff_max[0]) > BBOX_TOLERANCE ||
        std::abs(bbox_diff_max[1]) > BBOX_TOLERANCE ||
        std::abs(bbox_diff_max[2]) > BBOX_TOLERANCE) {
        std::cerr << "✗ Bounding box mismatch\n";
        meshes_match = false;
    }

    if (meshes_match) {
        std::cout << "✓ ASCII and Binary STL data match\n";
        std::cout << "  Same triangle count, vertex count, and bounding box\n\n";
    } else {
        std::cerr << "✗ ASCII and Binary STL data do NOT match\n\n";
        return 1;
    }

    // ========================================
    // Test 4: Generate SDF from ASCII STL
    // ========================================
    std::cout << "[Test 4] Generating SDF from ASCII STL...\n\n";

    float dx;
    int32_t ny, nz;
    Vec3f origin;
    test_utils::calculate_grid_parameters(min_box_ascii, max_box_ascii, target_nx, padding, dx, ny, nz, origin);

    // File paths
    const char* ascii_sdf_filename = "test_ascii_stl_cpu.sdf";
    const char* ascii_sdf_gpu_filename = "test_ascii_stl_gpu.sdf";

    // Run SDF generation test
    test_utils::SDFComparisonResult result;
    if (!test_utils::test_sdf_io_roundtrip(faceList_ascii, vertList_ascii, origin, dx,
                                           target_nx, ny, nz,
                                           ascii_sdf_filename, ascii_sdf_gpu_filename, result)) {
        return 1;
    }

    // Print results
    test_utils::print_test_summary("ASCII STL TEST", result);

    // Cleanup
    std::cout << "\nCleanup: Removing test files...\n";
    std::remove(ascii_sdf_filename);
    std::remove(ascii_sdf_gpu_filename);

    return result.passed() ? 0 : 1;
}
