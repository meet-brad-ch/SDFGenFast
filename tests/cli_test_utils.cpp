// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

// CLI testing utilities implementation

#include "cli_test_utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#ifdef _WIN32
    #include <io.h>
    #include <direct.h>
    #include <stdlib.h>  // for _fullpath, _MAX_PATH
    #define popen _popen
    #define pclose _pclose
    #define stat _stat
    #define access _access
    #define F_OK 0
#else
    #include <unistd.h>
    #include <linux/limits.h>  // for PATH_MAX
#endif

namespace cli_test {

// Constants
constexpr int32_t SDF_HEADER_SIZE = 36;  // 3 ints + 6 floats = 12 + 24 = 36 bytes
constexpr int32_t SIZEOF_FLOAT = 4;
constexpr int32_t BUFFER_SIZE = 4096;

// Execute SDFGen with given arguments
CommandResult run_sdfgen(
    const std::vector<std::string>& args,
    const TestConfig& config
) {
    CommandResult result;

    // Build command line
    std::string cmd = build_command_line(config.sdfgen_exe_path, args);

    // Redirect stderr to stdout to capture all output
    cmd += " 2>&1";

    if (config.verbose) {
        std::cout << "[CLI Test] Executing: " << cmd << "\n";
    }

    // Execute command and capture output (including stderr on Windows)
    FILE* pipe = popen(cmd.c_str(), "r");

    if (!pipe) {
        result.execution_failed = true;
        result.exit_code = -1;
        return result;
    }

    // Read output
    char buffer[BUFFER_SIZE];
    std::ostringstream output_stream;

    while (fgets(buffer, BUFFER_SIZE, pipe) != nullptr) {
        output_stream << buffer;
        if (config.verbose) {
            std::cout << buffer;
        }
    }

    result.stdout_output = output_stream.str();

    // Get exit code
    int32_t status = pclose(pipe);

#ifdef _WIN32
    // On Windows, pclose returns the exit code directly
    result.exit_code = status;
#else
    // On Unix, need to extract exit code from status
    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else {
        result.exit_code = -1;
    }
#endif

    return result;
}

// Build command line from arguments
std::string build_command_line(
    const std::string& executable,
    const std::vector<std::string>& args
) {
    std::ostringstream cmd;

    // Quote executable path if it contains spaces
    if (executable.find(' ') != std::string::npos) {
        cmd << "\"" << executable << "\"";
    } else {
        cmd << executable;
    }

    // Add arguments
    for (const auto& arg : args) {
        cmd << " ";
        // Quote arguments if they contain spaces
        if (arg.find(' ') != std::string::npos) {
            cmd << "\"" << arg << "\"";
        } else {
            cmd << arg;
        }
    }

    return cmd.str();
}

// File validation helpers
bool file_exists(const std::string& path) {
    return access(path.c_str(), F_OK) == 0;
}

bool file_is_readable(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return file.good();
}

int64_t get_file_size(const std::string& path) {
    struct stat stat_buf;
    int32_t rc = stat(path.c_str(), &stat_buf);
    return (rc == 0) ? static_cast<int64_t>(stat_buf.st_size) : -1;
}

bool delete_file_if_exists(const std::string& path) {
    if (file_exists(path)) {
        return std::remove(path.c_str()) == 0;
    }
    return true;  // File doesn't exist, considered success
}

// Read and validate SDF file header
SDFFileInfo read_sdf_header(const std::string& path) {
    SDFFileInfo info;
    info.file_size = get_file_size(path);

    if (info.file_size < SDF_HEADER_SIZE) {
        return info;  // Invalid: too small
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.good()) {
        return info;  // Invalid: can't open
    }

    // Read header: 3 ints (nx, ny, nz) + 6 floats (min_box, max_box)
    file.read(reinterpret_cast<char*>(&info.nx), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&info.ny), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&info.nz), sizeof(int32_t));

    file.read(reinterpret_cast<char*>(&info.min_x), sizeof(float));
    file.read(reinterpret_cast<char*>(&info.min_y), sizeof(float));
    file.read(reinterpret_cast<char*>(&info.min_z), sizeof(float));

    file.read(reinterpret_cast<char*>(&info.max_x), sizeof(float));
    file.read(reinterpret_cast<char*>(&info.max_y), sizeof(float));
    file.read(reinterpret_cast<char*>(&info.max_z), sizeof(float));

    if (!file.good()) {
        return info;  // Invalid: read failed
    }

    // Calculate expected file size
    int64_t total_cells = static_cast<int64_t>(info.nx) *
                         static_cast<int64_t>(info.ny) *
                         static_cast<int64_t>(info.nz);
    info.expected_size = SDF_HEADER_SIZE + (total_cells * SIZEOF_FLOAT);

    // Validate dimensions are positive
    if (info.nx > 0 && info.ny > 0 && info.nz > 0) {
        // Validate file size matches expected
        if (info.file_size == info.expected_size) {
            info.valid = true;
        }
    }

    return info;
}

// Helper function to check if a directory exists
static bool directory_exists(const std::string& path) {
#ifdef _WIN32
    return (_access(path.c_str(), F_OK) == 0);
#else
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
#endif
}

// Helper function to find the test resources directory
// Searches multiple possible locations to support running tests from any directory
static std::string find_resources_directory() {
    // List of possible resource locations (in priority order)
    const char* candidates[] = {
        "./resources/",           // Running from tests/ directory or build-Release/bin/ (with copied resources)
        "../../tests/resources/", // Running from build-Release/bin/ (without copied resources)
        "../resources/",          // Edge case: running from build-Release/
        "resources/",             // Fallback relative path
    };

    for (const char* candidate : candidates) {
        if (directory_exists(candidate)) {
#ifdef _WIN32
            // Convert to absolute path on Windows for reliability
            char abs_path[_MAX_PATH];
            if (_fullpath(abs_path, candidate, _MAX_PATH) != nullptr) {
                return std::string(abs_path) + "\\";
            }
#endif
            return candidate;
        }
    }

    // No resources found - return default and let tests fail with clear error
    return "./resources/";
}

// Get default test configuration
TestConfig get_default_test_config() {
    TestConfig config;

    // Resolve paths - use absolute paths on Windows to avoid popen() issues
#ifdef _WIN32
    char abs_exe_path[_MAX_PATH];

    // Try multiple possible locations for SDFGen.exe
    const char* exe_candidates[] = {
        "../build-Release/bin/SDFGen.exe",  // From tests/ directory
        "./SDFGen.exe",                      // From build-Release/bin/ directory
        "SDFGen.exe",                        // Current directory
    };

    bool exe_found = false;
    for (const char* candidate : exe_candidates) {
        if (_fullpath(abs_exe_path, candidate, _MAX_PATH) != nullptr) {
            // Check if file exists
            if (_access(abs_exe_path, F_OK) == 0) {
                config.sdfgen_exe_path = abs_exe_path;
                exe_found = true;
                break;
            }
        }
    }

    if (!exe_found) {
        // Fallback
        config.sdfgen_exe_path = "SDFGen.exe";
    }
#else
    // Unix: Get the directory containing this test executable and look for SDFGen there
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

    bool exe_found = false;
    if (len != -1) {
        exe_path[len] = '\0';
        // Get directory by finding last '/'
        char* last_slash = strrchr(exe_path, '/');
        if (last_slash != nullptr) {
            *last_slash = '\0';  // Null terminate at last slash to get directory
            std::string sdfgen_path = std::string(exe_path) + "/SDFGen";

            // Check if SDFGen exists in the same directory as the test executable
            if (access(sdfgen_path.c_str(), F_OK) == 0) {
                config.sdfgen_exe_path = sdfgen_path;
                exe_found = true;
            }
        }
    }

    if (!exe_found) {
        // Fallback: try relative paths
        const char* exe_candidates[] = {
            "./SDFGen",                      // From build-Release/bin/ directory
            "../build-Release/bin/SDFGen",   // From tests/ directory
        };

        for (const char* candidate : exe_candidates) {
            if (access(candidate, F_OK) == 0) {
                // Convert to absolute path for reliability
                char abs_path[PATH_MAX];
                if (realpath(candidate, abs_path) != nullptr) {
                    config.sdfgen_exe_path = abs_path;
                    exe_found = true;
                    break;
                }
            }
        }
    }

    if (!exe_found) {
        // Last resort fallback
        config.sdfgen_exe_path = "SDFGen";
    }
#endif

    // Find resources directory (works from any location)
    config.test_resources_dir = find_resources_directory();

    config.timeout_seconds = 120;
    config.verbose = false;

    return config;
}

// String helpers
bool string_contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

bool string_starts_with(const std::string& str, const std::string& prefix) {
    if (prefix.length() > str.length()) {
        return false;
    }
    return str.compare(0, prefix.length(), prefix) == 0;
}

bool string_ends_with(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

// Test assertion helpers
void assert_exit_code(
    const CommandResult& result,
    int32_t expected_code,
    const std::string& test_name
) {
    if (result.execution_failed) {
        std::cerr << "✗ " << test_name << " FAILED: Command execution failed\n";
        throw std::runtime_error("Command execution failed");
    }

    if (result.exit_code != expected_code) {
        std::cerr << "✗ " << test_name << " FAILED: Expected exit code "
                  << expected_code << ", got " << result.exit_code << "\n";
        std::cerr << "Output: " << result.stdout_output << "\n";
        throw std::runtime_error("Exit code mismatch");
    }
}

void assert_file_exists(
    const std::string& path,
    const std::string& test_name
) {
    if (!file_exists(path)) {
        std::cerr << "✗ " << test_name << " FAILED: Expected file does not exist: "
                  << path << "\n";
        throw std::runtime_error("File not found");
    }
}

void assert_output_contains(
    const CommandResult& result,
    const std::string& expected_text,
    const std::string& test_name
) {
    if (!string_contains(result.stdout_output, expected_text)) {
        std::cerr << "✗ " << test_name << " FAILED: Output does not contain '"
                  << expected_text << "'\n";
        std::cerr << "Actual output: " << result.stdout_output << "\n";
        throw std::runtime_error("Output text not found");
    }
}

void assert_sdf_dimensions(
    const SDFFileInfo& info,
    int32_t expected_nx,
    int32_t expected_ny,
    int32_t expected_nz,
    const std::string& test_name
) {
    if (!info.valid) {
        std::cerr << "✗ " << test_name << " FAILED: SDF file is invalid\n";
        throw std::runtime_error("Invalid SDF file");
    }

    if (info.nx != expected_nx || info.ny != expected_ny || info.nz != expected_nz) {
        std::cerr << "✗ " << test_name << " FAILED: Expected dimensions "
                  << expected_nx << "x" << expected_ny << "x" << expected_nz
                  << ", got " << info.nx << "x" << info.ny << "x" << info.nz << "\n";
        throw std::runtime_error("Dimension mismatch");
    }
}

} // namespace cli_test
