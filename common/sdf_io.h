// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include "array3.h"
#include "vec.h"
#include <string>

/**
 * @brief Write a signed distance field to a binary file
 *
 * Binary format (little-endian):
 * - Header (36 bytes):
 *   - 3 x int32: Grid dimensions (Nx, Ny, Nz)
 *   - 3 x float32: Bounding box minimum (x, y, z)
 *   - 3 x float32: Bounding box maximum (x, y, z)
 * - Data (Nx*Ny*Nz x float32):
 *   - SDF values in C-order: for(i) for(j) for(k) write(value)
 *   - Negative values = inside mesh
 *   - Positive values = outside mesh
 *   - Zero = surface
 *
 * @param filename Output file path
 * @param phi_grid SDF grid data (Array3f from make_level_set3)
 * @param min_box Minimum corner of bounding box
 * @param dx Grid cell spacing
 * @param out_inside_count Optional output: number of cells with negative SDF (inside mesh)
 * @return true on success, false on error
 */
bool write_sdf_binary(const std::string& filename,
                      const Array3f& phi_grid,
                      const Vec3f& min_box,
                      float dx,
                      int* out_inside_count = nullptr);

/**
 * @brief Read a signed distance field from a binary file
 *
 * Binary format (little-endian) - same as write_sdf_binary():
 * - Header (36 bytes):
 *   - 3 x int32: Grid dimensions (Nx, Ny, Nz)
 *   - 3 x float32: Bounding box minimum (x, y, z)
 *   - 3 x float32: Bounding box maximum (x, y, z)
 * - Data (Nx*Ny*Nz x float32):
 *   - SDF values in C-order: for(i) for(j) for(k) read(value)
 *
 * @param filename Input file path
 * @param phi_grid Output SDF grid data (will be resized)
 * @param min_box Output minimum corner of bounding box
 * @param max_box Output maximum corner of bounding box
 * @return true on success, false on error
 */
bool read_sdf_binary(const std::string& filename,
                     Array3f& phi_grid,
                     Vec3f& min_box,
                     Vec3f& max_box);

