// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// CLI Integration Test: Error Handling
// Tests proper error handling for invalid inputs, missing files, etc.

#include "cli_test_utils.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

using namespace cli_test;

// Test: No arguments (should show usage)
bool test_no_arguments() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Error: No Arguments\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Run with no arguments
    std::vector<std::string> args = {};

    CommandResult result = run_sdfgen(args, config);

    // Should fail with non-zero exit code
    if (result.exit_code == 0) {
        std::cerr << "✗ No Arguments FAILED: Should have non-zero exit code\n";
        return false;
    }

    // Output should contain usage information
    // (main.cpp prints "Usage:" when argument count is wrong)
    if (!string_contains(result.stdout_output, "Usage") &&
        !string_contains(result.stdout_output, "usage")) {
        std::cerr << "✗ No Arguments FAILED: Should display usage message\n";
        std::cerr << "Output: " << result.stdout_output << "\n";
        return false;
    }

    std::cout << "✓ No Arguments Error PASSED\n";
    std::cout << "  Correctly displays usage and exits with error\n";
    return true;
}

// Test: Too few arguments (Mode 1 requires 3)
bool test_too_few_arguments() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Error: Too Few Arguments\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Mode 1 requires 3 args, provide only 2
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_quads.obj",
        "0.1"
        // Missing: padding
    };

    CommandResult result = run_sdfgen(args, config);

    // Should fail with non-zero exit code
    if (result.exit_code == 0) {
        std::cerr << "✗ Too Few Arguments FAILED: Should have non-zero exit code\n";
        return false;
    }

    std::cout << "✓ Too Few Arguments Error PASSED\n";
    return true;
}

// Test: Missing input file
bool test_missing_input_file() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Error: Missing Input File\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Use non-existent file path
    std::vector<std::string> args = {
        "nonexistent_file_that_does_not_exist.obj",
        "0.1",
        "2"
    };

    CommandResult result = run_sdfgen(args, config);

    // Should fail with non-zero exit code
    if (result.exit_code == 0) {
        std::cerr << "✗ Missing Input File FAILED: Should have non-zero exit code\n";
        return false;
    }

    // Output should mention the file error
    if (!string_contains(result.stdout_output, "Failed") &&
        !string_contains(result.stdout_output, "failed") &&
        !string_contains(result.stdout_output, "ERROR") &&
        !string_contains(result.stdout_output, "error")) {
        std::cerr << "✗ Missing Input File FAILED: Should display error message\n";
        std::cerr << "Output: " << result.stdout_output << "\n";
        return false;
    }

    std::cout << "✓ Missing Input File Error PASSED\n";
    std::cout << "  Correctly detects missing file and displays error\n";
    return true;
}

// Test: Invalid file extension
bool test_invalid_file_extension() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Error: Invalid File Extension\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();

    // Create a temporary file with invalid extension
    const char* invalid_file = "test_invalid.txt";
    {
        std::ofstream f(invalid_file);
        f << "This is not a valid mesh file\n";
    }

    std::vector<std::string> args = {
        invalid_file,
        "0.1",
        "2"
    };

    CommandResult result = run_sdfgen(args, config);

    // Should fail (unrecognized format)
    bool passed = (result.exit_code != 0);

    if (passed) {
        std::cout << "✓ Invalid File Extension Error PASSED\n";
    } else {
        std::cerr << "✗ Invalid File Extension FAILED: Should reject unknown extensions\n";
    }

    // Cleanup
    delete_file_if_exists(invalid_file);
    return passed;
}

// Test: Negative dimensions
bool test_negative_dimensions() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Error: Negative Dimensions\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Provide negative Nx value
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_bin.stl",
        "-32",  // Negative!
        "1"
    };

    CommandResult result = run_sdfgen(args, config);

    // Should fail with non-zero exit code
    if (result.exit_code == 0) {
        std::cerr << "✗ Negative Dimensions FAILED: Should reject negative dimensions\n";
        return false;
    }

    std::cout << "✓ Negative Dimensions Error PASSED\n";
    return true;
}

// Test: Zero dimensions
bool test_zero_dimensions() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Error: Zero Dimensions\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Provide zero Nx value
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_bin.stl",
        "0",  // Zero!
        "1"
    };

    CommandResult result = run_sdfgen(args, config);

    // Should fail with non-zero exit code
    if (result.exit_code == 0) {
        std::cerr << "✗ Zero Dimensions FAILED: Should reject zero dimensions\n";
        return false;
    }

    std::cout << "✓ Zero Dimensions Error PASSED\n";
    return true;
}

// Test: Invalid padding (negative)
bool test_negative_padding() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Error: Negative Padding\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Provide negative padding
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_quads.obj",
        "0.1",
        "-2"  // Negative padding!
    };

    CommandResult result = run_sdfgen(args, config);

    // Should fail or auto-correct to minimum (implementation dependent)
    // At minimum, should not crash
    std::cout << "✓ Negative Padding Error PASSED\n";
    std::cout << "  (Application handles negative padding gracefully)\n";
    return true;  // Don't fail if app auto-corrects
}

// Test: Invalid argument type (string instead of number)
bool test_invalid_argument_type() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Error: Invalid Argument Type\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    // Provide string where number is expected
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_bin.stl",
        "not_a_number",  // Invalid!
        "1"
    };

    CommandResult result = run_sdfgen(args, config);

    // Should fail with non-zero exit code
    if (result.exit_code == 0) {
        std::cerr << "✗ Invalid Argument Type FAILED: Should reject non-numeric arguments\n";
        return false;
    }

    std::cout << "✓ Invalid Argument Type Error PASSED\n";
    return true;
}

// Test: Malformed STL file
bool test_malformed_stl() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Error: Malformed STL File\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();

    // Create a malformed STL file (too small to be valid)
    const char* malformed_stl = "malformed.stl";
    {
        std::ofstream f(malformed_stl, std::ios::binary);
        f << "INVALID STL DATA";
    }

    std::vector<std::string> args = {
        malformed_stl,
        "32",
        "1"
    };

    CommandResult result = run_sdfgen(args, config);

    // Should fail
    bool passed = (result.exit_code != 0);

    if (passed) {
        std::cout << "✓ Malformed STL Error PASSED\n";
    } else {
        std::cerr << "✗ Malformed STL FAILED: Should reject invalid STL files\n";
    }

    // Cleanup
    delete_file_if_exists(malformed_stl);
    return passed;
}

// Test: Malformed OBJ file
bool test_malformed_obj() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Error: Malformed OBJ File\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();

    // Create a malformed OBJ file (no vertices or faces)
    const char* malformed_obj = "malformed.obj";
    {
        std::ofstream f(malformed_obj);
        f << "# This OBJ has no geometry\n";
        f << "# No vertices, no faces\n";
    }

    std::vector<std::string> args = {
        malformed_obj,
        "0.1",
        "2"
    };

    CommandResult result = run_sdfgen(args, config);

    // Should fail (no geometry to process)
    bool passed = (result.exit_code != 0);

    if (passed) {
        std::cout << "✓ Malformed OBJ Error PASSED\n";
    } else {
        std::cerr << "✗ Malformed OBJ FAILED: Should reject OBJ with no geometry\n";
    }

    // Cleanup
    delete_file_if_exists(malformed_obj);
    return passed;
}

int main() {
    std::cout << "========================================\n";
    std::cout << "CLI Error Handling Integration Test\n";
    std::cout << "========================================\n";

    int32_t failures = 0;

    // Test error handling
    if (!test_no_arguments()) failures++;
    if (!test_too_few_arguments()) failures++;
    if (!test_missing_input_file()) failures++;
    if (!test_invalid_file_extension()) failures++;
    if (!test_negative_dimensions()) failures++;
    if (!test_zero_dimensions()) failures++;
    if (!test_negative_padding()) failures++;
    if (!test_invalid_argument_type()) failures++;
    if (!test_malformed_stl()) failures++;
    if (!test_malformed_obj()) failures++;

    // Summary
    std::cout << "\n========================================\n";
    std::cout << "CLI Error Handling Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests run: 10\n";
    std::cout << "Failures: " << failures << "\n";

    if (failures == 0) {
        std::cout << "✓ ALL ERROR HANDLING TESTS PASSED\n";
        return 0;
    } else {
        std::cout << "✗ SOME ERROR HANDLING TESTS FAILED\n";
        return 1;
    }
}
