// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include "vec.h"
#include <algorithm>
#include <cmath>

namespace sdfgen {

/**
 * @brief Grid layout computed from mesh bounds and a target resolution
 */
struct GridParameters {
    int nx = 0;      ///< Total grid size in X, including padding cells
    int ny = 0;      ///< Total grid size in Y, including padding cells
    int nz = 0;      ///< Total grid size in Z, including padding cells
    float dx = 0.f;  ///< Uniform cell size
    Vec3f origin;    ///< World position of grid cell (0, 0, 0)
};

/**
 * @brief Size a grid proportionally from a target X dimension
 *
 * The cell size is chosen so the mesh X extent fits nx minus the padding
 * cells. Y and Z dimensions keep the mesh aspect ratio. The grid is
 * centered on the mesh.
 *
 * @param min_box Mesh bounding box minimum
 * @param max_box Mesh bounding box maximum
 * @param nx Total grid size in X, including padding (must exceed 2*padding)
 * @param padding Padding cells on each side
 * @return Complete grid layout
 */
inline GridParameters proportional_grid(const Vec3f& min_box, const Vec3f& max_box,
                                        int nx, int padding) {
    GridParameters g;
    Vec3f mesh_size = max_box - min_box;

    g.nx = nx;
    g.dx = mesh_size[0] / (nx - 2 * padding);
    g.ny = (int)((mesh_size[1] / g.dx) + 0.5f) + 2 * padding;
    g.nz = (int)((mesh_size[2] / g.dx) + 0.5f) + 2 * padding;

    Vec3f grid_size(g.nx * g.dx, g.ny * g.dx, g.nz * g.dx);
    Vec3f mesh_center = (min_box + max_box) * 0.5f;
    g.origin = mesh_center - grid_size * 0.5f;
    return g;
}

/**
 * @brief Size a grid from explicit dimensions
 *
 * The largest per-axis cell size wins so the mesh always fits the
 * requested dimensions with the given padding. The grid is centered on
 * the mesh.
 *
 * @param min_box Mesh bounding box minimum
 * @param max_box Mesh bounding box maximum
 * @param nx Total grid size in X, including padding (must exceed 2*padding)
 * @param ny Total grid size in Y, including padding (must exceed 2*padding)
 * @param nz Total grid size in Z, including padding (must exceed 2*padding)
 * @param padding Padding cells on each side
 * @return Complete grid layout
 */
inline GridParameters manual_grid(const Vec3f& min_box, const Vec3f& max_box,
                                  int nx, int ny, int nz, int padding) {
    GridParameters g;
    Vec3f mesh_size = max_box - min_box;

    g.nx = nx;
    g.ny = ny;
    g.nz = nz;
    float dx_x = mesh_size[0] / (nx - 2 * padding);
    float dx_y = mesh_size[1] / (ny - 2 * padding);
    float dx_z = mesh_size[2] / (nz - 2 * padding);
    g.dx = std::max(dx_x, std::max(dx_y, dx_z));

    Vec3f grid_size(g.nx * g.dx, g.ny * g.dx, g.nz * g.dx);
    Vec3f mesh_center = (min_box + max_box) * 0.5f;
    g.origin = mesh_center - grid_size * 0.5f;
    return g;
}

} // namespace sdfgen
