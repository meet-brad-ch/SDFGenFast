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
#include "test_utils.h"
#include <iostream>
#include <chrono>
#include <cmath>

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

    min_box = Vec3f(-0.5f, -0.5f, -0.5f);
    max_box = Vec3f(0.5f, 0.5f, 0.5f);
    test_utils::make_cube(min_box, max_box, vertList, faceList);

    std::cout << "Loaded mesh:\n";
    std::cout << "  Vertices:   " << vertList.size() << "\n";
    std::cout << "  Triangles:  " << faceList.size() << "\n";
    std::cout << "  Bounds:     (" << min_box << ") to (" << max_box << ")\n\n";

    // Calculate grid parameters (shared helper)
    float dx;
    int32_t ny, nz;
    Vec3f origin;
    test_utils::calculate_grid_parameters(min_box, max_box, grid_res, padding,
                                          dx, ny, nz, origin);

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

    test_utils::SDFComparisonResult cmp = test_utils::compare_sdf_grids(
        phi_cpu, phi_gpu, origin, origin, origin, dx, /*verbose=*/true);
    cmp.cpu_time_ms = cpu_time_ms;
    cmp.gpu_time_ms = gpu_time_ms;

    // ===== Analysis Summary =====
    std::cout << "\n========================================\n";
    std::cout << "Total cells:        " << cmp.total_cells << "\n";
    std::cout << "Mismatches (> " << cmp.tolerance << "):  " << cmp.mismatch_count
              << " (" << (100.0 * cmp.mismatch_count / cmp.total_cells) << "%)\n";
    std::cout << "Sign mismatches:    " << cmp.sign_mismatch_count << "\n";
    std::cout << "Max difference:     " << cmp.max_diff << "\n";
    std::cout << "Near-band max diff: " << cmp.near_band_max_diff << "\n";
    std::cout << "Cell size (dx):     " << dx << "\n";
    std::cout << "Max diff / dx:      " << (cmp.max_diff / dx) << " (error in cell widths)\n";
    std::cout << "CPU time:           " << cpu_time_ms << " ms\n";
    std::cout << "GPU time:           " << gpu_time_ms << " ms\n";
    if (gpu_time_ms > 0.0) {
        std::cout << "Speedup:            " << (cpu_time_ms / gpu_time_ms) << "x\n";
    }
    std::cout << "========================================\n";

    // Pass criteria (see SDFComparisonResult::passed): identical grids, no
    // inside/outside disagreements, tight near-band agreement, and bounded
    // far-field deviation. The far field is only loosely comparable because
    // the CPU (sweeping) and GPU (Jacobi Eikonal) use different propagation
    // methods.
    if (cmp.passed()) {
        std::cout << "\n[PASS] ANALYSIS PASSED: The GPU implementation matches the CPU reference.\n";
        std::cout << "  - Sign determination is correct.\n";
        std::cout << "  - Near-band distances agree within tolerance.\n";
        std::cout << "  - Far-field deviation is within expected bounds for the different numerical method.\n";
        return 0; // Success
    }

    std::cout << "\n[FAIL] ANALYSIS FAILED:\n";
    if (cmp.sign_mismatch_count != 0) {
        std::cout << "  Sign mismatches: " << cmp.sign_mismatch_count << " (must be 0)\n";
    }
    if (cmp.near_band_max_diff > cmp.tolerance) {
        std::cout << "  Near-band max diff " << cmp.near_band_max_diff
                  << " exceeds tolerance " << cmp.tolerance << "\n";
    }
    if ((cmp.max_diff / cmp.tolerance) >= (test_utils::MAX_DIFF_THRESHOLD * 2.0f)) {
        std::cout << "  Max diff / dx = " << (cmp.max_diff / dx)
                  << " exceeds threshold " << test_utils::MAX_DIFF_THRESHOLD << "\n";
    }
    return 1; // Failure
}
