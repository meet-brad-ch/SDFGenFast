// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// Test utilities implementation

#include "test_utils.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>

namespace test_utils {

void generate_sdf_with_timing(
    const std::vector<Vec3ui>& faceList,
    const std::vector<Vec3f>& vertList,
    const Vec3f& origin,
    float dx,
    int32_t nx, int32_t ny, int32_t nz,
    Array3f& phi,
    sdfgen::HardwareBackend backend,
    double& time_ms
) {
    auto start = std::chrono::high_resolution_clock::now();
    sdfgen::make_level_set3(faceList, vertList, origin, dx, nx, ny, nz, phi, 1, backend);
    auto end = std::chrono::high_resolution_clock::now();
    time_ms = std::chrono::duration<double, std::milli>(end - start).count();
}

bool write_sdf_with_validation(
    const char* filename,
    const Array3f& phi,
    const Vec3f& origin,
    float dx,
    int32_t& inside_count
) {
    inside_count = 0;
    return write_sdf_binary(filename, phi, origin, dx, &inside_count);
}

SDFComparisonResult compare_sdf_grids(
    const Array3f& phi_cpu,
    const Array3f& phi_gpu,
    const Vec3f& cpu_origin,
    const Vec3f& gpu_origin,
    const Vec3f& expected_origin,
    float dx,
    bool verbose
) {
    SDFComparisonResult result;
    result.tolerance = dx * 0.5f;

    // Check dimensions
    result.dimensions_match = (phi_cpu.ni == phi_gpu.ni &&
                               phi_cpu.nj == phi_gpu.nj &&
                               phi_cpu.nk == phi_gpu.nk);

    if (!result.dimensions_match) {
        if (verbose) {
            std::cerr << "✗ ERROR: Grid dimensions mismatch!\n";
            std::cerr << "  CPU: " << phi_cpu.ni << " x " << phi_cpu.nj << " x " << phi_cpu.nk << "\n";
            std::cerr << "  GPU: " << phi_gpu.ni << " x " << phi_gpu.nj << " x " << phi_gpu.nk << "\n";
        }
        return result;
    }

    if (verbose) {
        std::cout << "✓ Dimensions match: " << phi_cpu.ni << "x" << phi_cpu.nj << "x" << phi_cpu.nk << "\n";
    }

    // Check bounding boxes
    Vec3f cpu_diff(std::abs(cpu_origin[0] - expected_origin[0]),
                   std::abs(cpu_origin[1] - expected_origin[1]),
                   std::abs(cpu_origin[2] - expected_origin[2]));

    Vec3f gpu_diff(std::abs(gpu_origin[0] - expected_origin[0]),
                   std::abs(gpu_origin[1] - expected_origin[1]),
                   std::abs(gpu_origin[2] - expected_origin[2]));

    result.bbox_match = (cpu_diff[0] < BBOX_TOLERANCE &&
                        cpu_diff[1] < BBOX_TOLERANCE &&
                        cpu_diff[2] < BBOX_TOLERANCE &&
                        gpu_diff[0] < BBOX_TOLERANCE &&
                        gpu_diff[1] < BBOX_TOLERANCE &&
                        gpu_diff[2] < BBOX_TOLERANCE);

    if (!result.bbox_match) {
        if (verbose) {
            std::cerr << "✗ ERROR: Bounding boxes don't match expected values\n";
        }
        return result;
    }

    if (verbose) {
        std::cout << "✓ Bounding boxes match\n";
    }

    // Compare SDF values
    result.total_cells = phi_cpu.ni * phi_cpu.nj * phi_cpu.nk;
    result.mismatch_count = 0;
    result.max_diff = 0.0f;

    for (int32_t k = 0; k < phi_cpu.nk; ++k) {
        for (int32_t j = 0; j < phi_cpu.nj; ++j) {
            for (int32_t i = 0; i < phi_cpu.ni; ++i) {
                float cpu_val = phi_cpu(i, j, k);
                float gpu_val = phi_gpu(i, j, k);
                float diff = std::abs(cpu_val - gpu_val);

                if (diff > result.max_diff) {
                    result.max_diff = diff;
                }

                if (diff > result.tolerance) {
                    if (result.mismatch_count < MAX_MISMATCH_PRINT && verbose) {
                        std::cerr << "  Mismatch at (" << i << "," << j << "," << k << "): "
                                  << "CPU=" << cpu_val << ", GPU=" << gpu_val
                                  << ", diff=" << diff << "\n";
                    }
                    result.mismatch_count++;
                }
            }
        }
    }

    return result;
}

bool test_sdf_io_roundtrip(
    const std::vector<Vec3ui>& faceList,
    const std::vector<Vec3f>& vertList,
    const Vec3f& origin,
    float dx,
    int32_t nx, int32_t ny, int32_t nz,
    const char* cpu_filename,
    const char* gpu_filename,
    SDFComparisonResult& result
) {
    // Generate CPU SDF
    std::cout << "[CPU] Generating SDF...\n";
    Array3f phi_cpu;
    generate_sdf_with_timing(faceList, vertList, origin, dx, nx, ny, nz,
                            phi_cpu, sdfgen::HardwareBackend::CPU, result.cpu_time_ms);

    std::cout << "[CPU] Writing to " << cpu_filename << "...\n";
    if (!write_sdf_with_validation(cpu_filename, phi_cpu, origin, dx, result.cpu_inside_count)) {
        std::cerr << "ERROR: Failed to write CPU SDF file\n";
        return false;
    }
    std::cout << "[CPU] Done. Time: " << result.cpu_time_ms << " ms, Inside cells: "
              << result.cpu_inside_count << "\n\n";

    // Generate GPU SDF (if available)
    bool gpu_available = sdfgen::is_gpu_available();
    Array3f phi_gpu;

    if (gpu_available) {
        std::cout << "[GPU] Generating SDF...\n";
        generate_sdf_with_timing(faceList, vertList, origin, dx, nx, ny, nz,
                                phi_gpu, sdfgen::HardwareBackend::GPU, result.gpu_time_ms);

        std::cout << "[GPU] Writing to " << gpu_filename << "...\n";
        if (!write_sdf_with_validation(gpu_filename, phi_gpu, origin, dx, result.gpu_inside_count)) {
            std::cerr << "ERROR: Failed to write GPU SDF file\n";
            std::remove(cpu_filename);
            return false;
        }
        std::cout << "[GPU] Done. Time: " << result.gpu_time_ms << " ms, Inside cells: "
                  << result.gpu_inside_count << "\n\n";
    } else {
        std::cout << "[GPU] Skipped (GPU not available - CPU-only build or no GPU access)\n\n";
        result.gpu_time_ms = 0.0;
        result.gpu_inside_count = 0;
    }

    // Read files back
    std::cout << "Reading files back...\n";

    Array3f phi_cpu_read;
    Vec3f cpu_origin_read, cpu_max_read;
    if (!read_sdf_binary(cpu_filename, phi_cpu_read, cpu_origin_read, cpu_max_read)) {
        std::cerr << "ERROR: Failed to read CPU SDF file\n";
        std::remove(cpu_filename);
        if (gpu_available) std::remove(gpu_filename);
        return false;
    }
    std::cout << "  CPU file: OK\n";

    if (gpu_available) {
        Array3f phi_gpu_read;
        Vec3f gpu_origin_read, gpu_max_read;
        if (!read_sdf_binary(gpu_filename, phi_gpu_read, gpu_origin_read, gpu_max_read)) {
            std::cerr << "ERROR: Failed to read GPU SDF file\n";
            std::remove(cpu_filename);
            std::remove(gpu_filename);
            return false;
        }
        std::cout << "  GPU file: OK\n\n";

        // Compare grids
        std::cout << "Validating file contents...\n\n";
        result = compare_sdf_grids(phi_cpu_read, phi_gpu_read,
                                   cpu_origin_read, gpu_origin_read,
                                   origin, dx, true);
    } else {
        std::cout << "  GPU file: Skipped\n\n";
        // For CPU-only, just mark as passed if CPU read was successful
        result.dimensions_match = true;
        result.bbox_match = true;
        result.total_cells = phi_cpu_read.ni * phi_cpu_read.nj * phi_cpu_read.nk;
        result.mismatch_count = 0;
        result.max_diff = 0.0f;
        result.tolerance = dx * 0.5f;
    }

    // Store timing info (already set above)
    return true;
}

void print_test_summary(
    const char* test_name,
    const SDFComparisonResult& result
) {
    std::cout << "\n========================================\n";
    std::cout << test_name << " - Test Results\n";
    std::cout << "========================================\n";
    std::cout << "Total cells:        " << result.total_cells << "\n";
    std::cout << "Max difference:     " << result.max_diff
              << " (" << (result.max_diff / result.tolerance * 0.5f) << " cell widths)\n";
    std::cout << "Mismatches (> " << result.tolerance << "): "
              << result.mismatch_count << " ("
              << (100.0 * result.mismatch_count / result.total_cells) << "%)\n";
    std::cout << "CPU inside count:   " << result.cpu_inside_count << "\n";
    std::cout << "GPU inside count:   " << result.gpu_inside_count << "\n";
    std::cout << "CPU time:           " << result.cpu_time_ms << " ms\n";
    std::cout << "GPU time:           " << result.gpu_time_ms << " ms\n";

    if (result.gpu_time_ms > 0) {
        std::cout << "Speedup:            " << (result.cpu_time_ms / result.gpu_time_ms) << "x\n";
    }

    std::cout << "========================================\n\n";

    if (result.passed()) {
        std::cout << "✓ " << test_name << " PASSED\n";
        std::cout << "  - Files written and read successfully\n";
        std::cout << "  - Headers match expected values\n";
        std::cout << "  - SDF data differences are acceptable\n";
    } else {
        std::cout << "✗ " << test_name << " FAILED\n";
        if (result.mismatch_count > 0) {
            std::cout << "  - SDF value differences exceed tolerance\n";
        }
        if (!result.dimensions_match || !result.bbox_match) {
            std::cout << "  - Header validation failed\n";
        }
    }
}

void print_mesh_info(
    const std::vector<Vec3f>& vertList,
    const std::vector<Vec3ui>& faceList,
    const Vec3f& min_box,
    const Vec3f& max_box
) {
    Vec3f mesh_size = max_box - min_box;
    std::cout << "Mesh properties:\n";
    std::cout << "  Vertices:  " << vertList.size() << "\n";
    std::cout << "  Triangles: " << faceList.size() << "\n";
    std::cout << "  Bounds:    (" << min_box << ") to (" << max_box << ")\n";
    std::cout << "  Size:      " << mesh_size << "\n\n";
}

void calculate_grid_parameters(
    const Vec3f& min_box,
    const Vec3f& max_box,
    int32_t target_nx,
    int32_t padding,
    float& dx,
    int32_t& ny,
    int32_t& nz,
    Vec3f& origin
) {
    Vec3f mesh_size = max_box - min_box;

    // Calculate cell size based on target X dimension
    dx = mesh_size[0] / (target_nx - 2 * padding);

    // Calculate Y and Z dimensions to maintain aspect ratio
    ny = static_cast<int32_t>((mesh_size[1] / dx) + 0.5f) + 2 * padding;
    nz = static_cast<int32_t>((mesh_size[2] / dx) + 0.5f) + 2 * padding;

    // Calculate grid size and origin (centered)
    Vec3f grid_size(target_nx * dx, ny * dx, nz * dx);
    Vec3f mesh_center = (min_box + max_box) * 0.5f;
    origin = mesh_center - grid_size * 0.5f;

    std::cout << "Grid parameters:\n";
    std::cout << "  Dimensions: " << target_nx << " x " << ny << " x " << nz << "\n";
    std::cout << "  Total cells: " << (target_nx * ny * nz) << "\n";
    std::cout << "  Cell size:  " << dx << " m\n";
    std::cout << "  Origin:     (" << origin << ")\n\n";
}

} // namespace test_utils
