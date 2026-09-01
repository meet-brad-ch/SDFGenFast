# ============================================================================
# C++ test suite
# ============================================================================
# Each entry: <executable>|<test name>|<timeout seconds>|<labels (comma-separated)>|<library to link>
# The foreach() below turns every entry into an executable + CTest registration.

option(SDFGEN_BUILD_BENCHMARKS "Build the performance benchmark executable" OFF)

# ----------------------------------------------------------------------------
# Shared test libraries
# ----------------------------------------------------------------------------
add_library(test_utils STATIC
    test_utils.cpp
)
target_include_directories(test_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_utils
    PUBLIC
        sdfgen_common
    PRIVATE
        sdfgen_options
)

add_library(cli_test_utils STATIC
    cli_test_utils.cpp
)
target_include_directories(cli_test_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cli_test_utils
    PUBLIC
        sdfgen_common
    PRIVATE
        sdfgen_options
)

# Tell the test code where the real artifacts live instead of hardcoding
# build-directory names into the sources.
target_compile_definitions(cli_test_utils PRIVATE
    SDFGEN_BINARY_PATH="$<TARGET_FILE:SDFGen>"
    SDFGEN_TEST_RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
)
target_compile_definitions(test_utils PRIVATE
    SDFGEN_TEST_RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
)

# ----------------------------------------------------------------------------
# Test registrations
# ----------------------------------------------------------------------------
set(SDFGEN_TESTS
    "test_correctness|correctness_test|300|correctness,gpu|test_utils"
    "test_file_io|file_io_test|300|io,gpu|test_utils"
    "test_stl_file_io|stl_file_io_test|600|io,gpu,stl|test_utils"
    "test_obj_file_io|obj_file_io_test|600|io,gpu,obj|test_utils"
    "test_cli_modes|cli_modes_test|600|cli,integration,modes|cli_test_utils"
    "test_cli_formats|cli_formats_test|600|cli,integration,formats|cli_test_utils"
    "test_cli_backend|cli_backend_test|600|cli,integration,backend,gpu|cli_test_utils"
    "test_cli_output|cli_output_test|600|cli,integration,output|cli_test_utils"
    "test_cli_errors|cli_errors_test|600|cli,integration,errors|cli_test_utils"
    "test_cli_threads|cli_threads_test|600|cli,integration,threads|cli_test_utils"
    "test_thread_slice_ratios|thread_slice_ratios_test|600|cpu,threading,edgecases|test_utils"
    "test_ascii_stl|ascii_stl_test|600|library,formats,stl,ascii,gpu|test_utils"
    "test_mode1_legacy|mode1_legacy_test|600|library,modes,legacy|test_utils"
    "test_vtk_output|vtk_output_test|600|library,output,vtk|test_utils"
    "test_mesh_repair|mesh_repair_test|60|library,mesh,repair|sdfgen_common"
)

foreach(entry IN LISTS SDFGEN_TESTS)
    string(REPLACE "|" ";" fields "${entry}")
    list(GET fields 0 exe)
    list(GET fields 1 test_name)
    list(GET fields 2 timeout)
    list(GET fields 3 labels)
    list(GET fields 4 linklib)
    string(REPLACE "," ";" labels "${labels}")

    add_executable(${exe} ${exe}.cpp)
    target_link_libraries(${exe} PRIVATE ${linklib} sdfgen_options)

    add_test(NAME ${test_name}
        COMMAND ${exe}
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT ${timeout}
        LABELS "${labels}"
    )
endforeach()

# CLI tests execute the SDFGen binary; make sure it is built first.
foreach(cli_exe test_cli_modes test_cli_formats test_cli_backend test_cli_output
                test_cli_errors test_cli_threads)
    add_dependencies(${cli_exe} SDFGen)
endforeach()

# ----------------------------------------------------------------------------
# Performance benchmark (not a test - generates performance data)
# ----------------------------------------------------------------------------
if(SDFGEN_BUILD_BENCHMARKS)
    add_executable(benchmark_performance
        benchmark_performance.cpp
    )
    target_link_libraries(benchmark_performance PRIVATE test_utils sdfgen_options)
    target_compile_definitions(benchmark_performance PRIVATE
        SDFGEN_TEST_RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
    )
endif()

# ----------------------------------------------------------------------------
# Copy test resources next to the binaries
# ----------------------------------------------------------------------------
# Tests first look at SDFGEN_TEST_RESOURCES_DIR (absolute, set above); the
# copy remains for running a test binary by hand from the bin directory.
add_custom_target(copy_test_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources
    COMMENT "Copying test resources to build directory"
    VERBATIM
)
