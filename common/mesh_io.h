// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include "vec.h"
#include <vector>
#include <string>

// Mesh I/O library for SDFGen
// Supports OBJ and STL (both binary and ASCII) formats

namespace meshio {

// ============================================================================
// Public API
// ============================================================================

/**
 * @brief Load triangle mesh from Wavefront OBJ file
 *
 * Parses a Wavefront OBJ file and extracts vertex positions and face definitions.
 * Supports triangular faces (f v1 v2 v3) and quad faces (f v1 v2 v3 v4), with quads
 * automatically triangulated. Also computes the axis-aligned bounding box of the mesh.
 * Texture coordinates and vertex normals are ignored. Lines starting with # are treated
 * as comments.
 *
 * @param filename Path to OBJ file to load
 * @param vertList Output vector of vertex positions
 * @param faceList Output vector of triangle indices (3 vertex indices per triangle)
 * @param min_box Output minimum corner of axis-aligned bounding box
 * @param max_box Output maximum corner of axis-aligned bounding box
 * @return true on successful load, false on file error or parse failure
 */
bool load_obj(const char* filename,
              std::vector<Vec3f>& vertList,
              std::vector<Vec3ui>& faceList,
              Vec3f& min_box,
              Vec3f& max_box);

/**
 * @brief Load triangle mesh from STL file with automatic format detection
 *
 * Reads an STL (STereoLithography) file and extracts triangle mesh data. Automatically
 * detects whether the file is in binary or ASCII format by examining the file header.
 * Binary STL files have a fixed 80-byte header followed by triangle count and triangle
 * data. ASCII STL files begin with "solid" keyword. The function handles both formats
 * transparently and computes the mesh bounding box. Note that STL files may contain
 * duplicate vertices (no vertex sharing), which are preserved in the output.
 *
 * @param filename Path to STL file to load
 * @param vertList Output vector of vertex positions (may contain duplicates)
 * @param faceList Output vector of triangle indices (3 vertex indices per triangle)
 * @param min_box Output minimum corner of axis-aligned bounding box
 * @param max_box Output maximum corner of axis-aligned bounding box
 * @return true on successful load, false on file error or parse failure
 */
bool load_stl(const char* filename,
              std::vector<Vec3f>& vertList,
              std::vector<Vec3ui>& faceList,
              Vec3f& min_box,
              Vec3f& max_box);

/**
 * @brief Load triangle mesh with automatic format detection from file extension
 *
 * Generic mesh loading function that determines the file format from the filename
 * extension and calls the appropriate loader (load_obj or load_stl). Supported
 * extensions are .obj for Wavefront OBJ files and .stl for STereoLithography files.
 * Extension matching is case-insensitive. This is the recommended high-level
 * function for loading meshes when the format is not known in advance.
 *
 * @param filename Path to mesh file (.obj or .stl)
 * @param vertList Output vector of vertex positions
 * @param faceList Output vector of triangle indices (3 vertex indices per triangle)
 * @param min_box Output minimum corner of axis-aligned bounding box
 * @param max_box Output maximum corner of axis-aligned bounding box
 * @return true on successful load, false if format unsupported or load failed
 */
bool load_mesh(const char* filename,
               std::vector<Vec3f>& vertList,
               std::vector<Vec3ui>& faceList,
               Vec3f& min_box,
               Vec3f& max_box);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Update axis-aligned bounding box to include a point
 *
 * Expands the bounding box (min_box, max_box) as necessary to include the given point.
 * Compares each coordinate component and updates min/max values independently.
 *
 * @param point 3D point to include in bounding box
 * @param min_box Minimum corner of bounding box (updated in-place)
 * @param max_box Maximum corner of bounding box (updated in-place)
 */
inline void update_minmax(const Vec3f& point, Vec3f& min_box, Vec3f& max_box) {
    min_box[0] = std::min(min_box[0], point[0]);
    min_box[1] = std::min(min_box[1], point[1]);
    min_box[2] = std::min(min_box[2], point[2]);
    max_box[0] = std::max(max_box[0], point[0]);
    max_box[1] = std::max(max_box[1], point[1]);
    max_box[2] = std::max(max_box[2], point[2]);
}

/**
 * @brief Extract file extension from filename and convert to lowercase
 *
 * Extracts the substring after the last period (.) in the filename and converts
 * it to lowercase for case-insensitive format detection. Returns empty string if
 * no extension is found.
 *
 * @param filename File path or name to extract extension from
 * @return File extension in lowercase (without the dot), or empty string if none
 */
std::string get_extension(const std::string& filename);

} // namespace meshio
