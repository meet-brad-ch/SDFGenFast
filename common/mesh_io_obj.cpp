// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// OBJ file loader for SDFGen
// Supports Wavefront OBJ format with triangular faces only

#include "mesh_io.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdint>

namespace meshio {

// Constants
constexpr size_t LINE_BUFFER_SIZE = 256;  // Typical OBJ line length

bool load_obj(const char* filename,
              std::vector<Vec3f>& vertList,
              std::vector<Vec3ui>& faceList,
              Vec3f& min_box,
              Vec3f& max_box) {

    // RAII: ifstream automatically closes file on scope exit
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "ERROR: Failed to open OBJ file: " << filename << std::endl;
        return false;
    }

    std::cout << "Reading OBJ file: " << filename << std::endl;

    // Clear output containers
    vertList.clear();
    faceList.clear();

    // Initialize bounding box
    min_box = Vec3f(std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max());
    max_box = Vec3f(std::numeric_limits<float>::lowest(),
                    std::numeric_limits<float>::lowest(),
                    std::numeric_limits<float>::lowest());

    int32_t ignored_lines = 0;
    std::string line;
    line.reserve(LINE_BUFFER_SIZE);

    while (std::getline(infile, line)) {
        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        // Parse line based on first character(s)
        if (line[0] == 'v') {
            if (line.size() >= 2 && line[1] == 'n') {
                // Vertex normal - skip (not needed for SDF generation)
                ++ignored_lines;
                continue;
            }
            else if (line.size() >= 2 && line[1] == 't') {
                // Texture coordinate - skip
                ++ignored_lines;
                continue;
            }
            else if (line[1] == ' ' || line[1] == '\t') {
                // Vertex position
                std::istringstream data(line);
                char c;
                Vec3f point;
                data >> c >> point[0] >> point[1] >> point[2];

                if (data.fail()) {
                    std::cerr << "WARNING: Failed to parse vertex: " << line << std::endl;
                    continue;
                }

                vertList.push_back(point);
                update_minmax(point, min_box, max_box);
            }
        }
        else if (line[0] == 'f' && (line[1] == ' ' || line[1] == '\t')) {
            // Face - can be v, v/vt, v/vt/vn, or v//vn format
            // Can be triangles or quads
            std::istringstream data(line);
            char c;
            data >> c; // Skip 'f'

            std::vector<int32_t> vertices;
            std::string vertex_str;

            // Read all vertex entries
            while (data >> vertex_str) {
                // Parse v/vt/vn format - extract only vertex index
                size_t first_slash = vertex_str.find('/');
                std::string v_str = (first_slash != std::string::npos)
                                   ? vertex_str.substr(0, first_slash)
                                   : vertex_str;

                int32_t v_idx = std::stoi(v_str);
                vertices.push_back(v_idx);
            }

            if (vertices.size() < 3) {
                std::cerr << "WARNING: Face has < 3 vertices: " << line << std::endl;
                continue;
            }

            // Convert to 0-based indexing and create triangles
            // Triangulate quads if needed (simple fan triangulation)
            for (size_t i = 1; i + 1 < vertices.size(); ++i) {
                faceList.push_back(Vec3ui(
                    static_cast<uint32_t>(vertices[0] - 1),
                    static_cast<uint32_t>(vertices[i] - 1),
                    static_cast<uint32_t>(vertices[i + 1] - 1)
                ));
            }
        }
        else if (line[0] == '#') {
            // Comment - ignore
            ++ignored_lines;
        }
        else {
            // Other lines (materials, groups, etc.) - ignore
            ++ignored_lines;
        }
    }

    // RAII: infile automatically closed here

    // Validate results
    if (vertList.empty()) {
        std::cerr << "ERROR: No vertices found in OBJ file" << std::endl;
        return false;
    }

    if (faceList.empty()) {
        std::cerr << "ERROR: No faces found in OBJ file" << std::endl;
        return false;
    }

    // Print summary
    if (ignored_lines > 0) {
        std::cout << "  Note: " << ignored_lines
                  << " lines ignored (comments, materials, etc.)" << std::endl;
    }

    std::cout << "  Loaded " << vertList.size() << " vertices and "
              << faceList.size() << " faces" << std::endl;
    std::cout << "  Bounds: (" << min_box << ") to (" << max_box << ")" << std::endl;

    return true;
}

} // namespace meshio
