// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// Library Test: Mode 1 Legacy (dx-based sizing)
// Tests grid dimension calculation from dx parameter

#include "test_utils.h"
#include "mesh_io.h"
#include <iostream>
#include <cstdio>
#include <cstdint>
#include <cmath>

// Calculate expected grid dimensions from dx (Mode 1 logic)
void calculate_mode1_dimensions(
    const Vec3f& min_box,
    const Vec3f& max_box,
    float dx,
    int32_t padding,
    int32_t& expected_nx,
    int32_t& expected_ny,
    int32_t& expected_nz
) {
    Vec3f mesh_size = max_box - min_box;

    // Mode 1 calculates dimensions as: ceil(size / dx) + 2*padding
    expected_nx = static_cast<int32_t>(std::ceil(mesh_size[0] / dx)) + 2 * padding;
    expected_ny = static_cast<int32_t>(std::ceil(mesh_size[1] / dx)) + 2 * padding;
    expected_nz = static_cast<int32_t>(std::ceil(mesh_size[2] / dx)) + 2 * padding;
}

// Test grid calculation with a specific dx value
bool test_dx_value(
    float dx,
    int32_t padding,
    const std::vector<Vec3f>& vertList,
    const std::vector<Vec3ui>& faceList,
    const Vec3f& min_box,
    const Vec3f& max_box
) {
    std::cout << "\n----------------------------------------\n";
    std::cout << "Testing dx=" << dx << ", padding=" << padding << "\n";
    std::cout << "----------------------------------------\n";

    // Calculate expected dimensions using Mode 1 logic
    int32_t expected_nx, expected_ny, expected_nz;
    calculate_mode1_dimensions(min_box, max_box, dx, padding, expected_nx, expected_ny, expected_nz);

    std::cout << "Expected dimensions: " << expected_nx << "x" << expected_ny << "x" << expected_nz << "\n";

    // Calculate origin (Mode 1: centered with padding)
    Vec3f origin = min_box - Vec3f(dx * padding, dx * padding, dx * padding);

    std::cout << "Expected origin: (" << origin << ")\n";
    std::cout << "Cell size: " << dx << "\n\n";

    // Generate SDF with these parameters
    std::cout << "Generating SDF with CPU backend...\n" << std::flush;
    Array3f phi_cpu;
    double cpu_time_ms;
    test_utils::generate_sdf_with_timing(
        faceList, vertList, origin, dx,
        expected_nx, expected_ny, expected_nz,
        phi_cpu, sdfgen::HardwareBackend::CPU, cpu_time_ms
    );

    // Verify dimensions
    if (phi_cpu.ni != expected_nx || phi_cpu.nj != expected_ny || phi_cpu.nk != expected_nz) {
        std::cerr << "✗ FAILED: Dimension mismatch\n";
        std::cerr << "  Expected: " << expected_nx << "x" << expected_ny << "x" << expected_nz << "\n";
        std::cerr << "  Got: " << phi_cpu.ni << "x" << phi_cpu.nj << "x" << phi_cpu.nk << "\n";
        return false;
    }

    std::cout << "✓ PASSED: Dimensions match\n";
    std::cout << "  Time: " << cpu_time_ms << " ms\n";

    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "Mode 1 Legacy (dx-based) Test\n";
    std::cout << "========================================\n\n";

    // Load test mesh (OBJ file)
    // Note: Tests run from tests/ directory (WORKING_DIRECTORY in CMakeLists.txt)
    const char* obj_path = argc > 1 ? argv[1] : "./resources/test_x3y4z5_quads.obj";

    std::cout << "Loading test mesh: " << obj_path << "\n";

    std::vector<Vec3f> vertList;
    std::vector<Vec3ui> faceList;
    Vec3f min_box, max_box;

    if (!meshio::load_obj(obj_path, vertList, faceList, min_box, max_box)) {
        std::cerr << "ERROR: Failed to load OBJ file\n";
        return 1;
    }

    test_utils::print_mesh_info(vertList, faceList, min_box, max_box);

    // ========================================
    // Test various dx values
    // ========================================
    std::cout << "\n========================================\n";
    std::cout << "Testing Various dx Values\n";
    std::cout << "========================================\n";

    int32_t failures = 0;
    const int32_t padding = 2;

    // Test different dx values
    // Mesh size is 3x5x4, so these dx values give different resolutions
    const float dx_values[] = {
        0.5f,   // Coarse: ~10x14x12
        0.2f,   // Medium: ~19x29x24
        0.1f,   // Fine: ~34x54x44
        0.05f   // Very fine: ~64x104x84
    };

    for (float dx : dx_values) {
        if (!test_dx_value(dx, padding, vertList, faceList, min_box, max_box)) {
            failures++;
        }
    }

    // ========================================
    // Test different padding values with fixed dx
    // ========================================
    std::cout << "\n========================================\n";
    std::cout << "Testing Various Padding Values\n";
    std::cout << "========================================\n";

    const float fixed_dx = 0.2f;
    const int32_t padding_values[] = {1, 2, 5, 10};

    for (int32_t pad : padding_values) {
        if (!test_dx_value(fixed_dx, pad, vertList, faceList, min_box, max_box)) {
            failures++;
        }
    }

    // ========================================
    // Summary
    // ========================================
    std::cout << "\n========================================\n";
    std::cout << "Mode 1 Legacy Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests run: " << (sizeof(dx_values)/sizeof(dx_values[0]) + sizeof(padding_values)/sizeof(padding_values[0])) << "\n";
    std::cout << "Failures: " << failures << "\n";

    if (failures == 0) {
        std::cout << "✓ ALL MODE 1 LEGACY TESTS PASSED\n";
        std::cout << "  Grid dimensions are correctly calculated from dx\n";
        return 0;
    } else {
        std::cout << "✗ SOME MODE 1 LEGACY TESTS FAILED\n";
        return 1;
    }
}
