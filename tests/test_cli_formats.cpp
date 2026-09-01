// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// CLI Integration Test: Input Format Support
// Tests binary STL, ASCII STL, OBJ triangulated, OBJ quads

#include "cli_test_utils.h"
#include <iostream>
#include <vector>

using namespace cli_test;

int main() {
    std::cout << "CLI Input Format Test\n";

    TestConfig config = get_default_test_config();
    const std::string res = config.test_resources_dir;

    const SuccessCase cases[] = {
        {"Binary STL", {res + "test_x3y4z5_bin.stl", "32", "1"},
         "test_x3y4z5_bin_sdf_32x42x52.sdf", 32, 42, 52, {}},
        {"ASCII STL", {res + "test_x3y4z5_ascii.stl", "32", "1"},
         "test_x3y4z5_ascii_sdf_32x42x52.sdf", 32, 42, 52, {}},
        {"OBJ triangulated", {res + "test_x3y4z5_triangulated.obj", "0.1", "2"},
         "test_x3y4z5_triangulated.sdf", 0, 0, 0, {}},
        {"OBJ quads (auto-triangulated)", {res + "test_x3y4z5_quads.obj", "0.1", "2"},
         "test_x3y4z5_quads.sdf", 0, 0, 0, {}},
    };

    int32_t failures = 0;
    for (const SuccessCase& c : cases) {
        if (!expect_success(c, config)) failures++;
    }

    return summarize("CLI Input Format", failures, 4);
}
