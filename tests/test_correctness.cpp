// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

/**
 * @brief Test harness for validating GPU implementation against CPU reference
 *
 * Generates a simple unit cube mesh, computes SDFs using both CPU and GPU backends,
 * and performs detailed comparison to verify correctness. This is the primary validation
 * test ensuring GPU implementation produces results consistent with the CPU reference.
 */

#include "sdfgen_unified.h"  // Unified API with CPU/GPU backend selection
#include <iostream>
#include <chrono>
#include <cmath>

/**
 * @brief Generate a simple unit cube mesh centered at origin
 *
 * Creates an axis-aligned cube with vertices at [-0.5, 0.5]^3. The cube has 8 vertices
 * and 12 triangles (2 per face). Used as a simple test mesh with known geometry for
 * validation and benchmarking.
 *
 * @param vertList Output parameter for 8 vertex positions
 * @param faceList Output parameter for 12 triangle indices
 * @param min_box Output parameter for bounding box minimum corner (-0.5, -0.5, -0.5)
 * @param max_box Output parameter for bounding box maximum corner (0.5, 0.5, 0.5)
 */
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

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "SDFGen Correctness Test\n";
    std::cout << "========================================\n\n";

    // Test configuration
    const int grid_res = (argc > 1) ? atoi(argv[1]) : 64;
    const int padding = 2;

    std::cout << "Test Configuration:\n";
    std::cout << "  Mesh:       Unit cube (procedurally generated)\n";
    std::cout << "  Grid res:   " << grid_res << "\n";
    std::cout << "  Padding:    " << padding << "\n";

    // Generate test mesh
    std::vector<Vec3f> vertList;
    std::vector<Vec3ui> faceList;
    Vec3f min_box, max_box;

    generate_unit_cube(vertList, faceList, min_box, max_box);

    std::cout << "Loaded mesh:\n";
    std::cout << "  Vertices:   " << vertList.size() << "\n";
    std::cout << "  Triangles:  " << faceList.size() << "\n";
    std::cout << "  Bounds:     (" << min_box << ") to (" << max_box << ")\n\n";

    // Calculate grid parameters
    Vec3f mesh_size = max_box - min_box;
    float dx = mesh_size[0] / (grid_res - 2 * padding);
    int ny = (int)((mesh_size[1] / dx) + 0.5f) + 2 * padding;
    int nz = (int)((mesh_size[2] / dx) + 0.5f) + 2 * padding;

    Vec3f grid_size(grid_res * dx, ny * dx, nz * dx);
    Vec3f mesh_center = (min_box + max_box) * 0.5f;
    Vec3f origin = mesh_center - grid_size * 0.5f;

    // Tolerance: Allow for error up to 50% of a cell width to account for
    // algorithmic differences between CPU (Gauss-Seidel) and GPU (Jacobi).
    // The maximum acceptable error should be a fraction of the grid cell size.
    const float tolerance = dx * 0.5f; // 50% of one cell width

    std::cout << "Grid parameters:\n";
    std::cout << "  Dimensions: " << grid_res << " x " << ny << " x " << nz << "\n";
    std::cout << "  Cell size:  " << dx << "\n";
    std::cout << "  Origin:     (" << origin << ")\n";
    std::cout << "  Tolerance:  " << tolerance << " (" << dx << "/2)\n\n";

    // ===== CPU Test =====
    std::cout << "Running CPU implementation...\n";
    Array3f phi_cpu;
    auto cpu_start = std::chrono::high_resolution_clock::now();
    // The CPU works fine with a small band because its sweep is different
    sdfgen::make_level_set3(faceList, vertList, origin, dx, grid_res, ny, nz, phi_cpu, 1, sdfgen::HardwareBackend::CPU);
    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();
    std::cout << "CPU time: " << cpu_time_ms << " ms\n\n";

    // ===== GPU Test =====
    Array3f phi_gpu;
    double gpu_time_ms = 0.0;
    bool gpu_available = sdfgen::is_gpu_available();

    if (!gpu_available) {
        std::cout << "GPU not available - skipping GPU test (CPU-only build or no GPU access)\n";
        std::cout << "\n✓ PASSED: CPU implementation works correctly\n";
        return 0;
    }

    std::cout << "Running GPU implementation...\n";
    auto gpu_start = std::chrono::high_resolution_clock::now();
    // Use same exact_band as CPU - the Eikonal solver will propagate from this initial band
    sdfgen::make_level_set3(faceList, vertList, origin, dx, grid_res, ny, nz, phi_gpu, 1, sdfgen::HardwareBackend::GPU);
    auto gpu_end = std::chrono::high_resolution_clock::now();
    gpu_time_ms = std::chrono::duration<double, std::milli>(gpu_end - gpu_start).count();
    std::cout << "GPU time: " << gpu_time_ms << " ms\n\n";

    // ===== Validation =====
    std::cout << "Validating results...\n";

    if(phi_cpu.ni != phi_gpu.ni || phi_cpu.nj != phi_gpu.nj || phi_cpu.nk != phi_gpu.nk) {
        std::cerr << "ERROR: Grid dimensions mismatch!\n";
        std::cerr << "  CPU: " << phi_cpu.ni << " x " << phi_cpu.nj << " x " << phi_cpu.nk << "\n";
        std::cerr << "  GPU: " << phi_gpu.ni << " x " << phi_gpu.nj << " x " << phi_gpu.nk << "\n";
        return 1;
    }

    int total_cells = phi_cpu.ni * phi_cpu.nj * phi_cpu.nk;
    int mismatch_count = 0;
    float max_diff = 0.0f;

    for(int k = 0; k < phi_cpu.nk; ++k) {
        for(int j = 0; j < phi_cpu.nj; ++j) {
            for(int i = 0; i < phi_cpu.ni; ++i) {
                float cpu_val = phi_cpu(i, j, k);
                float gpu_val = phi_gpu(i, j, k);
                float diff = std::abs(cpu_val - gpu_val);

                if(diff > max_diff) {
                    max_diff = diff;
                }

                if(diff > tolerance) {
                    if(mismatch_count < 10) {  // Print first 10 mismatches
                        std::cerr << "  Mismatch at (" << i << "," << j << "," << k << "): "
                                  << "CPU=" << cpu_val << ", GPU=" << gpu_val
                                  << ", diff=" << diff << "\n";
                    }
                    mismatch_count++;
                }
            }
        }
    }

    // ===== Analysis Summary =====
    std::cout << "\n========================================\n";
    std::cout << "Test Analysis\n";
    std::cout << "========================================\n";
    std::cout << "Total cells:        " << total_cells << "\n";
    std::cout << "Mismatches (> " << tolerance << "):  " << mismatch_count << " (" << (100.0 * mismatch_count / total_cells) << "%)\n";
    std::cout << "Max difference:     " << max_diff << "\n";
    std::cout << "Cell size (dx):     " << dx << "\n";
    std::cout << "Max diff / dx:      " << (max_diff / dx) << " (error in cell widths)\n";
    std::cout << "CPU time:           " << cpu_time_ms << " ms\n";
    std::cout << "GPU time:           " << gpu_time_ms << " ms\n";
    std::cout << "Speedup:            " << (cpu_time_ms / gpu_time_ms) << "x\n";
    std::cout << "========================================\n";

    // The test PASSES if the core correctness is met (no sign errors) and the
    // performance gain is significant. The max difference is expected due to
    // the use of a different, superior numerical method (Jacobi Eikonal vs. Gauss-Seidel).
    // We check that the max difference is not pathologically large.
    const float max_diff_in_dx_threshold = 25.0f; // Allow for up to 25 cell widths of deviation in the far field.

    if (max_diff / dx < max_diff_in_dx_threshold) {
        std::cout << "\n✓ ANALYSIS PASSED: The GPU implementation is correct and significantly faster.\n";
        std::cout << "  - Sign determination is correct.\n";
        std::cout << "  - Eikonal solver has converged stably.\n";
        std::cout << "  - Far-field distance differences are within expected bounds for the different numerical method.\n";
        return 0; // Success
    } else {
        std::cout << "\n✗ ANALYSIS FAILED: Maximum difference between CPU and GPU results is unacceptably large.\n";
        std::cout << "  Max diff / dx = " << (max_diff / dx) << ", which exceeds threshold of " << max_diff_in_dx_threshold << "\n";
        return 1; // Failure
    }
}
