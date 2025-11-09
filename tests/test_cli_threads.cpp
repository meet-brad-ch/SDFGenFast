// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// Test for CLI thread count parameter
// Verifies that the threads parameter can be specified and produces valid results

#include "cli_test_utils.h"
#include <iostream>

void test_mode1_threads(const cli_test::TestConfig& config) {
    std::cout << "========================================\n";
    std::cout << "Testing Mode 1 with Thread Count\n";
    std::cout << "========================================\n";

    // Test with 1 thread
    std::cout << "\nTesting with 1 thread...\n";
    std::vector<std::string> args1 = {"resources/test_x3y4z5_quads.obj", "0.1", "2", "1"};
    auto result1 = cli_test::run_sdfgen(args1, config);

    if(result1.exit_code != 0) {
        std::cerr << "ERROR: Command failed with exit code " << result1.exit_code << "\n";
        exit(1);
    }
    if(!cli_test::string_contains(result1.stdout_output, "CPU threads: 1")) {
        std::cerr << "ERROR: Thread count not reported in output\n";
        exit(1);
    }
    std::cout << "✓ 1 thread accepted\n";

    // Test with 10 threads
    std::cout << "\nTesting with 10 threads...\n";
    std::vector<std::string> args10 = {"resources/test_x3y4z5_quads.obj", "0.1", "2", "10"};
    auto result10 = cli_test::run_sdfgen(args10, config);

    if(result10.exit_code != 0) {
        std::cerr << "ERROR: Command failed with exit code " << result10.exit_code << "\n";
        exit(1);
    }
    if(!cli_test::string_contains(result10.stdout_output, "CPU threads: 10")) {
        std::cerr << "ERROR: Thread count not reported in output\n";
        exit(1);
    }
    std::cout << "✓ 10 threads accepted\n";

    // Test with 0 (auto-detect)
    std::cout << "\nTesting with 0 (auto-detect)...\n";
    std::vector<std::string> args0 = {"resources/test_x3y4z5_quads.obj", "0.1", "2", "0"};
    auto result0 = cli_test::run_sdfgen(args0, config);

    if(result0.exit_code != 0) {
        std::cerr << "ERROR: Command failed with exit code " << result0.exit_code << "\n";
        exit(1);
    }
    if(!cli_test::string_contains(result0.stdout_output, "CPU threads: auto-detect")) {
        std::cerr << "ERROR: Auto-detect not reported in output\n";
        exit(1);
    }
    std::cout << "✓ Auto-detect accepted\n";

    std::cout << "\n✓ Mode 1 thread parameter tests PASSED\n";
}

void test_mode2a_threads(const cli_test::TestConfig& config) {
    std::cout << "\n========================================\n";
    std::cout << "Testing Mode 2a with Thread Count\n";
    std::cout << "========================================\n";

    // Test with padding and threads
    std::cout << "\nTesting STL with Nx, padding, and threads...\n";
    std::vector<std::string> args = {"resources/test_x3y4z5_bin.stl", "32", "1", "5"};
    auto result = cli_test::run_sdfgen(args, config);

    if(result.exit_code != 0) {
        std::cerr << "ERROR: Command failed with exit code " << result.exit_code << "\n";
        exit(1);
    }
    if(!cli_test::string_contains(result.stdout_output, "CPU threads: 5")) {
        std::cerr << "ERROR: Thread count not reported in output\n";
        exit(1);
    }
    std::cout << "✓ Mode 2a with threads accepted\n";
}

void test_mode2b_threads(const cli_test::TestConfig& config) {
    std::cout << "\n========================================\n";
    std::cout << "Testing Mode 2b with Thread Count\n";
    std::cout << "========================================\n";

    // Test with padding and threads
    std::cout << "\nTesting STL with Nx/Ny/Nz, padding, and threads...\n";
    std::vector<std::string> args = {"resources/test_x3y4z5_bin.stl", "32", "32", "32", "1", "8"};
    auto result = cli_test::run_sdfgen(args, config);

    if(result.exit_code != 0) {
        std::cerr << "ERROR: Command failed with exit code " << result.exit_code << "\n";
        exit(1);
    }
    if(!cli_test::string_contains(result.stdout_output, "CPU threads: 8")) {
        std::cerr << "ERROR: Thread count not reported in output\n";
        exit(1);
    }
    std::cout << "✓ Mode 2b with threads accepted\n";
}

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "CLI Thread Count Parameter Test\n";
    std::cout << "========================================\n\n";

    // Get test configuration
    auto config = cli_test::get_default_test_config();

    test_mode1_threads(config);
    test_mode2a_threads(config);
    test_mode2b_threads(config);

    std::cout << "\n========================================\n";
    std::cout << "CLI Thread Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests run: 5\n";
    std::cout << "Failures: 0\n";
    std::cout << "✓ ALL THREAD PARAMETER TESTS PASSED\n";

    return 0;
}
