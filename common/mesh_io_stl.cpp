// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// STL file loader for SDFGen
// Supports both binary and ASCII STL formats with automatic detection

#include "mesh_io.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <memory>

namespace meshio {

// Constants for STL file format
constexpr size_t STL_HEADER_SIZE = 80;              // Binary STL header size in bytes
constexpr size_t STL_TRIANGLE_COUNT_SIZE = 4;       // Size of triangle count field (uint32)
constexpr size_t STL_TRIANGLE_SIZE = 50;            // Size of one triangle in binary STL (normal + 3 vertices + attribute)
constexpr size_t STL_NORMAL_SIZE = 12;              // Size of normal vector (3 floats)
constexpr size_t STL_VERTEX_DATA_SIZE = 36;         // Size of 3 vertices (9 floats)
constexpr size_t STL_ATTRIBUTE_SIZE = 2;            // Size of attribute bytes
constexpr size_t LINE_BUFFER_SIZE = 256;            // Typical ASCII STL line length
constexpr int32_t VERTICES_PER_TRIANGLE = 3;        // Number of vertices in a triangle
constexpr size_t MIN_HEADER_BYTES_TO_READ = 5;      // Minimum bytes needed to detect "solid"

// ============================================================================
// Internal helpers
// ============================================================================

enum class STLFormat {
    Binary,
    ASCII,
    Unknown
};

// Detect STL format by examining file header
static STLFormat detect_stl_format(const char* filename) {
    // RAII: file handle automatically managed
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return STLFormat::Unknown;
    }

    // Read first STL_HEADER_SIZE bytes (header for binary, or start of ASCII)
    char header[STL_HEADER_SIZE];
    file.read(header, STL_HEADER_SIZE);
    int64_t bytes_read = file.gcount();

    if (bytes_read < static_cast<int64_t>(MIN_HEADER_BYTES_TO_READ)) {
        return STLFormat::Unknown;
    }

    // Check for ASCII "solid" keyword (case-insensitive)
    int64_t header_len = (bytes_read < static_cast<int64_t>(STL_HEADER_SIZE)) ? bytes_read : static_cast<int64_t>(STL_HEADER_SIZE);
    std::string header_str(header, static_cast<size_t>(header_len));
    std::transform(header_str.begin(), header_str.end(), header_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (header_str.find("solid") == 0) {
        // Could be ASCII, but binary files sometimes have "solid" in header
        // Read the triangle count (next STL_TRIANGLE_COUNT_SIZE bytes after STL_HEADER_SIZE-byte header)
        file.seekg(STL_HEADER_SIZE, std::ios::beg);
        uint32_t num_triangles;
        file.read(reinterpret_cast<char*>(&num_triangles), sizeof(uint32_t));

        if (file.fail()) {
            return STLFormat::ASCII; // Couldn't read binary triangle count
        }

        // Calculate expected file size for binary STL
        // Header + count + triangles
        file.seekg(0, std::ios::end);
        int64_t file_size = file.tellg();
        int64_t expected_size = static_cast<int64_t>(STL_HEADER_SIZE + STL_TRIANGLE_COUNT_SIZE) +
                               (static_cast<int64_t>(num_triangles) * static_cast<int64_t>(STL_TRIANGLE_SIZE));

        if (file_size == expected_size) {
            return STLFormat::Binary;
        }
        else {
            return STLFormat::ASCII;
        }
    }

    // No "solid" keyword - definitely binary
    return STLFormat::Binary;
}

// ============================================================================
// Binary STL loader
// ============================================================================

static bool load_binary_stl(const char* filename,
                            std::vector<Vec3f>& vertList,
                            std::vector<Vec3ui>& faceList,
                            Vec3f& min_box,
                            Vec3f& max_box) {

    // RAII: file automatically closed on scope exit
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "ERROR: Failed to open STL file: " << filename << std::endl;
        return false;
    }

    // Skip STL_HEADER_SIZE-byte header
    file.seekg(STL_HEADER_SIZE, std::ios::beg);

    // Read triangle count
    uint32_t num_triangles;
    file.read(reinterpret_cast<char*>(&num_triangles), sizeof(uint32_t));

    if (file.fail()) {
        std::cerr << "ERROR: Failed to read triangle count from binary STL" << std::endl;
        return false;
    }

    std::cout << "Reading binary STL with " << num_triangles << " triangles..." << std::endl;

    // Clear and reserve space
    vertList.clear();
    faceList.clear();
    vertList.reserve(num_triangles * 3);
    faceList.reserve(num_triangles);

    // Initialize bounding box
    min_box = Vec3f(std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max());
    max_box = Vec3f(std::numeric_limits<float>::lowest(),
                    std::numeric_limits<float>::lowest(),
                    std::numeric_limits<float>::lowest());

    // Read triangles
    for (uint32_t i = 0; i < num_triangles; ++i) {
        // Skip normal (STL_NORMAL_SIZE bytes = 3 floats)
        file.seekg(STL_NORMAL_SIZE, std::ios::cur);

        // Read 3 vertices (9 floats = STL_VERTEX_DATA_SIZE bytes)
        float v[9];
        file.read(reinterpret_cast<char*>(v), STL_VERTEX_DATA_SIZE);

        if (file.fail()) {
            std::cerr << "ERROR: Failed to read triangle " << i << std::endl;
            return false;
        }

        // Skip attribute bytes (STL_ATTRIBUTE_SIZE bytes)
        file.seekg(STL_ATTRIBUTE_SIZE, std::ios::cur);

        // Store vertices and update bounds
        uint32_t idx_base = static_cast<uint32_t>(vertList.size());
        for (int32_t j = 0; j < VERTICES_PER_TRIANGLE; ++j) {
            Vec3f vertex(v[j*VERTICES_PER_TRIANGLE], v[j*VERTICES_PER_TRIANGLE+1], v[j*VERTICES_PER_TRIANGLE+2]);
            vertList.push_back(vertex);
            update_minmax(vertex, min_box, max_box);
        }

        // Add face (3 sequential vertices)
        faceList.push_back(Vec3ui(idx_base, idx_base + 1, idx_base + 2));
    }

    std::cout << "  Loaded " << vertList.size() << " vertices and "
              << faceList.size() << " faces" << std::endl;
    std::cout << "  Bounds: (" << min_box << ") to (" << max_box << ")" << std::endl;

    return true;
}

// ============================================================================
// ASCII STL loader
// ============================================================================

static bool load_ascii_stl(const char* filename,
                           std::vector<Vec3f>& vertList,
                           std::vector<Vec3ui>& faceList,
                           Vec3f& min_box,
                           Vec3f& max_box) {

    // RAII: file automatically closed on scope exit
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "ERROR: Failed to open STL file: " << filename << std::endl;
        return false;
    }

    std::cout << "Reading ASCII STL..." << std::endl;

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

    std::string line;
    line.reserve(LINE_BUFFER_SIZE);

    int32_t triangle_count = 0;
    bool in_solid = false;
    bool in_facet = false;
    bool in_loop = false;
    int32_t vertex_in_facet = 0;
    uint32_t facet_start_idx = 0;

    while (std::getline(file, line)) {
        // Trim leading whitespace
        auto start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue; // Empty line
        line = line.substr(start);

        // Convert to lowercase for keyword matching
        std::string line_lower = line;
        std::transform(line_lower.begin(), line_lower.end(), line_lower.begin(),
                      [](unsigned char c) { return std::tolower(c); });

        if (line_lower.find("solid") == 0) {
            in_solid = true;
        }
        else if (line_lower.find("endsolid") == 0) {
            in_solid = false;
        }
        else if (line_lower.find("facet") == 0) {
            if (!in_solid) {
                std::cerr << "ERROR: 'facet' outside 'solid' block" << std::endl;
                return false;
            }
            in_facet = true;
            vertex_in_facet = 0;
            facet_start_idx = static_cast<uint32_t>(vertList.size());
        }
        else if (line_lower.find("endfacet") == 0) {
            if (vertex_in_facet != VERTICES_PER_TRIANGLE) {
                std::cerr << "ERROR: Facet has " << vertex_in_facet
                         << " vertices (expected " << VERTICES_PER_TRIANGLE << ")" << std::endl;
                return false;
            }
            in_facet = false;
            ++triangle_count;

            // Add face
            faceList.push_back(Vec3ui(facet_start_idx,
                                     facet_start_idx + 1,
                                     facet_start_idx + 2));
        }
        else if (line_lower.find("outer loop") == 0) {
            in_loop = true;
        }
        else if (line_lower.find("endloop") == 0) {
            in_loop = false;
        }
        else if (line_lower.find("vertex") == 0) {
            if (!in_facet || !in_loop) {
                std::cerr << "ERROR: 'vertex' outside facet/loop" << std::endl;
                return false;
            }

            // Parse vertex coordinates
            std::istringstream iss(line);
            std::string keyword;
            Vec3f vertex;
            iss >> keyword >> vertex[0] >> vertex[1] >> vertex[2];

            if (iss.fail()) {
                std::cerr << "ERROR: Failed to parse vertex: " << line << std::endl;
                return false;
            }

            vertList.push_back(vertex);
            update_minmax(vertex, min_box, max_box);
            ++vertex_in_facet;
        }
        // Ignore: "facet normal", comments, etc.
    }

    // Validate results
    if (vertList.empty()) {
        std::cerr << "ERROR: No vertices found in ASCII STL file" << std::endl;
        return false;
    }

    if (faceList.empty()) {
        std::cerr << "ERROR: No faces found in ASCII STL file" << std::endl;
        return false;
    }

    std::cout << "  Loaded " << triangle_count << " triangles ("
              << vertList.size() << " vertices, "
              << faceList.size() << " faces)" << std::endl;
    std::cout << "  Bounds: (" << min_box << ") to (" << max_box << ")" << std::endl;

    return true;
}

// ============================================================================
// Public API
// ============================================================================

bool load_stl(const char* filename,
              std::vector<Vec3f>& vertList,
              std::vector<Vec3ui>& faceList,
              Vec3f& min_box,
              Vec3f& max_box) {

    // Detect format
    STLFormat format = detect_stl_format(filename);

    if (format == STLFormat::Unknown) {
        std::cerr << "ERROR: Could not determine STL format for: " << filename << std::endl;
        return false;
    }

    // Dispatch to appropriate loader
    if (format == STLFormat::Binary) {
        std::cout << "Detected: Binary STL" << std::endl;
        return load_binary_stl(filename, vertList, faceList, min_box, max_box);
    }
    else {
        std::cout << "Detected: ASCII STL" << std::endl;
        return load_ascii_stl(filename, vertList, faceList, min_box, max_box);
    }
}

} // namespace meshio
