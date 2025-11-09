// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief CLI testing utilities for spawning SDFGen executable and validating results
 *
 * Provides cross-platform process execution, output validation, file system operations,
 * and assertion helpers for testing the SDFGen command-line interface.
 */

namespace cli_test {

/**
 * @brief Result from running a CLI command
 *
 * Contains process exit status, captured output streams, and execution metadata
 * for validation and debugging of CLI test runs.
 */
struct CommandResult {
    int32_t exit_code = -1;          ///< Process exit code (-1 if not run)
    std::string stdout_output;       ///< Captured standard output
    std::string stderr_output;       ///< Captured standard error (if captured separately)
    bool timed_out = false;          ///< Whether command exceeded timeout limit
    bool execution_failed = false;   ///< Whether process execution itself failed (not exit code)
};

/**
 * @brief Configuration for CLI test execution
 *
 * Specifies paths, timeouts, and verbosity settings for running CLI tests.
 */
struct TestConfig {
    std::string sdfgen_exe_path;     ///< Path to SDFGen executable (e.g., "SDFGen.exe")
    std::string test_resources_dir;  ///< Path to test resources directory containing test meshes
    int32_t timeout_seconds = 120;   ///< Maximum execution time before killing process
    bool verbose = false;            ///< If true, print command output during execution
};

/**
 * @brief Execute SDFGen with given arguments
 *
 * Spawns the SDFGen executable as a subprocess with specified arguments, captures output,
 * and enforces timeout limits. Cross-platform implementation supporting Windows and Unix systems.
 *
 * @param args Command-line arguments to pass to SDFGen (without executable name)
 * @param config Test configuration including executable path and timeout
 * @return CommandResult with exit code, captured output, and execution status
 */
CommandResult run_sdfgen(
    const std::vector<std::string>& args,
    const TestConfig& config
);

/**
 * @brief Build command line from executable and arguments
 *
 * Constructs a properly escaped command line string for process execution.
 * Handles spaces and special characters in arguments appropriately for the platform.
 *
 * @param executable Path to executable file
 * @param args Command-line arguments
 * @return Properly formatted command line string
 */
std::string build_command_line(
    const std::string& executable,
    const std::vector<std::string>& args
);

/**
 * @brief Check if file exists
 * @param path File path to check
 * @return true if file exists, false otherwise
 */
bool file_exists(const std::string& path);

/**
 * @brief Check if file exists and is readable
 * @param path File path to check
 * @return true if file exists and can be read, false otherwise
 */
bool file_is_readable(const std::string& path);

/**
 * @brief Get file size in bytes
 * @param path File path
 * @return File size in bytes, or -1 if file doesn't exist or error occurred
 */
int64_t get_file_size(const std::string& path);

/**
 * @brief Delete file if it exists
 * @param path File path to delete
 * @return true if file was deleted or didn't exist, false if deletion failed
 */
bool delete_file_if_exists(const std::string& path);

/**
 * @brief SDF file metadata and validation information
 *
 * Contains header information extracted from SDF binary file including grid dimensions,
 * bounding box, and file size validation.
 */
struct SDFFileInfo {
    bool valid = false;              ///< Whether file header was read and validated successfully
    int32_t nx = 0, ny = 0, nz = 0;  ///< Grid dimensions
    float min_x = 0.0f, min_y = 0.0f, min_z = 0.0f;  ///< Bounding box minimum corner
    float max_x = 0.0f, max_y = 0.0f, max_z = 0.0f;  ///< Bounding box maximum corner
    int64_t file_size = 0;           ///< Actual file size in bytes
    int64_t expected_size = 0;       ///< Expected size (36 byte header + nx*ny*nz*4 bytes data)
};

/**
 * @brief Read and validate SDF file header
 *
 * Reads the 36-byte SDF binary header containing grid dimensions and bounding box.
 * Validates file size matches expected size based on header dimensions.
 *
 * @param path Path to SDF file
 * @return SDFFileInfo with header data and validation status
 */
SDFFileInfo read_sdf_header(const std::string& path);

/**
 * @brief Get default test configuration
 *
 * Automatically locates the SDFGen executable and test resources directory based
 * on the current build configuration and platform conventions.
 *
 * @return TestConfig with paths configured for the current environment
 */
TestConfig get_default_test_config();

/**
 * @brief Check if string contains substring
 * @param haystack String to search in
 * @param needle Substring to search for
 * @return true if haystack contains needle, false otherwise
 */
bool string_contains(const std::string& haystack, const std::string& needle);

/**
 * @brief Check if string starts with prefix
 * @param str String to check
 * @param prefix Prefix to search for
 * @return true if str starts with prefix, false otherwise
 */
bool string_starts_with(const std::string& str, const std::string& prefix);

/**
 * @brief Check if string ends with suffix
 * @param str String to check
 * @param suffix Suffix to search for
 * @return true if str ends with suffix, false otherwise
 */
bool string_ends_with(const std::string& str, const std::string& suffix);

/**
 * @brief Assert that command exit code matches expected value
 *
 * Validates process exit code and prints descriptive error message if mismatch occurs.
 * Terminates test execution on assertion failure.
 *
 * @param result Command execution result
 * @param expected_code Expected exit code (typically 0 for success)
 * @param test_name Name of test for error reporting
 */
void assert_exit_code(
    const CommandResult& result,
    int32_t expected_code,
    const std::string& test_name
);

/**
 * @brief Assert that file exists
 *
 * Validates that specified file exists and is accessible. Prints descriptive error
 * message and terminates test execution if file doesn't exist.
 *
 * @param path File path to check
 * @param test_name Name of test for error reporting
 */
void assert_file_exists(
    const std::string& path,
    const std::string& test_name
);

/**
 * @brief Assert that command output contains expected text
 *
 * Validates that command stdout contains specified substring. Prints descriptive error
 * message with actual output and terminates test execution if text not found.
 *
 * @param result Command execution result
 * @param expected_text Text that should appear in stdout
 * @param test_name Name of test for error reporting
 */
void assert_output_contains(
    const CommandResult& result,
    const std::string& expected_text,
    const std::string& test_name
);

/**
 * @brief Assert that SDF file has expected dimensions
 *
 * Validates that SDF file header contains specified grid dimensions. Prints descriptive
 * error message with actual dimensions and terminates test execution if mismatch occurs.
 *
 * @param info SDF file information from header read
 * @param expected_nx Expected X dimension
 * @param expected_ny Expected Y dimension
 * @param expected_nz Expected Z dimension
 * @param test_name Name of test for error reporting
 */
void assert_sdf_dimensions(
    const SDFFileInfo& info,
    int32_t expected_nx,
    int32_t expected_ny,
    int32_t expected_nz,
    const std::string& test_name
);

} // namespace cli_test
