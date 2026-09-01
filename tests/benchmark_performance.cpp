// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// Performance Benchmark: CPU (multi-threaded) vs GPU
// Generates data to back up README performance claims

#include "test_utils.h"
#include "sdfgen_unified.h"
#include "mesh_io.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <chrono>

// Test configuration
struct BenchmarkConfig {
    int grid_size;
    int padding;
    std::string description;
};

// Result storage
struct BenchmarkResult {
    std::string config_name;
    int grid_size;
    int total_cells;
    double cpu_1thread_ms;
    double cpu_10thread_ms;
    double cpu_20thread_ms;
    double cpu_max_thread_ms;
    double gpu_time_ms;
    bool gpu_available;
    int max_threads;
};

void print_header() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "SDFGen Performance Benchmark\n";
    std::cout << "========================================\n";
    std::cout << "\n";
}

void print_result_table(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "Benchmark Results\n";
    std::cout << "========================================\n";
    std::cout << "\n";

    // Table header
    std::cout << std::left;
    std::cout << std::setw(12) << "Grid Size";
    std::cout << std::setw(15) << "Total Cells";
    std::cout << std::setw(15) << "CPU (1)";
    std::cout << std::setw(15) << "CPU (10)";
    std::cout << std::setw(15) << "CPU (20)";
    std::cout << std::setw(15) << ("CPU (" + std::to_string(results[0].max_threads) + ")");
    std::cout << std::setw(15) << "GPU";
    std::cout << "\n";
    std::cout << std::string(102, '-') << "\n";

    for (const auto& result : results) {
        std::cout << std::setw(12) << result.config_name;
        std::cout << std::setw(15) << result.total_cells;

        // CPU timings
        std::cout << std::setw(15) << (std::to_string(static_cast<int>(result.cpu_1thread_ms)) + " ms");
        std::cout << std::setw(15) << (std::to_string(static_cast<int>(result.cpu_10thread_ms)) + " ms");
        std::cout << std::setw(15) << (std::to_string(static_cast<int>(result.cpu_20thread_ms)) + " ms");
        std::cout << std::setw(15) << (std::to_string(static_cast<int>(result.cpu_max_thread_ms)) + " ms");

        // GPU timing
        if (result.gpu_available && result.gpu_time_ms > 0) {
            std::cout << std::setw(15) << (std::to_string(static_cast<int>(result.gpu_time_ms)) + " ms");
        } else {
            std::cout << std::setw(15) << "N/A";
        }

        std::cout << "\n";
    }

    std::cout << "\n";
}

void print_analysis(const std::vector<BenchmarkResult>& results) {
    std::cout << "========================================\n";
    std::cout << "Performance Analysis\n";
    std::cout << "========================================\n";
    std::cout << "\n";

    bool has_gpu = results[0].gpu_available;

    std::cout << "Multi-threading Speedup (vs 1 thread):\n";
    for (const auto& result : results) {
        std::cout << "  " << result.config_name << ":\n";

        double speedup_10 = result.cpu_1thread_ms / result.cpu_10thread_ms;
        double efficiency_10 = (speedup_10 / 10.0) * 100.0;
        std::cout << "    10 threads: " << std::fixed << std::setprecision(1)
                  << speedup_10 << "x (" << efficiency_10 << "% efficiency)\n";

        double speedup_20 = result.cpu_1thread_ms / result.cpu_20thread_ms;
        double efficiency_20 = (speedup_20 / 20.0) * 100.0;
        std::cout << "    20 threads: " << std::fixed << std::setprecision(1)
                  << speedup_20 << "x (" << efficiency_20 << "% efficiency)\n";

        double speedup_max = result.cpu_1thread_ms / result.cpu_max_thread_ms;
        double efficiency_max = (speedup_max / result.max_threads) * 100.0;
        std::cout << "    " << result.max_threads << " threads: " << std::fixed << std::setprecision(1)
                  << speedup_max << "x (" << efficiency_max << "% efficiency)\n";
    }
    std::cout << "\n";

    if (has_gpu) {
        std::cout << "GPU Speedup:\n";
        for (const auto& result : results) {
            if (result.gpu_time_ms > 0) {
                std::cout << "  " << result.config_name << ":\n";

                double speedup_vs_1 = result.cpu_1thread_ms / result.gpu_time_ms;
                std::cout << "    vs 1 thread: " << std::fixed << std::setprecision(1)
                          << speedup_vs_1 << "x\n";

                double speedup_vs_10 = result.cpu_10thread_ms / result.gpu_time_ms;
                std::cout << "    vs 10 threads: " << std::fixed << std::setprecision(1)
                          << speedup_vs_10 << "x\n";

                double speedup_vs_20 = result.cpu_20thread_ms / result.gpu_time_ms;
                std::cout << "    vs 20 threads: " << std::fixed << std::setprecision(1)
                          << speedup_vs_20 << "x\n";

                double speedup_vs_max = result.cpu_max_thread_ms / result.gpu_time_ms;
                std::cout << "    vs " << result.max_threads << " threads: "
                          << std::fixed << std::setprecision(1) << speedup_vs_max << "x\n";
            }
        }
    } else {
        std::cout << "GPU not available - CPU-only results\n";
    }

    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    print_header();

    // Load test mesh
    const char* mesh_file = "resources/test_x3y4z5_bin.stl";
    std::cout << "Loading test mesh: " << mesh_file << "\n\n";

    std::vector<Vec3f> verts;
    std::vector<Vec3ui> faces;
    Vec3f min_box, max_box;

    if (!meshio::load_stl(mesh_file, verts, faces, min_box, max_box)) {
        std::cerr << "ERROR: Failed to load test mesh\n";
        return 1;
    }

    test_utils::print_mesh_info(verts, faces, min_box, max_box);

    // Check hardware
    bool gpu_available = sdfgen::is_gpu_available();
    unsigned int cpu_threads = std::thread::hardware_concurrency();
    if(cpu_threads == 0) cpu_threads = 4; // fallback

    std::cout << "Hardware Detection:\n";
    std::cout << "  CPU Threads: " << cpu_threads << "\n";
    std::cout << "  GPU Available: " << (gpu_available ? "YES" : "NO") << "\n";
    if (gpu_available) {
        std::cout << "  Running CPU (" << cpu_threads << " threads) vs GPU benchmark\n";
    } else {
        std::cout << "  Running CPU-only benchmark (" << cpu_threads << " threads)\n";
    }
    std::cout << "\n";

    // Benchmark configurations
    std::vector<BenchmarkConfig> configs = {
        {64,  2, "64³"},
        {128, 2, "128³"},
        {256, 2, "256³"}
    };

    std::vector<BenchmarkResult> results;

    for (const auto& config : configs) {
        std::cout << "Benchmarking " << config.description << " grid...\n";

        BenchmarkResult result;
        result.config_name = config.description;
        result.grid_size = config.grid_size;
        result.gpu_available = gpu_available;
        result.max_threads = cpu_threads;

        // Calculate grid parameters
        float dx;
        int ny, nz;
        Vec3f origin;
        test_utils::calculate_grid_parameters(min_box, max_box, config.grid_size,
                                              config.padding, dx, ny, nz, origin);

        result.total_cells = config.grid_size * ny * nz;

        // CPU benchmarks at fixed and auto thread counts
        std::cout << "  CPU (1 thread)... " << std::flush;
        Array3f phi_cpu;
        test_utils::generate_sdf_with_timing(faces, verts, origin, dx,
                                             config.grid_size, ny, nz, phi_cpu,
                                             sdfgen::HardwareBackend::CPU,
                                             result.cpu_1thread_ms, 1);
        std::cout << result.cpu_1thread_ms << " ms\n";

        std::cout << "  CPU (10 threads)... " << std::flush;
        test_utils::generate_sdf_with_timing(faces, verts, origin, dx,
                                             config.grid_size, ny, nz, phi_cpu,
                                             sdfgen::HardwareBackend::CPU,
                                             result.cpu_10thread_ms, 10);
        std::cout << result.cpu_10thread_ms << " ms\n";

        std::cout << "  CPU (20 threads)... " << std::flush;
        test_utils::generate_sdf_with_timing(faces, verts, origin, dx,
                                             config.grid_size, ny, nz, phi_cpu,
                                             sdfgen::HardwareBackend::CPU,
                                             result.cpu_20thread_ms, 20);
        std::cout << result.cpu_20thread_ms << " ms\n";

        std::cout << "  CPU (" << cpu_threads << " threads)... " << std::flush;
        test_utils::generate_sdf_with_timing(faces, verts, origin, dx,
                                             config.grid_size, ny, nz, phi_cpu,
                                             sdfgen::HardwareBackend::CPU,
                                             result.cpu_max_thread_ms, 0);
        std::cout << result.cpu_max_thread_ms << " ms\n";

        // GPU benchmark
        if (gpu_available) {
            std::cout << "  GPU... " << std::flush;
            Array3f phi_gpu;
            test_utils::generate_sdf_with_timing(faces, verts, origin, dx,
                                                 config.grid_size, ny, nz,
                                                 phi_gpu, sdfgen::HardwareBackend::GPU,
                                                 result.gpu_time_ms);
            std::cout << result.gpu_time_ms << " ms\n";
        } else {
            result.gpu_time_ms = 0;
        }

        results.push_back(result);
        std::cout << "\n";
    }

    // Print results
    print_result_table(results);
    print_analysis(results);

    std::cout << "========================================\n";
    std::cout << "Benchmark Complete\n";
    std::cout << "========================================\n";
    std::cout << "\n";

    return 0;
}
