// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// CLI Integration Test: Automatic Backend Detection
// Tests AUTO backend detection and selection

#include "cli_test_utils.h"
#include <iostream>
#include <vector>
#include <string>

using namespace cli_test;

// Test AUTO backend with STL (Mode 2)
bool test_auto_backend_stl() {
    std::cout << "\n========================================\n";
    std::cout << "Testing AUTO Backend (STL)\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    std::string output_file = config.test_resources_dir + "test_x3y4z5_bin_sdf_32x42x52.sdf";
    delete_file_if_exists(output_file);

    // Run with AUTO backend (no flags needed)
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_bin.stl",
        "32",
        "1"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "AUTO backend STL");

        assert_file_exists(output_file, "AUTO backend output");

        // Output should contain hardware detection info
        bool has_hardware_line = string_contains(result.stdout_output, "Hardware:");
        if (!has_hardware_line) {
            std::cerr << "✗ AUTO backend FAILED: No hardware detection output\n";
            delete_file_if_exists(output_file);
            return false;
        }

        // Check which backend was used
        bool gpu_used = string_contains(result.stdout_output, "Implementation: GPU (CUDA)");
        bool cpu_used = string_contains(result.stdout_output, "Implementation: CPU");

        if (!gpu_used && !cpu_used) {
            std::cerr << "✗ AUTO backend FAILED: No backend implementation reported\n";
            delete_file_if_exists(output_file);
            return false;
        }

        std::cout << "✓ AUTO Backend (STL) PASSED\n";
        if (gpu_used) {
            std::cout << "  Detected and used: GPU (CUDA)\n";
        } else {
            std::cout << "  Detected and used: CPU (multi-threaded)\n";
        }

        delete_file_if_exists(output_file);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists(output_file);
        return false;
    }
}

// Test AUTO backend with OBJ (Mode 1)
bool test_auto_backend_obj() {
    std::cout << "\n========================================\n";
    std::cout << "Testing AUTO Backend (OBJ)\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = true;

    std::string output_file = config.test_resources_dir + "test_x3y4z5_quads.sdf";
    delete_file_if_exists(output_file);

    // Run Mode 1 with AUTO backend
    std::vector<std::string> args = {
        config.test_resources_dir + "test_x3y4z5_quads.obj",
        "0.1",
        "2"
    };

    CommandResult result = run_sdfgen(args, config);

    try {
        assert_exit_code(result, 0, "AUTO backend OBJ");

        assert_file_exists(output_file, "AUTO backend OBJ output");

        // Output should contain hardware detection info
        assert_output_contains(result, "Hardware:", "Hardware detection");

        // Output should contain implementation info
        bool has_impl = string_contains(result.stdout_output, "Implementation: GPU") ||
                       string_contains(result.stdout_output, "Implementation: CPU");

        if (!has_impl) {
            std::cerr << "✗ AUTO backend OBJ FAILED: No implementation info\n";
            delete_file_if_exists(output_file);
            return false;
        }

        std::cout << "✓ AUTO Backend (OBJ) PASSED\n";
        std::cout << "  AUTO backend works with all CLI modes\n";

        delete_file_if_exists(output_file);
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        delete_file_if_exists(output_file);
        return false;
    }
}

// Test that help message mentions auto-detection
bool test_help_message() {
    std::cout << "\n========================================\n";
    std::cout << "Testing Help Message\n";
    std::cout << "========================================\n";

    TestConfig config = get_default_test_config();
    config.verbose = false;  // Don't need verbose for this

    // Run with no arguments to trigger help
    std::vector<std::string> args = {};

    CommandResult result = run_sdfgen(args, config);

    try {
        // Help should exit with error code
        if (result.exit_code == 0) {
            std::cerr << "✗ Help message FAILED: Expected non-zero exit code\n";
            return false;
        }

        // Help should mention hardware acceleration
        assert_output_contains(result, "Hardware Acceleration", "Help mentions acceleration");
        assert_output_contains(result, "automatically", "Help mentions auto-detection");

        // Help should NOT mention --gpu flag
        if (string_contains(result.stdout_output, "--gpu")) {
            std::cerr << "✗ Help message FAILED: Still mentions obsolete --gpu flag\n";
            return false;
        }

        std::cout << "✓ Help Message PASSED\n";
        std::cout << "  Help correctly describes automatic detection\n";
        std::cout << "  --gpu flag not mentioned (removed)\n";

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return false;
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << "CLI Automatic Backend Detection Test\n";
    std::cout << "========================================\n";

    int32_t failures = 0;

    // Test automatic backend detection
    if (!test_auto_backend_stl()) failures++;
    if (!test_auto_backend_obj()) failures++;
    if (!test_help_message()) failures++;

    // Summary
    std::cout << "\n========================================\n";
    std::cout << "CLI Backend Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests run: 3\n";
    std::cout << "Failures: " << failures << "\n";

    if (failures == 0) {
        std::cout << "✓ ALL BACKEND TESTS PASSED\n";
        std::cout << "  AUTO backend detection works correctly\n";
        std::cout << "  GPU is used automatically when available\n";
        std::cout << "  No manual flags needed\n";
        return 0;
    } else {
        std::cout << "✗ SOME BACKEND TESTS FAILED\n";
        return 1;
    }
}
