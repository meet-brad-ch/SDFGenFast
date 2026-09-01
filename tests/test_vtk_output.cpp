// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// SDF output round-trip test.
//
// VTK (.vti) writing is implemented in the CLI only, so it is exercised
// by the CLI integration tests (test_cli_output). This test validates the
// library-level output path both builds share: generate an SDF, write the
// binary .sdf, read it back, and verify dimensions and values match.

#include "test_utils.h"
#include "mesh_io.h"
#include "sdf_io.h"
#include "config.h"
#include <cmath>
#include <cstdio>
#include <iostream>

int main() {
    std::cout << "========================================\n";
    std::cout << "SDF Output Round-Trip Test\n";
    std::cout << "========================================\n\n";
#ifdef HAVE_VTK
    std::cout << "VTK support: compiled in (.vti writing is CLI-level; see test_cli_output)\n\n";
#else
    std::cout << "VTK support: not compiled in\n\n";
#endif

#ifdef SDFGEN_TEST_RESOURCES_DIR
    const std::string obj_path = std::string(SDFGEN_TEST_RESOURCES_DIR) + "test_x3y4z5_quads.obj";
#else
    const std::string obj_path = "./resources/test_x3y4z5_quads.obj";
#endif
    const int32_t target_nx = 32;
    const int32_t padding = 2;

    std::vector<Vec3f> vertList;
    std::vector<Vec3ui> faceList;
    Vec3f min_box, max_box;
    if (!meshio::load_obj(obj_path.c_str(), vertList, faceList, min_box, max_box)) {
        std::cerr << "ERROR: Failed to load OBJ file\n";
        return 1;
    }

    float dx;
    int32_t ny, nz;
    Vec3f origin;
    test_utils::calculate_grid_parameters(min_box, max_box, target_nx, padding,
                                          dx, ny, nz, origin);

    std::cout << "[1/3] Generating SDF...\n";
    Array3f phi;
    double cpu_time_ms;
    test_utils::generate_sdf_with_timing(faceList, vertList, origin, dx,
                                         target_nx, ny, nz, phi,
                                         sdfgen::HardwareBackend::CPU, cpu_time_ms);

    std::cout << "[2/3] Writing binary .sdf...\n";
    const char* sdf_filename = "test_vtk_output.sdf";
    if (!write_sdf_binary(sdf_filename, phi, origin, dx, nullptr)) {
        std::cerr << "ERROR: Failed to write binary .sdf file\n";
        return 1;
    }

    std::cout << "[3/3] Reading back and verifying...\n";
    Array3f phi_read;
    Vec3f read_min, read_max;
    bool ok = read_sdf_binary(sdf_filename, phi_read, read_min, read_max);
    std::remove(sdf_filename);
    if (!ok) {
        std::cerr << "[FAIL] Could not read the written SDF back\n";
        return 1;
    }
    if (phi_read.ni != phi.ni || phi_read.nj != phi.nj || phi_read.nk != phi.nk) {
        std::cerr << "[FAIL] Dimensions changed in round trip\n";
        return 1;
    }
    for (int32_t k = 0; k < phi.nk; ++k) {
        for (int32_t j = 0; j < phi.nj; ++j) {
            for (int32_t i = 0; i < phi.ni; ++i) {
                if (phi_read(i, j, k) != phi(i, j, k)) {
                    std::cerr << "[FAIL] Value mismatch at (" << i << "," << j
                              << "," << k << ")\n";
                    return 1;
                }
            }
        }
    }

    std::cout << "[PASS] SDF output round trip is lossless\n";
    return 0;
}
