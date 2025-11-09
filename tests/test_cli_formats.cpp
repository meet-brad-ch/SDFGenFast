// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// CLI Integration Test: Input Format Support
// Tests binary STL, ASCII STL, OBJ triangulated, OBJ quads

#include "cli_test_utils.h"
#include <iostream>
#include <vector>
#include <string>

using namespace cli_test;

// Test binary STL loading
bool test_binary_stl() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Binary STL Format\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    std::string output_file = config.test_resources_dir + "test_x3y4z5_bin_sdf_32x42x52.sdf";
    delete_file_if_exists(output_file);

    // Run with binary STL
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_bin.stl",
        "32",
        "1"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "Binary STL");

        assert_file_exists(output_file, "Binary STL output");

        SDFFileInfo info = read_sdf_header(output_file);
        assert_sdf_dimensions(info, 32, 42, 52, "Binary STL");

        std::cout << "✓ Binary STL PASSED\n";
        std::cout << "  Dimensions: " << info.nx << "x" << info.ny << "x" << info.nz << "\n";

        delete_file_if_exists(output_file);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists(output_file);
        return false;
    }
}

// Test ASCII STL loading
bool test_ascii_stl() {
    std::cout << "\n========================================\n";
    std::cout << "Testing ASCII STL Format\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    std::string output_file = config.test_resources_dir + "test_x3y4z5_ascii_sdf_32x42x52.sdf";
    delete_file_if_exists(output_file);

    // Run with ASCII STL
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_ascii.stl",
        "32",
        "1"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "ASCII STL");

        assert_file_exists(output_file, "ASCII STL output");

        SDFFileInfo info = read_sdf_header(output_file);
        assert_sdf_dimensions(info, 32, 42, 52, "ASCII STL");

        std::cout << "✓ ASCII STL PASSED\n";
        std::cout << "  Dimensions: " << info.nx << "x" << info.ny << "x" << info.nz << "\n";

        delete_file_if_exists(output_file);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists(output_file);
        return false;
    }
}

// Test OBJ with quads (should be triangulated automatically)
bool test_obj_quads() {
    std::cout << "\n========================================\n";
    std::cout << "Testing OBJ with Quads\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    std::string output_file = config.test_resources_dir + "test_x3y4z5_quads.sdf";
    delete_file_if_exists(output_file);

    // Run with OBJ containing quads (Mode 1: dx-based)
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_quads.obj",
        "0.1",
        "2"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "OBJ quads");

        assert_file_exists(output_file, "OBJ quads output");

        SDFFileInfo info = read_sdf_header(output_file);
        if (!info.valid) {
            std::cerr << "✗ OBJ quads FAILED: Invalid SDF file\n";
            return false;
        }

        std::cout << "✓ OBJ quads PASSED\n";
        std::cout << "  Dimensions: " << info.nx << "x" << info.ny << "x" << info.nz << "\n";
        std::cout << "  (Quads were automatically triangulated)\n";

        delete_file_if_exists(output_file);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists(output_file);
        return false;
    }
}

// Test OBJ with triangles
bool test_obj_triangulated() {
    std::cout << "\n========================================\n";
    std::cout << "Testing OBJ with Triangles\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    std::string output_file = config.test_resources_dir + "test_x3y4z5_triangulated.sdf";
    delete_file_if_exists(output_file);

    // Run with pre-triangulated OBJ (Mode 1: dx-based)
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_triangulated.obj",
        "0.1",
        "2"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "OBJ triangles");

        assert_file_exists(output_file, "OBJ triangles output");

        SDFFileInfo info = read_sdf_header(output_file);
        if (!info.valid) {
            std::cerr << "✗ OBJ triangles FAILED: Invalid SDF file\n";
            return false;
        }

        std::cout << "✓ OBJ triangles PASSED\n";
        std::cout << "  Dimensions: " << info.nx << "x" << info.ny << "x" << info.nz << "\n";

        delete_file_if_exists(output_file);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists(output_file);
        return false;
    }
}

// Test format auto-detection (binary vs ASCII STL)
bool test_stl_auto_detection() {
    std::cout << "\n========================================\n";
    std::cout << "Testing STL Format Auto-Detection\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();

    std::string bin_output = config.test_resources_dir + "test_x3y4z5_bin_sdf_32x42x52.sdf";
    std::string ascii_output = config.test_resources_dir + "test_x3y4z5_ascii_sdf_32x42x52.sdf";

    // Clean up any previous test output
    delete_file_if_exists(bin_output);
    delete_file_if_exists(ascii_output);

    // Test 1: Binary STL should be auto-detected
    std::cout << "\nTesting binary STL auto-detection...\n";
    std::vector<std::string> args_bin = {
        config.test_resources_dir + "test_x3y4z5_bin.stl",
        "32", "1"
    };
    CommandResult result_bin = run_sdfgen(args_bin, config);

    // Test 2: ASCII STL should be auto-detected
    std::cout << "\nTesting ASCII STL auto-detection...\n";
    std::vector<std::string> args_ascii = {
        config.test_resources_dir + "test_x3y4z5_ascii.stl",
        "32", "1"
    };
    CommandResult result_ascii = run_sdfgen(args_ascii, config);

    // Both should succeed
    bool success = true;
    try {
        assert_exit_code(result_bin, 0, "Binary STL auto-detection");
        assert_exit_code(result_ascii, 0, "ASCII STL auto-detection");

        std::cout << "✓ STL Format Auto-Detection PASSED\n";
        std::cout << "  Both binary and ASCII STL were correctly detected\n";

        delete_file_if_exists(bin_output);
        delete_file_if_exists(ascii_output);

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists(bin_output);
        delete_file_if_exists(ascii_output);
        success = false;
    }

    return success;
}

int main() {
    std::cout << "========================================\n";
    std::cout << "CLI Input Formats Integration Test\n";
    std::cout << "========================================\n";

    int32_t failures = 0;

    // Test all input formats
    if (!test_binary_stl()) failures++;
    if (!test_ascii_stl()) failures++;
    if (!test_obj_quads()) failures++;
    if (!test_obj_triangulated()) failures++;
    if (!test_stl_auto_detection()) failures++;

    // Summary
    std::cout << "\n========================================\n";
    std::cout << "CLI Formats Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests run: 5\n";
    std::cout << "Failures: " << failures << "\n";

    if (failures == 0) {
        std::cout << "✓ ALL FORMAT TESTS PASSED\n";
        return 0;
    } else {
        std::cout << "✗ SOME FORMAT TESTS FAILED\n";
        return 1;
    }
}
