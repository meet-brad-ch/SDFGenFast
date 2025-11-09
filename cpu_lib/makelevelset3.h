// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include "array3.h"
#include "vec.h"

namespace sdfgen {
namespace cpu {

/**
 * @brief Generate signed distance field using multi-threaded CPU implementation
 *
 * Computes a 3D signed distance field from a triangle mesh using CPU-based parallel processing.
 * The algorithm operates in two phases: (1) exact distance computation for grid cells within
 * exact_band of any triangle surface, and (2) fast sweeping to propagate distances to far-field
 * regions. The mesh should be closed and manifold for accurate inside/outside signs; triangle
 * soups will produce correct absolute distances but may have incorrect signs. The implementation
 * uses multi-threading to parallelize the computation across multiple CPU cores for improved
 * performance on large grids.
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
 * @param num_threads Number of CPU threads to use, 0 = auto-detect via hardware_concurrency (default: 0)
 *
 * @note Distances within exact_band cells of triangles are computed exactly
 * @note Distances beyond exact_band may not be to the closest triangle but to a nearby one
 * @note Thread count is automatically determined if num_threads=0, using std::thread::hardware_concurrency()
 */
void make_level_set3(const std::vector<Vec3ui> &tri, const std::vector<Vec3f> &x,
                     const Vec3f &origin, float dx, int nx, int ny, int nz,
                     Array3f &phi, const int exact_band=1, int num_threads=0);

} // namespace cpu
} // namespace sdfgen
