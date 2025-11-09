// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include "array3.h"
#include "vec.h"
#include <vector>

namespace sdfgen {

/**
 * @brief Hardware backend selection for SDF generation
 */
enum class HardwareBackend {
    Auto,  /**< Try GPU first, fall back to CPU if unavailable */
    CPU,   /**< Force CPU implementation */
    GPU    /**< Force GPU implementation (fails if CUDA not available) */
};

/**
 * @brief Generate a signed distance field from a triangle mesh
 *
 * Creates a regular 3D grid and computes the signed distance from each grid point to the
 * nearest triangle surface. Negative distances indicate points inside the mesh, positive
 * distances indicate points outside. This is the unified interface that automatically
 * selects between CPU and GPU implementations based on hardware availability and user
 * preference. The function handles grid initialization, exact distance computation near
 * the surface, and fast sweeping for far-field propagation.
 *
 * @param tri Triangle indices (mesh topology), each Vec3ui contains 3 vertex indices
 * @param x Vertex positions (mesh geometry) in world coordinates
 * @param origin Grid origin point in world space (corner of grid)
 * @param dx Grid cell spacing (uniform in all dimensions)
 * @param nx Grid dimension in X (number of cells)
 * @param ny Grid dimension in Y (number of cells)
 * @param nz Grid dimension in Z (number of cells)
 * @param phi Output SDF grid (will be resized to nx*ny*nz)
 * @param exact_band Distance band in cells for exact computation (default: 1)
 * @param backend Hardware selection: Auto, CPU, or GPU (default: Auto)
 * @param num_threads CPU thread count, 0 = auto-detect (only used for CPU backend)
 *
 * @note When backend is Auto, GPU is tried first and falls back to CPU if unavailable
 * @note The exact_band parameter controls accuracy vs performance tradeoff
 */
void make_level_set3(
    const std::vector<Vec3ui>& tri,
    const std::vector<Vec3f>& x,
    const Vec3f& origin,
    float dx,
    int nx, int ny, int nz,
    Array3f& phi,
    int exact_band = 1,
    HardwareBackend backend = HardwareBackend::Auto,
    int num_threads = 0
);

/**
 * @brief Query if GPU acceleration is available at runtime
 *
 * Checks if the library was compiled with CUDA support and if a compatible CUDA GPU
 * is present and accessible on the system. This can be used to determine if the GPU
 * backend option will succeed.
 *
 * @return true if CUDA GPU is available and functional, false otherwise
 */
bool is_gpu_available();

} // namespace sdfgen
