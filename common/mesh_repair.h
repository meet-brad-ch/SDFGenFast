// SDFGen - Signed Distance Field Generator
// Mesh watertightness detection and repair
// Copyright (c) 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include "vec.h"
#include <vector>
#include <map>
#include <set>

namespace meshio {

/**
 * @brief Results of mesh watertightness analysis
 */
struct MeshAnalysis {
    int total_edges = 0;
    int boundary_edges = 0;      ///< Edges with 1 adjacent triangle (holes)
    int non_manifold_edges = 0;  ///< Edges with >2 adjacent triangles
    int num_holes = 0;
    bool is_manifold = false;
    bool is_watertight = false;
};

/**
 * @brief Analyze mesh for watertightness
 *
 * Checks if mesh is watertight (closed, manifold). A watertight mesh has:
 * - Every edge shared by exactly 2 triangles
 * - No boundary edges (holes)
 * - No non-manifold edges (>2 triangles per edge)
 *
 * @param vertices Vertex positions
 * @param faces Triangle indices (3 indices per triangle)
 * @return MeshAnalysis with detailed results
 */
MeshAnalysis analyze_mesh(const std::vector<Vec3f>& vertices,
                          const std::vector<Vec3ui>& faces);

/**
 * @brief Print mesh analysis results to stdout
 *
 * @param analysis Results from analyze_mesh()
 * @param verbose If true, print detailed hole information
 */
void print_mesh_analysis(const MeshAnalysis& analysis, bool verbose = false);

/**
 * @brief Attempt to repair non-watertight mesh by filling holes
 *
 * Uses ear-clipping triangulation to fill detected holes. Works best
 * for simple planar or near-planar holes. Complex 3D holes may not
 * triangulate perfectly.
 *
 * @param vertices Vertex positions (may be modified if welding needed)
 * @param faces Triangle indices (new triangles appended)
 * @param weld_tolerance Tolerance for welding duplicate vertices (0 = no welding)
 * @return Number of holes filled, or -1 on error
 */
int repair_mesh(std::vector<Vec3f>& vertices,
                std::vector<Vec3ui>& faces,
                float weld_tolerance = 1e-5f);

/**
 * @brief Weld duplicate vertices within tolerance
 *
 * Merges vertices that are within tolerance distance of each other.
 * Updates face indices to reference merged vertices.
 *
 * @param vertices Vertex positions (modified in-place, duplicates removed)
 * @param faces Triangle indices (updated to reference merged vertices)
 * @param tolerance Maximum distance between vertices to merge
 * @return Number of vertices removed by welding
 */
int weld_vertices(std::vector<Vec3f>& vertices,
                  std::vector<Vec3ui>& faces,
                  float tolerance = 1e-5f);

} // namespace meshio
