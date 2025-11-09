// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// Test harness for validating SDF file I/O for both CPU and GPU implementations
#include "test_utils.h"
#include <iostream>
#include <cstdio>
#include <cstdint>

// Generate a simple unit cube mesh centered at origin
void generate_unit_cube(std::vector<Vec3f>& vertList, std::vector<Vec3ui>& faceList,
                        Vec3f& min_box, Vec3f& max_box) {
    // 8 vertices of a unit cube: [-0.5, 0.5]^3
    vertList = {
        Vec3f(-0.5f, -0.5f, -0.5f),  // 0
        Vec3f( 0.5f, -0.5f, -0.5f),  // 1
        Vec3f( 0.5f,  0.5f, -0.5f),  // 2
        Vec3f(-0.5f,  0.5f, -0.5f),  // 3
        Vec3f(-0.5f, -0.5f,  0.5f),  // 4
        Vec3f( 0.5f, -0.5f,  0.5f),  // 5
        Vec3f( 0.5f,  0.5f,  0.5f),  // 6
        Vec3f(-0.5f,  0.5f,  0.5f)   // 7
    };

    // 12 triangles (2 per face)
    faceList = {
        // Bottom face (z = -0.5)
        Vec3ui(0, 1, 2), Vec3ui(0, 2, 3),
        // Top face (z = 0.5)
        Vec3ui(4, 6, 5), Vec3ui(4, 7, 6),
        // Front face (y = -0.5)
        Vec3ui(0, 5, 1), Vec3ui(0, 4, 5),
        // Back face (y = 0.5)
        Vec3ui(2, 7, 3), Vec3ui(2, 6, 7),
        // Left face (x = -0.5)
        Vec3ui(0, 3, 7), Vec3ui(0, 7, 4),
        // Right face (x = 0.5)
        Vec3ui(1, 6, 2), Vec3ui(1, 5, 6)
    };

    min_box = Vec3f(-0.5f, -0.5f, -0.5f);
    max_box = Vec3f(0.5f, 0.5f, 0.5f);
}

// Test with specific parameters
bool test_configuration(
    int32_t grid_res,
    int32_t padding,
    const std::vector<Vec3f>& vertList,
    const std::vector<Vec3ui>& faceList,
    const Vec3f& min_box,
    const Vec3f& max_box
) {
    std::cout << "\n----------------------------------------\n";
    std::cout << "Testing grid_res=" << grid_res << ", padding=" << padding << "\n";
    std::cout << "----------------------------------------\n";

    // Calculate grid parameters
    float dx;
    int32_t ny, nz;
    Vec3f origin;
    test_utils::calculate_grid_parameters(min_box, max_box, grid_res, padding, dx, ny, nz, origin);

    // File paths
    const char* cpu_filename = "test_output_cpu.sdf";
    const char* gpu_filename = "test_output_gpu.sdf";

    // Run test
    test_utils::SDFComparisonResult result;
    if (!test_utils::test_sdf_io_roundtrip(faceList, vertList, origin, dx,
                                           grid_res, ny, nz,
                                           cpu_filename, gpu_filename, result)) {
        std::cout << "✗ FAILED\n";
        std::remove(cpu_filename);
        std::remove(gpu_filename);
        return false;
    }

    // Quick validation
    if (!result.passed()) {
        std::cout << "✗ FAILED: Result validation failed\n";
        std::remove(cpu_filename);
        std::remove(gpu_filename);
        return false;
    }

    std::cout << "✓ PASSED (max_diff=" << result.max_diff << ", speedup="
              << (result.cpu_time_ms / result.gpu_time_ms) << "x)\n";

    // Cleanup test files
    std::remove(cpu_filename);
    std::remove(gpu_filename);

    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "SDFGen File I/O Test (Parameter Variations)\n";
    std::cout << "========================================\n\n";

    // Generate test mesh once
    std::vector<Vec3f> vertList;
    std::vector<Vec3ui> faceList;
    Vec3f min_box, max_box;

    generate_unit_cube(vertList, faceList, min_box, max_box);
    std::cout << "Test Mesh:\n";
    test_utils::print_mesh_info(vertList, faceList, min_box, max_box);

    int32_t failures = 0;

    // ========================================
    // Test 1: Different grid resolutions
    // ========================================
    std::cout << "\n========================================\n";
    std::cout << "Test 1: Different Grid Resolutions\n";
    std::cout << "========================================\n";

    const int32_t fixed_padding = 2;
    const int32_t grid_resolutions[] = {16, 32, 64, 128};

    for (int32_t res : grid_resolutions) {
        if (!test_configuration(res, fixed_padding, vertList, faceList, min_box, max_box)) {
            failures++;
        }
    }

    // ========================================
    // Test 2: Different padding values
    // ========================================
    std::cout << "\n========================================\n";
    std::cout << "Test 2: Different Padding Values\n";
    std::cout << "========================================\n";

    const int32_t fixed_grid_res = 32;
    const int32_t padding_values[] = {1, 2, 3, 5, 10};

    for (int32_t pad : padding_values) {
        if (!test_configuration(fixed_grid_res, pad, vertList, faceList, min_box, max_box)) {
            failures++;
        }
    }

    // ========================================
    // Summary
    // ========================================
    std::cout << "\n========================================\n";
    std::cout << "File I/O Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests run: " << (sizeof(grid_resolutions)/sizeof(grid_resolutions[0]) +
                                   sizeof(padding_values)/sizeof(padding_values[0])) << "\n";
    std::cout << "Failures: " << failures << "\n";

    if (failures == 0) {
        std::cout << "✓ ALL FILE I/O TESTS PASSED\n";
        std::cout << "  Tested multiple grid resolutions and padding values\n";
        return 0;
    } else {
        std::cout << "✗ SOME FILE I/O TESTS FAILED\n";
        return 1;
    }
}
