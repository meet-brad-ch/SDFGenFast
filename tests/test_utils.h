// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include "sdfgen_unified.h"
#include "sdf_io.h"
#include <string>
#include <cstdint>

/**
 * @brief Test utilities for SDFGen test suite
 *
 * Provides common functions to eliminate code duplication across tests including
 * SDF generation with timing, file I/O validation, grid comparison, and mesh utilities.
 */

namespace test_utils {

/// Tolerance for bounding box comparisons in world coordinates
constexpr float BBOX_TOLERANCE = 1e-5f;
/// Maximum acceptable difference threshold measured in cell widths
constexpr float MAX_DIFF_THRESHOLD = 25.0f;
/// Maximum number of mismatch details to print during comparison
constexpr int32_t MAX_MISMATCH_PRINT = 5;

/**
 * @brief Result structure for comparing two SDF grids
 *
 * Contains metrics and validation results from comparing CPU and GPU SDF implementations.
 * Used to verify that both backends produce consistent results within acceptable tolerances.
 */
struct SDFComparisonResult {
    bool dimensions_match = false;        ///< Grid dimensions are identical (nx, ny, nz)
    bool bbox_match = false;              ///< Bounding boxes match within BBOX_TOLERANCE
    int32_t total_cells = 0;              ///< Total number of grid cells compared
    int32_t mismatch_count = 0;           ///< Number of cells exceeding tolerance
    float max_diff = 0.0f;                ///< Maximum absolute difference found
    float tolerance = 0.0f;               ///< Cell spacing (dx) used as base tolerance

    double cpu_time_ms = 0.0;             ///< CPU execution time in milliseconds
    double gpu_time_ms = 0.0;             ///< GPU execution time in milliseconds
    int32_t cpu_inside_count = 0;         ///< Count of negative (inside) cells on CPU
    int32_t gpu_inside_count = 0;         ///< Count of negative (inside) cells on GPU

    /**
     * @brief Check if comparison passed all validation criteria
     *
     * @return true if dimensions match, bbox matches, and max difference is within acceptable bounds
     */
    bool passed() const {
        return dimensions_match && bbox_match &&
               (max_diff / tolerance) < (MAX_DIFF_THRESHOLD * 2.0f);
    }
};

/**
 * @brief Generate SDF with timing measurement
 *
 * Generates a signed distance field using the specified backend and measures execution time.
 * Used for performance benchmarking and comparing CPU vs GPU implementations.
 *
 * @param faceList Triangle indices (mesh topology)
 * @param vertList Vertex positions (mesh geometry)
 * @param origin Grid origin point in world space
 * @param dx Grid cell spacing (uniform in all dimensions)
 * @param nx Grid dimension in X
 * @param ny Grid dimension in Y
 * @param nz Grid dimension in Z
 * @param phi Output SDF grid (will be resized)
 * @param backend Hardware backend selection (CPU or GPU)
 * @param time_ms Output parameter for execution time in milliseconds
 */
void generate_sdf_with_timing(
    const std::vector<Vec3ui>& faceList,
    const std::vector<Vec3f>& vertList,
    const Vec3f& origin,
    float dx,
    int32_t nx, int32_t ny, int32_t nz,
    Array3f& phi,
    sdfgen::HardwareBackend backend,
    double& time_ms
);

/**
 * @brief Write SDF to file and measure inside cell count
 *
 * Writes an SDF grid to binary file format and counts the number of cells with negative
 * distance values (inside the mesh). Used for validation and statistics collection.
 *
 * @param filename Output file path
 * @param phi SDF grid to write
 * @param origin Grid origin point
 * @param dx Grid cell spacing
 * @param inside_count Output parameter for count of cells with negative values
 * @return true if file was written successfully, false otherwise
 */
bool write_sdf_with_validation(
    const char* filename,
    const Array3f& phi,
    const Vec3f& origin,
    float dx,
    int32_t& inside_count
);

/**
 * @brief Compare two SDF grids with detailed reporting
 *
 * Performs comprehensive comparison of CPU and GPU SDF grids including dimension validation,
 * bounding box comparison, and cell-by-cell difference analysis. Reports mismatches and
 * provides detailed statistics for test validation.
 *
 * @param phi_cpu CPU-generated SDF grid
 * @param phi_gpu GPU-generated SDF grid
 * @param cpu_origin CPU grid origin
 * @param gpu_origin GPU grid origin
 * @param expected_origin Expected origin for validation
 * @param dx Cell spacing (used as tolerance base)
 * @param verbose If true, prints detailed mismatch information (default: true)
 * @return SDFComparisonResult structure with comparison metrics and pass/fail status
 */
SDFComparisonResult compare_sdf_grids(
    const Array3f& phi_cpu,
    const Array3f& phi_gpu,
    const Vec3f& cpu_origin,
    const Vec3f& gpu_origin,
    const Vec3f& expected_origin,
    float dx,
    bool verbose = true
);

/**
 * @brief Test write/read roundtrip for both CPU and GPU
 *
 * Comprehensive test that generates SDFs using both CPU and GPU backends, writes them to files,
 * reads them back, and performs detailed comparison. Validates file I/O correctness and
 * backend consistency in a single integrated test.
 *
 * @param faceList Triangle indices
 * @param vertList Vertex positions
 * @param origin Grid origin
 * @param dx Cell spacing
 * @param nx Grid dimension in X
 * @param ny Grid dimension in Y
 * @param nz Grid dimension in Z
 * @param cpu_filename Output file path for CPU SDF
 * @param gpu_filename Output file path for GPU SDF
 * @param result Output parameter with detailed comparison results
 * @return true if test passed (SDFs match within tolerance), false otherwise
 */
bool test_sdf_io_roundtrip(
    const std::vector<Vec3ui>& faceList,
    const std::vector<Vec3f>& vertList,
    const Vec3f& origin,
    float dx,
    int32_t nx, int32_t ny, int32_t nz,
    const char* cpu_filename,
    const char* gpu_filename,
    SDFComparisonResult& result
);

/**
 * @brief Print formatted test summary
 *
 * Outputs comprehensive test results including timing, cell counts, mismatch statistics,
 * and pass/fail status in a readable format.
 *
 * @param test_name Name of the test being reported
 * @param result Comparison results to display
 */
void print_test_summary(
    const char* test_name,
    const SDFComparisonResult& result
);

/**
 * @brief Print mesh information
 *
 * Displays mesh statistics including vertex/triangle counts and bounding box dimensions.
 * Useful for test output and debugging.
 *
 * @param vertList Vertex positions
 * @param faceList Triangle indices
 * @param min_box Minimum corner of bounding box
 * @param max_box Maximum corner of bounding box
 */
void print_mesh_info(
    const std::vector<Vec3f>& vertList,
    const std::vector<Vec3ui>& faceList,
    const Vec3f& min_box,
    const Vec3f& max_box
);

/**
 * @brief Calculate grid parameters from mesh bounds
 *
 * Computes grid dimensions and origin from mesh bounding box using proportional sizing.
 * Given a target X dimension and padding, calculates Y and Z dimensions that maintain
 * the mesh aspect ratio, and adjusts origin to include padding.
 *
 * @param min_box Minimum corner of mesh bounding box
 * @param max_box Maximum corner of mesh bounding box
 * @param target_nx Desired grid dimension in X
 * @param padding Number of padding cells to add on each side
 * @param dx Output parameter for calculated cell spacing
 * @param ny Output parameter for calculated Y dimension
 * @param nz Output parameter for calculated Z dimension
 * @param origin Output parameter for grid origin (min_box - padding * dx)
 */
void calculate_grid_parameters(
    const Vec3f& min_box,
    const Vec3f& max_box,
    int32_t target_nx,
    int32_t padding,
    float& dx,
    int32_t& ny,
    int32_t& nz,
    Vec3f& origin
);

} // namespace test_utils
