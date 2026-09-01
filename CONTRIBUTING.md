# Contributing to SDFGenFast

Thank you for your interest in contributing. This document describes how to build the project, run the tests, and submit a change.

## Build

See [BUILD.md](BUILD.md) for the full instructions. The short version:

```bash
git clone --recurse-submodules https://github.com/meet-radek/SDFGenFast
cd SDFGenFast
cmake -B build-Release -DCMAKE_BUILD_TYPE=Release
cmake --build build-Release
```

Add `-DBUILD_PYTHON_BINDINGS=ON` to build the Python extension, or use `pip install .`.

## Test

Every change must keep both test suites green:

```bash
# C++ (15 suites)
ctest --test-dir build-Release --output-on-failure

# Python (51 tests)
pip install pytest
pytest python/tests -q
```

When you add a feature or fix a bug, add a test that fails without your change:

- **C++ library tests**: add a `test_*.cpp` in `tests/`, register it with one line in the test table in `tests/CMakeLists.txt`, and use the helpers in `test_utils`.
- **C++ CLI tests**: add `SuccessCase`/`FailureCase` entries to the relevant `tests/test_cli_*.cpp` table.
- **Python tests**: add to `python/tests/test_sdfgen.py`.

## Code standards

- **C++17**, formatted to match the surrounding code.
- **Warnings are errors in spirit**: the build uses `/W4` (MSVC) or `-Wall -Wextra -Wpedantic` (GCC/Clang). Your change must not add warnings. You can enforce this locally with `-DSDFGEN_WARNINGS_AS_ERRORS=ON`.
- **Keep the CPU and GPU paths in agreement.** `test_correctness` compares them: zero sign mismatches, near-band agreement within half a cell. If you change one path, check the other.
- **No dead code.** Do not commit commented-out code or unused parameters.
- **Documentation**: public functions carry Doxygen comments. Update README.md, BUILD.md, or python/README.md when behavior visible to users changes, and add a CHANGELOG.md entry.

## Submitting a change

1. Fork the repository and create a branch.
2. Make the change, with tests.
3. Verify both test suites pass and the build is warning-free.
4. Open a pull request that describes what changed and why. Reference any related issue.

For a large change, open an issue first to discuss the approach.

## Ideas for contributions

- Additional output formats (OpenVDB, NanoVDB)
- GPU-accelerated mesh preprocessing
- Distance field gradients
- Multi-GPU support
- Batch processing API

## License

By contributing, you agree that your contributions are licensed under the MIT License (see [LICENSE](LICENSE)).
