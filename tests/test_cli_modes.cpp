// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// CLI Integration Test: Usage Modes
// Tests all three CLI usage modes according to documentation

#include "cli_test_utils.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstdio>

using namespace cli_test;

// Test Mode 1: Legacy OBJ with dx spacing
// Usage: SDFGen <file.obj> <dx> <padding>
bool test_mode1_obj_dx() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Mode 1: OBJ + dx spacing\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Cleanup any existing output
    delete_file_if_exists("test_x3y4z5_quads.sdf");

    // Run: SDFGen test_x3y4z5_quads.obj 0.1 2
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_quads.obj",
        "0.1",
        "2"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        // Should succeed (exit code 0)
        assert_exit_code(result, 0, "Mode 1: OBJ + dx");

        // Mode 1 writes output to same directory as input file
        std::string output_file = config.test_resources_dir + "test_x3y4z5_quads.sdf";
        assert_file_exists(output_file, "Mode 1: output file");

        // Validate SDF file
        SDFFileInfo info = read_sdf_header(output_file);
        if (!info.valid) {
            std::cerr << "✗ Mode 1 FAILED: Invalid SDF file\n";
            return false;
        }

        std::cout << "✓ Mode 1 PASSED\n";
        std::cout << "  Output dimensions: " << info.nx << "x" << info.ny << "x" << info.nz << "\n";

        // Cleanup
        delete_file_if_exists(output_file);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists("test_x3y4z5_quads.sdf");
        return false;
    }
}

// Test Mode 2a: STL with proportional dimensions
// Usage: SDFGen <file.stl> <Nx> [padding]
bool test_mode2a_stl_nx() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Mode 2a: STL + Nx (proportional)\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Cleanup any existing output
    delete_file_if_exists("test_x3y4z5_bin_sdf_32x42x52.sdf");

    // Run: SDFGen test_x3y4z5_bin.stl 32 1
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_bin.stl",
        "32",
        "1"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        // Should succeed (exit code 0)
        assert_exit_code(result, 0, "Mode 2a: STL + Nx");

        // Mode 2 writes output to same directory as input file
        std::string expected_output = config.test_resources_dir + "test_x3y4z5_bin_sdf_32x42x52.sdf";

        assert_file_exists(expected_output, "Mode 2a: output file with dimensions");

        // Validate SDF file dimensions
        SDFFileInfo info = read_sdf_header(expected_output);
        assert_sdf_dimensions(info, 32, 42, 52, "Mode 2a: dimensions");

        std::cout << "✓ Mode 2a PASSED\n";

        // Cleanup
        delete_file_if_exists(expected_output);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists("test_x3y4z5_bin_sdf_32x42x52.sdf");
        return false;
    }
}

// Test Mode 2a with default padding
// Usage: SDFGen <file.stl> <Nx>
bool test_mode2a_stl_nx_default_padding() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Mode 2a: STL + Nx (default padding)\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Cleanup
    delete_file_if_exists("test_x3y4z5_bin_sdf_32x42x52.sdf");

    // Run: SDFGen test_x3y4z5_bin.stl 32 (no padding specified, should default to 1)
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_bin.stl",
        "32"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "Mode 2a: default padding");

        // Should create same output as with padding=1
        std::string expected_output = config.test_resources_dir + "test_x3y4z5_bin_sdf_32x42x52.sdf";
        assert_file_exists(expected_output, "Mode 2a: default padding output");

        std::cout << "✓ Mode 2a (default padding) PASSED\n";

        delete_file_if_exists(expected_output);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists("test_x3y4z5_bin_sdf_32x42x52.sdf");
        return false;
    }
}

// Test Mode 2b: STL with manual dimensions
// Usage: SDFGen <file.stl> <Nx> <Ny> <Nz> [padding]
bool test_mode2b_stl_manual_dims() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Mode 2b: STL + Nx/Ny/Nz (manual)\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Cleanup
    delete_file_if_exists("test_x3y4z5_bin_sdf_64x64x64.sdf");

    // Run: SDFGen test_x3y4z5_bin.stl 64 64 64 2
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_bin.stl",
        "64",
        "64",
        "64",
        "2"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "Mode 2b: STL + Nx/Ny/Nz");

        // Should create file with specified dimensions in name
        std::string expected_output = config.test_resources_dir + "test_x3y4z5_bin_sdf_64x64x64.sdf";
        assert_file_exists(expected_output, "Mode 2b: output file");

        // Validate dimensions match what we specified
        SDFFileInfo info = read_sdf_header(expected_output);
        assert_sdf_dimensions(info, 64, 64, 64, "Mode 2b: dimensions");

        std::cout << "✓ Mode 2b PASSED\n";

        delete_file_if_exists(expected_output);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists("test_x3y4z5_bin_sdf_64x64x64.sdf");
        return false;
    }
}

// Test Mode 2b with default padding
// Usage: SDFGen <file.stl> <Nx> <Ny> <Nz>
bool test_mode2b_stl_manual_dims_default_padding() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Mode 2b: STL + Nx/Ny/Nz (default padding)\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Cleanup
    delete_file_if_exists("test_x3y4z5_bin_sdf_48x48x48.sdf");

    // Run: SDFGen test_x3y4z5_bin.stl 48 48 48
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_bin.stl",
        "48",
        "48",
        "48"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "Mode 2b: default padding");

        std::string expected_output = config.test_resources_dir + "test_x3y4z5_bin_sdf_48x48x48.sdf";
        assert_file_exists(expected_output, "Mode 2b: default padding output");

        SDFFileInfo info = read_sdf_header(expected_output);
        assert_sdf_dimensions(info, 48, 48, 48, "Mode 2b: default padding dimensions");

        std::cout << "✓ Mode 2b (default padding) PASSED\n";

        delete_file_if_exists(expected_output);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists("test_x3y4z5_bin_sdf_48x48x48.sdf");
        return false;
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << "CLI Modes Integration Test\n";
    std::cout << "========================================\n";

    int32_t failures = 0;

    // Test all modes
    if (!test_mode1_obj_dx()) failures++;
    if (!test_mode2a_stl_nx()) failures++;
    if (!test_mode2a_stl_nx_default_padding()) failures++;
    if (!test_mode2b_stl_manual_dims()) failures++;
    if (!test_mode2b_stl_manual_dims_default_padding()) failures++;

    // Summary
    std::cout << "\n========================================\n";
    std::cout << "CLI Modes Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests run: 5\n";
    std::cout << "Failures: " << failures << "\n";

    if (failures == 0) {
        std::cout << "✓ ALL CLI MODES TESTS PASSED\n";
        return 0;
    } else {
        std::cout << "✗ SOME CLI MODES TESTS FAILED\n";
        return 1;
    }
}
