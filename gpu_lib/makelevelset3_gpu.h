// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include "array3.h"
#include "vec.h"

namespace sdfgen {
namespace gpu {

/**
 * @brief Generate signed distance field using GPU-accelerated CUDA implementation
 *
 * Computes a 3D signed distance field from a triangle mesh using CUDA GPU acceleration.
 * The GPU implementation parallelizes across grid cells, with each CUDA thread processing
 * one or more cells. The algorithm mirrors the CPU version with two phases: (1) exact
 * distance computation for cells within exact_band of triangle surfaces, and (2) fast
 * sweeping for far-field distance propagation. The mesh should be closed and manifold
 * for accurate inside/outside determination; triangle soups will have correct absolute
 * distances but may have sign errors. This implementation typically achieves 10-40x
 * speedup over multi-threaded CPU for large grids.
 *
 * @param tri Triangle indices defining mesh topology, each Vec3ui contains 3 vertex indices
 * @param x Vertex positions in world coordinates
 * @param origin Grid origin point (lower corner) in world space
 * @param dx Grid cell spacing, uniform in all dimensions
 * @param nx Number of grid cells in X dimension
 * @param ny Number of grid cells in Y dimension
 * @param nz Number of grid cells in Z dimension
 * @param phi Output signed distance field array (will be resized to nx*ny*nz)
 * @param exact_band Width of exact computation band in grid cells (default: 1)
 *
 * @note Requires CUDA-capable GPU and CUDA runtime libraries
 * @note Results should be numerically identical or nearly identical to CPU version
 * @note Distances within exact_band cells of triangles are computed exactly
 * @note Distances beyond exact_band may not be to the closest triangle
 */
void make_level_set3(const std::vector<Vec3ui> &tri, const std::vector<Vec3f> &x,
                     const Vec3f &origin, float dx, int nx, int ny, int nz,
                     Array3f &phi, const int exact_band=1);

} // namespace gpu
} // namespace sdfgen
