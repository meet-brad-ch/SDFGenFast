// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// CLI Integration Test: Thread Count Parameter (-t/--threads)

#include "cli_test_utils.h"
#include <iostream>
#include <vector>

using namespace cli_test;

int main() {
    std::cout << "CLI Thread Count Parameter Test\n";

    TestConfig config = get_default_test_config();
    const std::string res = config.test_resources_dir;

    const SuccessCase cases[] = {
        {"OBJ with 1 thread", {res + "test_x3y4z5_quads.obj", "0.1", "2", "-t", "1"},
         "test_x3y4z5_quads.sdf", 0, 0, 0, {"Threads: 1"}},
        {"OBJ with 10 threads", {res + "test_x3y4z5_quads.obj", "0.1", "2", "-t", "10"},
         "test_x3y4z5_quads.sdf", 0, 0, 0, {"Threads: 10"}},
        {"OBJ with auto threads", {res + "test_x3y4z5_quads.obj", "0.1", "2", "-t", "0"},
         "test_x3y4z5_quads.sdf", 0, 0, 0, {"Threads: auto"}},
        {"STL proportional with -p and -t", {res + "test_x3y4z5_bin.stl", "32", "-p", "1", "-t", "5"},
         "test_x3y4z5_bin_sdf_32x42x52.sdf", 32, 42, 52, {"Threads: 5"}},
        {"STL manual dims with -p and -t",
         {res + "test_x3y4z5_bin.stl", "40", "40", "40", "-p", "1", "-t", "8"},
         "test_x3y4z5_bin_sdf_40x40x40.sdf", 40, 40, 40, {"Threads: 8"}},
    };

    int32_t failures = 0;
    for (const SuccessCase& c : cases) {
        if (!expect_success(c, config)) failures++;
    }

    return summarize("CLI Thread Count", failures, 5);
}
