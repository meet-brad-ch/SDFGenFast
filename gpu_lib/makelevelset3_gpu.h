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
 * Computes a 3D signed distance field from a triangle mesh on a CUDA GPU.
 * The kernel assigns grid cells to CUDA threads. The algorithm mirrors the
 * CPU version and has two phases: exact distance computation for cells within
 * exact_band of a triangle, then iterative sweeping to propagate distances to
 * the far field. The mesh should be closed and manifold for correct
 * inside/outside signs. A triangle soup gets correct absolute distances, but
 * its signs can be wrong. Measured speedups over the CPU path are in
 * README.md, Appendix A; the advantage grows with grid size.
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
 * @note Requires a CUDA-capable GPU and the CUDA runtime; throws std::runtime_error on CUDA errors
 * @note Agreement with the CPU path, as enforced by test_correctness: identical
 *       inside/outside sign for every cell, and distances within dx/2 of the CPU
 *       result for cells within 2 cells of the surface. Far-field cells can
 *       differ more, because the two backends propagate distances in different
 *       orders.
 * @note Distances within exact_band cells of triangles are computed exactly
 * @note Distances beyond exact_band may not be to the closest triangle
 */
void make_level_set3(const std::vector<Vec3ui> &tri, const std::vector<Vec3f> &x,
                     const Vec3f &origin, float dx, int nx, int ny, int nz,
                     Array3f &phi, const int exact_band=1);

} // namespace gpu
} // namespace sdfgen
