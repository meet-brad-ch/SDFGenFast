// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// CLI Integration Test: Output Generation
// Binary SDF naming, dimension-stamped filenames, overwrite behavior,
// and VTK output when compiled in.

#include "cli_test_utils.h"
#include "config.h"
#include <fstream>
#include <iostream>
#include <vector>

using namespace cli_test;

// SDFGen must replace an existing output file with a valid SDF.
static bool test_file_overwrite(const TestConfig& config) {
    std::cout << "\n--- File overwrite ---\n";

    std::string output_file = config.test_resources_dir + "test_x3y4z5_quads.sdf";
    {
        std::ofstream dummy(output_file);
        dummy << "This is a dummy file that should be overwritten\n";
    }
    int64_t dummy_size = get_file_size(output_file);

    CommandResult result = run_sdfgen(
        {config.test_resources_dir + "test_x3y4z5_quads.obj", "0.1", "2"}, config);

    bool ok = true;
    if (result.exit_code != 0) {
        std::cerr << "[FAIL] File overwrite: exit code " << result.exit_code << "\n";
        ok = false;
    } else {
        SDFFileInfo info = read_sdf_header(output_file);
        if (!info.valid) {
            std::cerr << "[FAIL] File overwrite: output is not a valid SDF\n";
            ok = false;
        } else if (info.file_size == dummy_size) {
            std::cerr << "[FAIL] File overwrite: file was not replaced\n";
            ok = false;
        }
    }
    delete_file_if_exists(output_file);
    if (ok) std::cout << "[PASS] File overwrite\n";
    return ok;
}

// With VTK compiled in, a .vti file is produced instead of .sdf.
static bool test_vtk_output([[maybe_unused]] const TestConfig& config) {
    std::cout << "\n--- VTK output ---\n";
#ifndef HAVE_VTK
    std::cout << "[SKIP] VTK support not compiled in (HAVE_VTK undefined)\n";
    return true;
#else
    std::string vtk_file = config.test_resources_dir + "test_x3y4z5_quads.vti";
    delete_file_if_exists(vtk_file);

    CommandResult result = run_sdfgen(
        {config.test_resources_dir + "test_x3y4z5_quads.obj", "0.1", "2"}, config);

    bool ok = (result.exit_code == 0) && file_exists(vtk_file);
    if (!ok) {
        std::cerr << "[FAIL] VTK output: exit " << result.exit_code
                  << ", file exists: " << file_exists(vtk_file) << "\n";
    } else {
        std::cout << "[PASS] VTK output\n";
    }
    delete_file_if_exists(vtk_file);
    return ok;
#endif
}

int main() {
    std::cout << "CLI Output Generation Test\n";

    TestConfig config = get_default_test_config();
    const std::string res = config.test_resources_dir;

    const SuccessCase cases[] = {
        // OBJ mode: output name mirrors the input name
        {"Binary SDF (OBJ mode)", {res + "test_x3y4z5_quads.obj", "0.1", "2"},
         "test_x3y4z5_quads.sdf", 0, 0, 0, {}},
        // STL mode: dimensions are stamped into the filename
        {"Dimension-stamped filename", {res + "test_x3y4z5_bin.stl", "32", "1"},
         "test_x3y4z5_bin_sdf_32x42x52.sdf", 32, 42, 52, {}},
    };

    int32_t failures = 0;
    int32_t total = 0;
    for (const SuccessCase& c : cases) {
        total++;
        if (!expect_success(c, config)) failures++;
    }
    total++;
    if (!test_file_overwrite(config)) failures++;
    total++;
    if (!test_vtk_output(config)) failures++;

    return summarize("CLI Output Generation", failures, total);
}
