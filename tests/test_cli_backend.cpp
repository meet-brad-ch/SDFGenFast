// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// CLI Integration Test: Automatic Backend Detection
// The CLI reports the hardware it selected; --cpu forces the CPU path.

#include "cli_test_utils.h"
#include <iostream>
#include <vector>

using namespace cli_test;

// The help text must describe automatic acceleration and must not
// advertise a --gpu flag (GPU use is automatic).
static bool test_help_message(const TestConfig& config) {
    std::cout << "\n--- Help message ---\n";

    CommandResult result = run_sdfgen({"-h"}, config);

    if (!string_contains(result.stdout_output, "Hardware Acceleration") ||
        !string_contains(result.stdout_output, "automatically")) {
        std::cerr << "[FAIL] Help: must mention automatic hardware acceleration\n";
        std::cerr << "Output: " << result.stdout_output << "\n";
        return false;
    }
    if (string_contains(result.stdout_output, "--gpu")) {
        std::cerr << "[FAIL] Help: must not advertise an obsolete --gpu flag\n";
        return false;
    }
    std::cout << "[PASS] Help message\n";
    return true;
}

int main() {
    std::cout << "CLI Automatic Backend Detection Test\n";

    TestConfig config = get_default_test_config();
    const std::string res = config.test_resources_dir;

    // "Implementation: CPU" also matches the multi-threaded CPU line; a
    // GPU build on GPU hardware prints "Implementation: GPU (CUDA)".
    // Both are valid AUTO outcomes, so only the report line is required.
    const SuccessCase cases[] = {
        {"AUTO backend (STL)", {res + "test_x3y4z5_bin.stl", "32", "1"},
         "test_x3y4z5_bin_sdf_32x42x52.sdf", 32, 42, 52,
         {"Hardware:", "Implementation:"}},
        {"AUTO backend (OBJ)", {res + "test_x3y4z5_quads.obj", "0.1", "2"},
         "test_x3y4z5_quads.sdf", 0, 0, 0,
         {"Hardware:", "Implementation:"}},
        {"Forced CPU backend (--cpu)", {res + "test_x3y4z5_bin.stl", "32", "1", "--cpu"},
         "test_x3y4z5_bin_sdf_32x42x52.sdf", 32, 42, 52,
         {"Implementation: CPU"}},
    };

    int32_t failures = 0;
    int32_t total = 0;
    for (const SuccessCase& c : cases) {
        total++;
        if (!expect_success(c, config)) failures++;
    }
    total++;
    if (!test_help_message(config)) failures++;

    return summarize("CLI Backend Detection", failures, total);
}
