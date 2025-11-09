// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// CLI Integration Test: Output File Generation
// Tests binary .sdf output, filename generation, and VTK output (if available)

#include "cli_test_utils.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

using namespace cli_test;

// Test binary .sdf output (Mode 1)
bool test_binary_sdf_mode1() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Binary .sdf Output (Mode 1)\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    std::string output_file = config.test_resources_dir + "test_x3y4z5_quads.sdf";
    delete_file_if_exists(output_file);

    // Mode 1: Output filename matches input filename (.obj → .sdf)
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_quads.obj",
        "0.1",
        "2"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "Binary SDF Mode 1");

        assert_file_exists(output_file, "Binary SDF output");

        // Verify it's a valid SDF file
        SDFFileInfo info = read_sdf_header(output_file);
        if (!info.valid) {
            std::cerr << "✗ Binary SDF FAILED: Invalid SDF file format\n";
            return false;
        }

        // Verify file size matches header
        if (info.file_size != info.expected_size) {
            std::cerr << "✗ Binary SDF FAILED: File size mismatch\n";
            std::cerr << "  Expected: " << info.expected_size << " bytes\n";
            std::cerr << "  Actual: " << info.file_size << " bytes\n";
            return false;
        }

        std::cout << "✓ Binary SDF (Mode 1) PASSED\n";
        std::cout << "  File: test_x3y4z5_quads.sdf\n";
        std::cout << "  Size: " << info.file_size << " bytes\n";
        std::cout << "  Dimensions: " << info.nx << "x" << info.ny << "x" << info.nz << "\n";

        delete_file_if_exists(output_file);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists(output_file);
        return false;
    }
}

// Test filename generation with dimensions (Mode 2)
bool test_filename_with_dimensions() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Filename with Dimensions (Mode 2)\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Expected format: <basename>_sdf_<Nx>x<Ny>x<Nz>.sdf
    std::string output_file = config.test_resources_dir + "test_x3y4z5_bin_sdf_32x42x52.sdf";
    delete_file_if_exists(output_file);

    // Mode 2: Output filename includes dimensions
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_bin.stl",
        "32",
        "1"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "Filename with dimensions");

        assert_file_exists(output_file, "Dimensioned filename");

        // Verify dimensions in filename match file contents
        SDFFileInfo info = read_sdf_header(output_file);
        assert_sdf_dimensions(info, 32, 42, 52, "Filename dimensions");

        std::cout << "✓ Filename with Dimensions PASSED\n";
        std::cout << "  Generated: test_x3y4z5_bin_sdf_32x42x52.sdf\n";
        std::cout << "  Dimensions match: " << info.nx << "x" << info.ny << "x" << info.nz << "\n";

        delete_file_if_exists(output_file);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists(output_file);
        return false;
    }
}

// Test file overwrite behavior
bool test_file_overwrite() {
    std::cout << "\n========================================\n";
    std::cout << "Testing File Overwrite Behavior\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    std::string output_file = config.test_resources_dir + "test_x3y4z5_quads.sdf";

    // Create a dummy file first
    {
        std::ofstream dummy(output_file);
        dummy << "This is a dummy file that should be overwritten\n";
    }

    if (!file_exists(output_file)) {
        std::cerr << "✗ File Overwrite FAILED: Could not create dummy file\n";
        return false;
    }

    int64_t dummy_size = get_file_size(output_file);
    std::cout << "Created dummy file: " << dummy_size << " bytes\n";

    // Run SDFGen (should overwrite the dummy file)
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_quads.obj",
        "0.1",
        "2"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "File overwrite");
        assert_file_exists(output_file, "Overwritten file");

        // Verify file was replaced with valid SDF
        SDFFileInfo info = read_sdf_header(output_file);
        if (!info.valid) {
            std::cerr << "✗ File Overwrite FAILED: Output is not a valid SDF\n";
            return false;
        }

        // File size should be different from dummy
        if (info.file_size == dummy_size) {
            std::cerr << "✗ File Overwrite FAILED: File was not overwritten\n";
            return false;
        }

        std::cout << "✓ File Overwrite PASSED\n";
        std::cout << "  File was successfully overwritten with valid SDF\n";
        std::cout << "  New size: " << info.file_size << " bytes\n";

        delete_file_if_exists(output_file);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists(output_file);
        return false;
    }
}

// Test VTK output (conditional on HAVE_VTK)
bool test_vtk_output() {
    std::cout << "\n========================================\n";
    std::cout << "Testing VTK .vti Output (if available)\n";
    std::cout << "========================================\n";

#ifndef HAVE_VTK
    std::cout << "⊘ VTK Support Not Compiled\n";
    std::cout << "  Skipping VTK test (HAVE_VTK not defined)\n";
    return true;  // Not a failure, just skipped
#else
    TestConfig config = get_default_test_config();
    config.verbose = true;

    std::string vtk_file = config.test_resources_dir + "test_x3y4z5_quads.vti";
    std::string sdf_file = config.test_resources_dir + "test_x3y4z5_quads.sdf";

    delete_file_if_exists(vtk_file);
    delete_file_if_exists(sdf_file);

    // VTK output should be generated automatically alongside .sdf
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_quads.obj",
        "0.1",
        "2"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "VTK output");

        assert_file_exists(vtk_file, "VTK .vti output");

        // Basic validation: VTK files start with XML header
        std::ifstream vtk(vtk_file);
        std::string first_line;
        std::getline(vtk, first_line);

        if (!string_starts_with(first_line, "<?xml")) {
            std::cerr << "✗ VTK output FAILED: Not a valid XML file\n";
            return false;
        }

        std::cout << "✓ VTK Output PASSED\n";
        std::cout << "  Generated: test_x3y4z5_quads.vti\n";
        std::cout << "  Valid XML format\n";

        delete_file_if_exists(vtk_file);
        delete_file_if_exists(sdf_file);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists(vtk_file);
        delete_file_if_exists(sdf_file);
        return false;
    }
#endif
}

int main() {
    std::cout << "========================================\n";
    std::cout << "CLI Output File Generation Test\n";
    std::cout << "========================================\n";

    int32_t failures = 0;

    // Test output file generation
    if (!test_binary_sdf_mode1()) failures++;
    if (!test_filename_with_dimensions()) failures++;
    if (!test_file_overwrite()) failures++;
    if (!test_vtk_output()) failures++;

    // Summary
    std::cout << "\n========================================\n";
    std::cout << "CLI Output Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests run: 4\n";
    std::cout << "Failures: " << failures << "\n";

    if (failures == 0) {
        std::cout << "✓ ALL OUTPUT TESTS PASSED\n";
        return 0;
    } else {
        std::cout << "✗ SOME OUTPUT TESTS FAILED\n";
        return 1;
    }
}
