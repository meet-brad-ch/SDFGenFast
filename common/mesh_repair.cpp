// SDFGen - Signed Distance Field Generator
// Mesh watertightness detection and repair implementation
// Copyright (c) 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#include "mesh_repair.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <tuple>

namespace meshio {

// Edge represented by sorted vertex indices
struct Edge {
    unsigned int v1, v2;
    Edge(unsigned int a, unsigned int b) : v1(std::min(a,b)), v2(std::max(a,b)) {}
    bool operator<(const Edge& o) const {
        if (v1 != o.v1) return v1 < o.v1;
        return v2 < o.v2;
    }
};

// Internal structure for detailed analysis
struct DetailedAnalysis {
    MeshAnalysis summary;
    std::map<Edge, std::vector<int>> edge_triangles;
    std::vector<std::vector<unsigned int>> boundary_loops;
};

static Vec3f cross(const Vec3f& a, const Vec3f& b) {
    return Vec3f(a[1]*b[2] - a[2]*b[1],
                 a[2]*b[0] - a[0]*b[2],
                 a[0]*b[1] - a[1]*b[0]);
}

static float length(const Vec3f& v) {
    return std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

static DetailedAnalysis analyze_mesh_detailed(const std::vector<Vec3f>& vertices,
                                               const std::vector<Vec3ui>& faces) {
    DetailedAnalysis result;
    int numTriangles = faces.size();

    // Count triangles per edge
    for (int t = 0; t < numTriangles; t++) {
        unsigned int i0 = faces[t][0];
        unsigned int i1 = faces[t][1];
        unsigned int i2 = faces[t][2];

        result.edge_triangles[Edge(i0, i1)].push_back(t);
        result.edge_triangles[Edge(i1, i2)].push_back(t);
        result.edge_triangles[Edge(i2, i0)].push_back(t);
    }

    result.summary.total_edges = result.edge_triangles.size();

    // Classify edges and build boundary adjacency
    std::set<unsigned int> boundary_vertices;
    std::map<unsigned int, std::vector<unsigned int>> boundary_adj;

    for (auto& kv : result.edge_triangles) {
        int count = kv.second.size();
        if (count == 1) {
            result.summary.boundary_edges++;
            boundary_vertices.insert(kv.first.v1);
            boundary_vertices.insert(kv.first.v2);
            boundary_adj[kv.first.v1].push_back(kv.first.v2);
            boundary_adj[kv.first.v2].push_back(kv.first.v1);
        } else if (count > 2) {
            result.summary.non_manifold_edges++;
        }
    }

    // Find boundary loops (holes)
    std::set<unsigned int> visited;
    for (unsigned int startV : boundary_vertices) {
        if (visited.count(startV)) continue;

        std::vector<unsigned int> loop;
        unsigned int current = startV;
        unsigned int prev = UINT_MAX;

        while (true) {
            loop.push_back(current);
            visited.insert(current);

            // Find next unvisited boundary neighbor
            unsigned int next = UINT_MAX;
            for (unsigned int adj : boundary_adj[current]) {
                if (adj != prev && (visited.count(adj) == 0 || adj == startV)) {
                    next = adj;
                    break;
                }
            }

            if (next == UINT_MAX || next == startV) break;
            prev = current;
            current = next;
        }

        if (loop.size() >= 3) {
            result.boundary_loops.push_back(loop);
        }
    }

    result.summary.num_holes = result.boundary_loops.size();
    result.summary.is_manifold = (result.summary.non_manifold_edges == 0);
    result.summary.is_watertight = (result.summary.boundary_edges == 0 && result.summary.is_manifold);

    return result;
}

MeshAnalysis analyze_mesh(const std::vector<Vec3f>& vertices,
                          const std::vector<Vec3ui>& faces) {
    return analyze_mesh_detailed(vertices, faces).summary;
}

void print_mesh_analysis(const MeshAnalysis& analysis, bool verbose) {
    std::cout << "\n";
    std::cout << "Mesh Analysis:\n";
    std::cout << "  Total edges:        " << analysis.total_edges << "\n";
    std::cout << "  Boundary edges:     " << analysis.boundary_edges;
    if (analysis.boundary_edges > 0) std::cout << " (holes detected)";
    std::cout << "\n";
    std::cout << "  Non-manifold edges: " << analysis.non_manifold_edges;
    if (analysis.non_manifold_edges > 0) std::cout << " (problem)";
    std::cout << "\n";
    std::cout << "  Number of holes:    " << analysis.num_holes << "\n";
    std::cout << "  Is manifold:        " << (analysis.is_manifold ? "yes" : "NO") << "\n";
    std::cout << "  Is watertight:      " << (analysis.is_watertight ? "yes" : "NO") << "\n";

    if (!analysis.is_watertight) {
        std::cout << "\n  WARNING: Mesh is not watertight. SDF sign determination may be incorrect.\n";
        std::cout << "           Use --fix flag to attempt automatic hole filling.\n";
    }
}

// Simple ear-clipping triangulation for hole filling
static std::vector<Vec3ui> triangulate_hole(const std::vector<unsigned int>& loop,
                                             const std::vector<Vec3f>& vertices) {
    std::vector<Vec3ui> result;
    if (loop.size() < 3) return result;

    std::vector<unsigned int> remaining = loop;

    while (remaining.size() > 3) {
        bool ear_found = false;

        for (size_t i = 0; i < remaining.size(); i++) {
            size_t prev = (i + remaining.size() - 1) % remaining.size();
            size_t next = (i + 1) % remaining.size();

            Vec3f v0 = vertices[remaining[prev]];
            Vec3f v1 = vertices[remaining[i]];
            Vec3f v2 = vertices[remaining[next]];

            // Check if ear (convex)
            Vec3f edge1(v1[0]-v0[0], v1[1]-v0[1], v1[2]-v0[2]);
            Vec3f edge2(v2[0]-v1[0], v2[1]-v1[1], v2[2]-v1[2]);
            Vec3f normal = cross(edge1, edge2);

            if (length(normal) < 1e-10f) continue;  // Degenerate

            // Simple ear - just take it
            result.push_back(Vec3ui(remaining[prev], remaining[i], remaining[next]));
            remaining.erase(remaining.begin() + i);
            ear_found = true;
            break;
        }

        if (!ear_found) {
            // Fallback: just take first three vertices
            result.push_back(Vec3ui(remaining[0], remaining[1], remaining[2]));
            remaining.erase(remaining.begin() + 1);
        }
    }

    // Last triangle
    if (remaining.size() == 3) {
        result.push_back(Vec3ui(remaining[0], remaining[1], remaining[2]));
    }

    return result;
}

int weld_vertices(std::vector<Vec3f>& vertices,
                  std::vector<Vec3ui>& faces,
                  float tolerance) {
    if (tolerance <= 0) return 0;

    std::vector<Vec3f> new_vertices;
    std::vector<int> vertex_map(vertices.size(), -1);

    // Spatial hash grid for fast lookup
    float inv_tol = 1.0f / tolerance;
    std::map<std::tuple<int,int,int>, std::vector<int>> grid;

    int welded = 0;

    for (size_t i = 0; i < vertices.size(); i++) {
        const Vec3f& v = vertices[i];
        int gx = (int)std::floor(v[0] * inv_tol);
        int gy = (int)std::floor(v[1] * inv_tol);
        int gz = (int)std::floor(v[2] * inv_tol);

        // Check neighboring cells for existing vertex
        int found_idx = -1;
        for (int dx = -1; dx <= 1 && found_idx < 0; dx++) {
            for (int dy = -1; dy <= 1 && found_idx < 0; dy++) {
                for (int dz = -1; dz <= 1 && found_idx < 0; dz++) {
                    auto key = std::make_tuple(gx+dx, gy+dy, gz+dz);
                    auto it = grid.find(key);
                    if (it != grid.end()) {
                        for (int idx : it->second) {
                            Vec3f diff(new_vertices[idx][0] - v[0],
                                       new_vertices[idx][1] - v[1],
                                       new_vertices[idx][2] - v[2]);
                            if (length(diff) < tolerance) {
                                found_idx = idx;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (found_idx >= 0) {
            vertex_map[i] = found_idx;
            welded++;
        } else {
            int new_idx = new_vertices.size();
            new_vertices.push_back(v);
            vertex_map[i] = new_idx;
            auto key = std::make_tuple(gx, gy, gz);
            grid[key].push_back(new_idx);
        }
    }

    // Update face indices
    for (auto& face : faces) {
        face[0] = vertex_map[face[0]];
        face[1] = vertex_map[face[1]];
        face[2] = vertex_map[face[2]];
    }

    // Remove degenerate triangles
    std::vector<Vec3ui> valid_faces;
    for (const auto& face : faces) {
        if (face[0] != face[1] && face[1] != face[2] && face[0] != face[2]) {
            valid_faces.push_back(face);
        }
    }

    vertices = std::move(new_vertices);
    faces = std::move(valid_faces);

    return welded;
}

int repair_mesh(std::vector<Vec3f>& vertices,
                std::vector<Vec3ui>& faces,
                float weld_tolerance) {
    // First weld vertices
    if (weld_tolerance > 0) {
        int welded = weld_vertices(vertices, faces, weld_tolerance);
        if (welded > 0) {
            std::cout << "  Welded " << welded << " duplicate vertices\n";
        }
    }

    // Analyze to find holes
    DetailedAnalysis analysis = analyze_mesh_detailed(vertices, faces);

    if (analysis.summary.is_watertight) {
        std::cout << "  Mesh is already watertight, no repair needed\n";
        return 0;
    }

    if (analysis.summary.non_manifold_edges > 0) {
        std::cerr << "  WARNING: Mesh has non-manifold edges, repair may not succeed\n";
    }

    // Fill holes
    int holes_filled = 0;
    for (const auto& loop : analysis.boundary_loops) {
        std::vector<Vec3ui> new_tris = triangulate_hole(loop, vertices);
        for (const auto& tri : new_tris) {
            faces.push_back(tri);
        }
        holes_filled++;
    }

    std::cout << "  Filled " << holes_filled << " holes\n";

    // Verify fix
    MeshAnalysis after = analyze_mesh(vertices, faces);
    if (after.is_watertight) {
        std::cout << "  Mesh is now watertight\n";
    } else {
        std::cout << "  WARNING: Mesh still has " << after.num_holes << " holes after repair\n";
    }

    return holes_filled;
}

} // namespace meshio
