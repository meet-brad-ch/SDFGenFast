cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Test Utilities Library (shared code for all tests)
# ============================================================================
add_library(test_utils STATIC
    test_utils.cpp
)

target_link_libraries(test_utils PUBLIC
    sdfgen_common
    sdfgen_cpu
)

# Conditionally link GPU library if available
if(HAVE_CUDA AND SDFGEN_BUILD_GPU)
    target_link_libraries(test_utils PUBLIC sdfgen_gpu)
endif()

target_include_directories(test_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(test_utils PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# ============================================================================
# Test: Correctness comparison
# ============================================================================
add_executable(test_correctness
    test_correctness.cpp
)

target_link_libraries(test_correctness PRIVATE
    test_utils
)

set_target_properties(test_correctness PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Register with CTest
add_test(NAME correctness_test
    COMMAND test_correctness
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Set test properties
set_tests_properties(correctness_test PROPERTIES
    TIMEOUT 300  # 5 minute timeout
    LABELS "correctness;gpu"
)

# ============================================================================
# Test: File I/O for CPU and GPU implementations
# ============================================================================
add_executable(test_file_io
    test_file_io.cpp
)

target_link_libraries(test_file_io PRIVATE
    test_utils
)

set_target_properties(test_file_io PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Register with CTest
add_test(NAME file_io_test
    COMMAND test_file_io
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Set test properties
set_tests_properties(file_io_test PROPERTIES
    TIMEOUT 300  # 5 minute timeout
    LABELS "io;gpu"
)

# ============================================================================
# Test: STL File I/O for CPU and GPU implementations (using real STL files)
# ============================================================================
add_executable(test_stl_file_io
    test_stl_file_io.cpp
)

target_link_libraries(test_stl_file_io PRIVATE
    test_utils
)

set_target_properties(test_stl_file_io PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Register with CTest
add_test(NAME stl_file_io_test
    COMMAND test_stl_file_io
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Set test properties
set_tests_properties(stl_file_io_test PROPERTIES
    TIMEOUT 600  # 10 minute timeout (larger mesh)
    LABELS "io;gpu;stl"
)

# ============================================================================
# Test: OBJ File I/O for CPU and GPU implementations (using real OBJ files)
# ============================================================================
add_executable(test_obj_file_io
    test_obj_file_io.cpp
)

target_link_libraries(test_obj_file_io PRIVATE
    test_utils
)

set_target_properties(test_obj_file_io PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Register with CTest
add_test(NAME obj_file_io_test
    COMMAND test_obj_file_io
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Set test properties
set_tests_properties(obj_file_io_test PROPERTIES
    TIMEOUT 600  # 10 minute timeout
    LABELS "io;gpu;obj"
)

# ============================================================================
# CLI Test Utilities Library (for CLI integration tests)
# ============================================================================
add_library(cli_test_utils STATIC
    cli_test_utils.cpp
)

target_link_libraries(cli_test_utils PUBLIC
    sdfgen_common
)

target_include_directories(cli_test_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(cli_test_utils PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# ============================================================================
# CLI Integration Test: Usage Modes
# ============================================================================
add_executable(test_cli_modes
    test_cli_modes.cpp
)

target_link_libraries(test_cli_modes PRIVATE
    cli_test_utils
)

set_target_properties(test_cli_modes PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(NAME cli_modes_test
    COMMAND test_cli_modes
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

set_tests_properties(cli_modes_test PROPERTIES
    TIMEOUT 600
    LABELS "cli;integration;modes"
)

# ============================================================================
# CLI Integration Test: Input Formats
# ============================================================================
add_executable(test_cli_formats
    test_cli_formats.cpp
)

target_link_libraries(test_cli_formats PRIVATE
    cli_test_utils
)

set_target_properties(test_cli_formats PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(NAME cli_formats_test
    COMMAND test_cli_formats
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

set_tests_properties(cli_formats_test PROPERTIES
    TIMEOUT 600
    LABELS "cli;integration;formats"
)

# ============================================================================
# CLI Integration Test: Backend Selection
# ============================================================================
add_executable(test_cli_backend
    test_cli_backend.cpp
)

target_link_libraries(test_cli_backend PRIVATE
    cli_test_utils
)

set_target_properties(test_cli_backend PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(NAME cli_backend_test
    COMMAND test_cli_backend
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

set_tests_properties(cli_backend_test PROPERTIES
    TIMEOUT 600
    LABELS "cli;integration;backend;gpu"
)

# ============================================================================
# CLI Integration Test: Output File Generation
# ============================================================================
add_executable(test_cli_output
    test_cli_output.cpp
)

target_link_libraries(test_cli_output PRIVATE
    cli_test_utils
)

set_target_properties(test_cli_output PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(NAME cli_output_test
    COMMAND test_cli_output
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

set_tests_properties(cli_output_test PROPERTIES
    TIMEOUT 600
    LABELS "cli;integration;output"
)

# ============================================================================
# CLI Integration Test: Error Handling
# ============================================================================
add_executable(test_cli_errors
    test_cli_errors.cpp
)

target_link_libraries(test_cli_errors PRIVATE
    cli_test_utils
)

set_target_properties(test_cli_errors PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(NAME cli_errors_test
    COMMAND test_cli_errors
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

set_tests_properties(cli_errors_test PROPERTIES
    LABELS "CLI;Integration"
)

# ==============================================================================
# Test: CLI Thread Count Parameter
# ==============================================================================
add_executable(test_cli_threads
    test_cli_threads.cpp
)

target_link_libraries(test_cli_threads PRIVATE
    cli_test_utils
)

set_target_properties(test_cli_threads PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(NAME cli_threads_test
    COMMAND test_cli_threads
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

set_tests_properties(cli_threads_test PROPERTIES
    LABELS "CLI;Integration"
)

# ==============================================================================
# Test: Thread/Slice Ratio Edge Cases
# ==============================================================================
add_executable(test_thread_slice_ratios
    test_thread_slice_ratios.cpp
)

target_link_libraries(test_thread_slice_ratios PRIVATE
    test_utils
)

set_target_properties(test_thread_slice_ratios PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(NAME thread_slice_ratios_test
    COMMAND test_thread_slice_ratios
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

set_tests_properties(thread_slice_ratios_test PROPERTIES
    LABELS "CPU;Threading;EdgeCases"
)

# ============================================================================
# Library Test: ASCII STL Format Support
# ============================================================================
add_executable(test_ascii_stl
    test_ascii_stl.cpp
)

target_link_libraries(test_ascii_stl PRIVATE
    test_utils
)

set_target_properties(test_ascii_stl PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(NAME ascii_stl_test
    COMMAND test_ascii_stl
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

set_tests_properties(ascii_stl_test PROPERTIES
    TIMEOUT 600
    LABELS "library;formats;stl;ascii;gpu"
)

# ============================================================================
# Library Test: Mode 1 Legacy (dx-based sizing)
# ============================================================================
add_executable(test_mode1_legacy
    test_mode1_legacy.cpp
)

target_link_libraries(test_mode1_legacy PRIVATE
    test_utils
)

set_target_properties(test_mode1_legacy PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(NAME mode1_legacy_test
    COMMAND test_mode1_legacy
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

set_tests_properties(mode1_legacy_test PROPERTIES
    TIMEOUT 600
    LABELS "library;modes;legacy"
)

# ============================================================================
# Library Test: VTK Output Format Support (conditional on HAVE_VTK)
# ============================================================================
add_executable(test_vtk_output
    test_vtk_output.cpp
)

target_link_libraries(test_vtk_output PRIVATE
    test_utils
)

set_target_properties(test_vtk_output PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(NAME vtk_output_test
    COMMAND test_vtk_output
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

set_tests_properties(vtk_output_test PROPERTIES
    TIMEOUT 600
    LABELS "library;output;vtk"
)

# ============================================================================
# Library Test: Mesh Repair API
# ============================================================================
add_executable(test_mesh_repair
    test_mesh_repair.cpp
)

target_link_libraries(test_mesh_repair PRIVATE
    sdfgen_common
)

set_target_properties(test_mesh_repair PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(NAME mesh_repair_test
    COMMAND test_mesh_repair
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

set_tests_properties(mesh_repair_test PROPERTIES
    TIMEOUT 60
    LABELS "library;mesh;repair"
)

# ============================================================================
# Performance Benchmark (not a test - generates performance data)
# ============================================================================
add_executable(benchmark_performance
    benchmark_performance.cpp
)

target_link_libraries(benchmark_performance PRIVATE
    test_utils
)

set_target_properties(benchmark_performance PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# ============================================================================
# Copy test resources to build directory
# ============================================================================
# Copy resources/ directory to bin/resources/ so tests can be run from anywhere
add_custom_target(copy_test_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources
    COMMENT "Copying test resources to build directory"
    VERBATIM
)
