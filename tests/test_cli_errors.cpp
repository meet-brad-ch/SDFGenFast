// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// CLI Integration Test: Error Handling
// Invalid inputs must fail with a non-zero exit code and a clear message.

#include "cli_test_utils.h"
#include <fstream>
#include <iostream>
#include <vector>

using namespace cli_test;

// Writes content to a temporary file, runs the failure case, cleans up.
static bool expect_failure_with_temp_file(const std::string& temp_name,
                                          const std::string& content,
                                          FailureCase c,
                                          const TestConfig& config) {
    {
        std::ofstream f(temp_name, std::ios::binary);
        f << content;
    }
    c.args.insert(c.args.begin(), temp_name);
    bool ok = expect_failure(c, config);
    delete_file_if_exists(temp_name);
    return ok;
}

int main() {
    std::cout << "CLI Error Handling Test\n";

    TestConfig config = get_default_test_config();
    const std::string res = config.test_resources_dir;

    int32_t failures = 0;
    int32_t total = 0;

    const FailureCase simple_cases[] = {
        {"No arguments", {}, {"Usage", "usage"}},
        {"Missing grid dimensions", {res + "test_x3y4z5_quads.obj"}, {}},
        {"Missing input file",
         {"nonexistent_file_that_does_not_exist.obj", "0.1", "2"},
         {"Failed", "failed", "ERROR", "error", "does not exist"}},
        {"Negative grid dimension", {res + "test_x3y4z5_bin.stl", "-32", "1"}, {}},
        {"Zero grid dimension", {res + "test_x3y4z5_bin.stl", "0", "1"}, {}},
        {"Non-numeric grid dimension",
         {res + "test_x3y4z5_bin.stl", "not_a_number", "1"}, {}},
    };
    for (const FailureCase& c : simple_cases) {
        total++;
        if (!expect_failure(c, config)) failures++;
    }

    // Files that exist but hold garbage
    total++;
    if (!expect_failure_with_temp_file(
            "test_invalid.txt", "This is not a valid mesh file\n",
            {"Unsupported file extension", {"0.1", "2"}, {}}, config)) {
        failures++;
    }
    total++;
    if (!expect_failure_with_temp_file(
            "malformed.stl", "INVALID STL DATA",
            {"Malformed STL file", {"32", "1"}, {}}, config)) {
        failures++;
    }
    total++;
    if (!expect_failure_with_temp_file(
            "malformed.obj", "# This OBJ has no geometry\n# No vertices, no faces\n",
            {"OBJ without geometry", {"0.1", "2"}, {}}, config)) {
        failures++;
    }

    // Negative padding is auto-corrected to the minimum of 1, not an error.
    total++;
    SuccessCase negative_padding = {
        "Negative padding auto-corrects to 1",
        {res + "test_x3y4z5_quads.obj", "0.1", "-2"},
        "test_x3y4z5_quads.sdf", 0, 0, 0, {"Padding: 1"}};
    if (!expect_success(negative_padding, config)) failures++;

    return summarize("CLI Error Handling", failures, total);
}
