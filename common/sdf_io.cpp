// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#include "sdf_io.h"
#include <fstream>
#include <iostream>
#include <vector>

bool write_sdf_binary(const std::string& filename,
                      const Array3f& phi_grid,
                      const Vec3f& min_box,
                      float dx,
                      int* out_inside_count) {
    // Open file for binary writing
    std::ofstream outfile(filename.c_str(), std::ios::binary);
    if (!outfile) {
        std::cerr << "ERROR: Failed to open file for writing: " << filename << std::endl;
        return false;
    }

    // Header: dimensions (3 x int32)
    int ni = phi_grid.ni;
    int nj = phi_grid.nj;
    int nk = phi_grid.nk;
    outfile.write(reinterpret_cast<char*>(&ni), sizeof(int));
    outfile.write(reinterpret_cast<char*>(&nj), sizeof(int));
    outfile.write(reinterpret_cast<char*>(&nk), sizeof(int));

    // Header: bounds_min (3 x float32)
    float bounds_min_x = min_box[0];
    float bounds_min_y = min_box[1];
    float bounds_min_z = min_box[2];
    outfile.write(reinterpret_cast<char*>(&bounds_min_x), sizeof(float));
    outfile.write(reinterpret_cast<char*>(&bounds_min_y), sizeof(float));
    outfile.write(reinterpret_cast<char*>(&bounds_min_z), sizeof(float));

    // Header: bounds_max (3 x float32)
    float bounds_max_x = min_box[0] + phi_grid.ni * dx;
    float bounds_max_y = min_box[1] + phi_grid.nj * dx;
    float bounds_max_z = min_box[2] + phi_grid.nk * dx;
    outfile.write(reinterpret_cast<char*>(&bounds_max_x), sizeof(float));
    outfile.write(reinterpret_cast<char*>(&bounds_max_y), sizeof(float));
    outfile.write(reinterpret_cast<char*>(&bounds_max_z), sizeof(float));

    // Data: SDF values as float32
    // Write in C-order (last dimension varies fastest): for(i) for(j) for(k)
    int inside_count = 0;
    for (int i = 0; i < phi_grid.ni; ++i) {
        for (int j = 0; j < phi_grid.nj; ++j) {
            for (int k = 0; k < phi_grid.nk; ++k) {
                float val = static_cast<float>(phi_grid(i, j, k));
                if (val < 0.0f) inside_count++;
                outfile.write(reinterpret_cast<char*>(&val), sizeof(float));
            }
        }
    }

    // Check for write errors
    if (outfile.fail()) {
        std::cerr << "ERROR: Failed to write SDF data to file: " << filename << std::endl;
        outfile.close();
        return false;
    }

    outfile.close();

    // Return inside count if requested
    if (out_inside_count != nullptr) {
        *out_inside_count = inside_count;
    }

    return true;
}

bool read_sdf_binary(const std::string& filename,
                     Array3f& phi_grid,
                     Vec3f& min_box,
                     Vec3f& max_box) {
    // Open file for binary reading
    std::ifstream infile(filename.c_str(), std::ios::binary);
    if (!infile) {
        std::cerr << "ERROR: Failed to open file for reading: " << filename << std::endl;
        return false;
    }

    // Read header: dimensions (3 x int32)
    int ni, nj, nk;
    infile.read(reinterpret_cast<char*>(&ni), sizeof(int));
    infile.read(reinterpret_cast<char*>(&nj), sizeof(int));
    infile.read(reinterpret_cast<char*>(&nk), sizeof(int));

    // Validate dimensions
    if (ni <= 0 || nj <= 0 || nk <= 0) {
        std::cerr << "ERROR: Invalid dimensions in SDF file: "
                  << ni << "x" << nj << "x" << nk << std::endl;
        infile.close();
        return false;
    }

    // Read header: bounds_min (3 x float32)
    float bounds_min_x, bounds_min_y, bounds_min_z;
    infile.read(reinterpret_cast<char*>(&bounds_min_x), sizeof(float));
    infile.read(reinterpret_cast<char*>(&bounds_min_y), sizeof(float));
    infile.read(reinterpret_cast<char*>(&bounds_min_z), sizeof(float));

    // Read header: bounds_max (3 x float32)
    float bounds_max_x, bounds_max_y, bounds_max_z;
    infile.read(reinterpret_cast<char*>(&bounds_max_x), sizeof(float));
    infile.read(reinterpret_cast<char*>(&bounds_max_y), sizeof(float));
    infile.read(reinterpret_cast<char*>(&bounds_max_z), sizeof(float));

    // Check if header reading was successful
    if (infile.fail()) {
        std::cerr << "ERROR: Failed to read SDF file header: " << filename << std::endl;
        infile.close();
        return false;
    }

    // Store bounding box
    min_box = Vec3f(bounds_min_x, bounds_min_y, bounds_min_z);
    max_box = Vec3f(bounds_max_x, bounds_max_y, bounds_max_z);

    // Resize phi_grid to match dimensions
    phi_grid.resize(ni, nj, nk);

    // Read data: SDF values as float32
    // Read in C-order (same order as written): for(i) for(j) for(k)
    for (int i = 0; i < ni; ++i) {
        for (int j = 0; j < nj; ++j) {
            for (int k = 0; k < nk; ++k) {
                float val;
                infile.read(reinterpret_cast<char*>(&val), sizeof(float));
                if (infile.fail()) {
                    std::cerr << "ERROR: Failed to read SDF data at ("
                              << i << "," << j << "," << k << ")" << std::endl;
                    infile.close();
                    return false;
                }
                phi_grid(i, j, k) = val;
            }
        }
    }

    infile.close();
    return true;
}
