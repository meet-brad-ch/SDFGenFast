// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// Mesh I/O utility functions

#include "mesh_io.h"
#include <algorithm>
#include <cctype>
#include <iostream>

namespace meshio {

std::string get_extension(const std::string& filename) {
    auto dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }

    std::string ext = filename.substr(dot_pos);

    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return ext;
}

bool load_mesh(const char* filename,
               std::vector<Vec3f>& vertList,
               std::vector<Vec3ui>& faceList,
               Vec3f& min_box,
               Vec3f& max_box) {

    std::string ext = get_extension(filename);

    if (ext == ".obj") {
        return load_obj(filename, vertList, faceList, min_box, max_box);
    }
    else if (ext == ".stl") {
        return load_stl(filename, vertList, faceList, min_box, max_box);
    }
    else {
        std::cerr << "ERROR: Unsupported file format: " << ext << std::endl;
        std::cerr << "       Supported formats: .obj, .stl" << std::endl;
        return false;
    }
}

} // namespace meshio
