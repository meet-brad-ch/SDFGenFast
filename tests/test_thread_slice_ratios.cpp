// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// Test for thread count vs slice count edge cases
// Validates that multi-threading works correctly when:
// - threads > slices (more threads than work)
// - threads == slices (equal distribution)
// - threads < slices (less threads than work)

#include "test_utils.h"
#include "sdfgen_unified.h"
#include "mesh_io.h"
#include <iostream>
#include <vector>

void test_threads_greater_than_slices() {
    std::cout << "========================================\n";
    std::cout << "Test: threads > slices\n";
    std::cout << "========================================\n";

    // Create a SMALL grid (10x10x10) with MANY threads (24)
    // k_range will be ~8, threads=24, so threads > slices
    std::cout << "Grid: 10x10x10, Threads: 24\n";
    std::cout << "Expected: threads (24) > k-slices (~8)\n\n";

    const char* mesh_file = "resources/test_x3y4z5_quads.obj";
    std::vector<Vec3f> verts;
    std::vector<Vec3ui> faces;
    Vec3f min_box, max_box;

    if(!meshio::load_obj(mesh_file, verts, faces, min_box, max_box)) {
        std::cerr << "ERROR: Failed to load test mesh\n";
        exit(1);
    }

    test_utils::print_mesh_info(verts, faces, min_box, max_box);

    int grid_size = 10;
    int padding = 1;
    float dx;
    int ny, nz;
    Vec3f origin;
    test_utils::calculate_grid_parameters(min_box, max_box, grid_size,
                                          padding, dx, ny, nz, origin);

    std::cout << "Grid parameters:\n";
    std::cout << "  Dimensions: " << grid_size << " x " << ny << " x " << nz << "\n";
    std::cout << "  K-range for sweeping: " << (nz - 2) << " slices\n";
    std::cout << "  Using 24 threads\n\n";

    Array3f phi;
    try {
        sdfgen::make_level_set3(faces, verts, origin, dx, grid_size, ny, nz,
                               phi, 1, sdfgen::HardwareBackend::CPU, 24);
        std::cout << "✓ PASSED: No crash with threads > slices\n\n";
    } catch(const std::exception& e) {
        std::cerr << "✗ FAILED: Exception thrown: " << e.what() << "\n";
        exit(1);
    }
}

void test_threads_equal_to_slices() {
    std::cout << "========================================\n";
    std::cout << "Test: threads == slices\n";
    std::cout << "========================================\n";

    // Create a grid where k_range == threads
    // Grid 20x20x22 gives k_range = 20, threads=20
    std::cout << "Grid: 20x20x22, Threads: 20\n";
    std::cout << "Expected: threads (20) == k-slices (20)\n\n";

    const char* mesh_file = "resources/test_x3y4z5_quads.obj";
    std::vector<Vec3f> verts;
    std::vector<Vec3ui> faces;
    Vec3f min_box, max_box;

    if(!meshio::load_obj(mesh_file, verts, faces, min_box, max_box)) {
        std::cerr << "ERROR: Failed to load test mesh\n";
        exit(1);
    }

    int nx = 20, ny = 20, nz = 22;
    int padding = 1;
    float dx = 0.2f;
    Vec3f origin = min_box - Vec3f(padding * dx, padding * dx, padding * dx);

    std::cout << "Grid parameters:\n";
    std::cout << "  Dimensions: " << nx << " x " << ny << " x " << nz << "\n";
    std::cout << "  K-range for sweeping: " << (nz - 2) << " slices\n";
    std::cout << "  Using 20 threads\n\n";

    Array3f phi;
    try {
        sdfgen::make_level_set3(faces, verts, origin, dx, nx, ny, nz,
                               phi, 1, sdfgen::HardwareBackend::CPU, 20);
        std::cout << "✓ PASSED: No crash with threads == slices\n\n";
    } catch(const std::exception& e) {
        std::cerr << "✗ FAILED: Exception thrown: " << e.what() << "\n";
        exit(1);
    }
}

void test_threads_less_than_slices() {
    std::cout << "========================================\n";
    std::cout << "Test: threads < slices\n";
    std::cout << "========================================\n";

    // Create a LARGE grid with FEW threads
    // Grid 64x64x64 gives k_range = 62, threads=8
    std::cout << "Grid: 64x64x64, Threads: 8\n";
    std::cout << "Expected: threads (8) < k-slices (62)\n\n";

    const char* mesh_file = "resources/test_x3y4z5_quads.obj";
    std::vector<Vec3f> verts;
    std::vector<Vec3ui> faces;
    Vec3f min_box, max_box;

    if(!meshio::load_obj(mesh_file, verts, faces, min_box, max_box)) {
        std::cerr << "ERROR: Failed to load test mesh\n";
        exit(1);
    }

    test_utils::print_mesh_info(verts, faces, min_box, max_box);

    int grid_size = 64;
    int padding = 1;
    float dx;
    int ny, nz;
    Vec3f origin;
    test_utils::calculate_grid_parameters(min_box, max_box, grid_size,
                                          padding, dx, ny, nz, origin);

    std::cout << "Grid parameters:\n";
    std::cout << "  Dimensions: " << grid_size << " x " << ny << " x " << nz << "\n";
    std::cout << "  K-range for sweeping: " << (nz - 2) << " slices\n";
    std::cout << "  Using 8 threads\n\n";

    Array3f phi;
    try {
        sdfgen::make_level_set3(faces, verts, origin, dx, grid_size, ny, nz,
                               phi, 1, sdfgen::HardwareBackend::CPU, 8);
        std::cout << "✓ PASSED: No crash with threads < slices\n\n";
    } catch(const std::exception& e) {
        std::cerr << "✗ FAILED: Exception thrown: " << e.what() << "\n";
        exit(1);
    }
}

void test_extreme_cases() {
    std::cout << "========================================\n";
    std::cout << "Test: Extreme cases\n";
    std::cout << "========================================\n";

    const char* mesh_file = "resources/test_x3y4z5_quads.obj";
    std::vector<Vec3f> verts;
    std::vector<Vec3ui> faces;
    Vec3f min_box, max_box;

    if(!meshio::load_obj(mesh_file, verts, faces, min_box, max_box)) {
        std::cerr << "ERROR: Failed to load test mesh\n";
        exit(1);
    }

    // Test 1: Tiny grid (5x5x5) with many threads (100)
    std::cout << "Subtest 1: Grid 5x5x5 with 100 threads\n";
    {
        int nx = 5, ny = 5, nz = 5;
        float dx = 1.0f;
        Vec3f origin = min_box;
        Array3f phi;

        try {
            sdfgen::make_level_set3(faces, verts, origin, dx, nx, ny, nz,
                                   phi, 1, sdfgen::HardwareBackend::CPU, 100);
            std::cout << "  ✓ PASSED\n\n";
        } catch(const std::exception& e) {
            std::cerr << "  ✗ FAILED: " << e.what() << "\n";
            exit(1);
        }
    }

    // Test 2: Single thread
    std::cout << "Subtest 2: Grid 32x32x32 with 1 thread\n";
    {
        int grid_size = 32;
        int padding = 1;
        float dx;
        int ny, nz;
        Vec3f origin;
        test_utils::calculate_grid_parameters(min_box, max_box, grid_size,
                                              padding, dx, ny, nz, origin);
        Array3f phi;

        try {
            sdfgen::make_level_set3(faces, verts, origin, dx, grid_size, ny, nz,
                                   phi, 1, sdfgen::HardwareBackend::CPU, 1);
            std::cout << "  ✓ PASSED\n\n";
        } catch(const std::exception& e) {
            std::cerr << "  ✗ FAILED: " << e.what() << "\n";
            exit(1);
        }
    }

    // Test 3: Zero threads (should auto-detect)
    std::cout << "Subtest 3: Grid 32x32x32 with 0 threads (auto-detect)\n";
    {
        int grid_size = 32;
        int padding = 1;
        float dx;
        int ny, nz;
        Vec3f origin;
        test_utils::calculate_grid_parameters(min_box, max_box, grid_size,
                                              padding, dx, ny, nz, origin);
        Array3f phi;

        try {
            sdfgen::make_level_set3(faces, verts, origin, dx, grid_size, ny, nz,
                                   phi, 1, sdfgen::HardwareBackend::CPU, 0);
            std::cout << "  ✓ PASSED\n\n";
        } catch(const std::exception& e) {
            std::cerr << "  ✗ FAILED: " << e.what() << "\n";
            exit(1);
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "Thread/Slice Ratio Edge Case Tests\n";
    std::cout << "========================================\n";
    std::cout << "Validates multi-threading with various\n";
    std::cout << "thread-to-slice ratios\n";
    std::cout << "========================================\n\n";

    test_threads_greater_than_slices();
    test_threads_equal_to_slices();
    test_threads_less_than_slices();
    test_extreme_cases();

    std::cout << "========================================\n";
    std::cout << "Thread/Slice Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "✓ ALL THREAD/SLICE RATIO TESTS PASSED\n";
    std::cout << "  - threads > slices: OK\n";
    std::cout << "  - threads == slices: OK\n";
    std::cout << "  - threads < slices: OK\n";
    std::cout << "  - Extreme cases: OK\n";
    std::cout << "========================================\n";

    return 0;
}
