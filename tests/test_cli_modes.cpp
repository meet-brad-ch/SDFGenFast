// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// CLI Integration Test: Usage Modes
// Mode 1: OBJ + dx [padding]. Mode 2a: STL + Nx [padding]. Mode 2b: STL + Nx Ny Nz [padding].

#include "cli_test_utils.h"
#include <iostream>
#include <vector>

using namespace cli_test;

int main() {
    std::cout << "CLI Modes Integration Test\n";

    TestConfig config = get_default_test_config();
    const std::string res = config.test_resources_dir;

    const SuccessCase cases[] = {
        {"Mode 1: OBJ + dx", {res + "test_x3y4z5_quads.obj", "0.1", "2"},
         "test_x3y4z5_quads.sdf", 0, 0, 0, {}},
        {"Mode 2a: STL + Nx + padding", {res + "test_x3y4z5_bin.stl", "32", "1"},
         "test_x3y4z5_bin_sdf_32x42x52.sdf", 32, 42, 52, {}},
        {"Mode 2a: STL + Nx (default padding)", {res + "test_x3y4z5_bin.stl", "32"},
         "test_x3y4z5_bin_sdf_32x42x52.sdf", 32, 42, 52, {}},
        {"Mode 2b: STL + Nx/Ny/Nz + padding", {res + "test_x3y4z5_bin.stl", "64", "64", "64", "1"},
         "test_x3y4z5_bin_sdf_64x64x64.sdf", 64, 64, 64, {}},
        {"Mode 2b: STL + Nx/Ny/Nz (default padding)", {res + "test_x3y4z5_bin.stl", "48", "48", "48"},
         "test_x3y4z5_bin_sdf_48x48x48.sdf", 48, 48, 48, {}},
    };

    int32_t failures = 0;
    for (const SuccessCase& c : cases) {
        if (!expect_success(c, config)) failures++;
    }

    return summarize("CLI Modes", failures, 5);
}
