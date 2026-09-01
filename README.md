# SDFGen

[![CI](https://github.com/meet-radek/SDFGenFast/actions/workflows/ci.yml/badge.svg)](https://github.com/meet-radek/SDFGenFast/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![GitHub release](https://img.shields.io/github/v/release/meet-radek/SDFGenFast)](https://github.com/meet-radek/SDFGenFast/releases)
[![Python 3.8+](https://img.shields.io/badge/python-3.8%2B-blue)](python/README.md)

A command-line tool and Python library that generates grid-based signed distance fields (SDFs) from triangle meshes.

This is an enhanced fork of [Christopher Batty's SDFGen](https://github.com/christopherbatty/SDFGen). The fork adds GPU acceleration, a multi-threaded CPU path, automatic hardware detection, Python bindings, and a cross-platform CMake build. See [CHANGELOG.md](CHANGELOG.md) for the full list of changes.

## Features

- **Automatic GPU acceleration**: The tool detects a CUDA-capable GPU and uses it. No flags are needed.
- **Multi-threaded CPU fallback**: The CPU path runs when no GPU is available, or when you pass `--cpu`.
- **Mesh watertightness check**: The tool always reports holes and non-manifold edges.
- **Mesh repair**: The `--fix` flag fills holes in non-watertight meshes.
- **Python bindings**: A nanobind-based API with NumPy integration and GPU support.
- **Input formats**: Binary STL, ASCII STL, and Wavefront OBJ. Quads are triangulated automatically.
- **Grid sizing**: Proportional sizing from one dimension, or explicit dimensions.
- **Binary SDF output**: A compact binary format with a metadata header.
- **Cross-platform**: Windows (MSVC) and Linux (GCC/Clang).
- **Tests**: 15 C++ test suites and 51 Python tests.

## Quick Start

### Installation

Build from source. See [BUILD.md](BUILD.md) for full instructions.

```bash
git clone --recurse-submodules https://github.com/meet-radek/SDFGenFast
cd SDFGenFast
cmake -B build-Release -DCMAKE_BUILD_TYPE=Release
cmake --build build-Release
```

Python bindings:

```bash
pip install .
```

### CLI Usage

**OBJ files** take a cell size (`dx`) and an optional padding value:

```bash
SDFGen mesh.obj 0.01        # dx = 0.01, padding = 1 (default)
SDFGen mesh.obj 0.01 2      # dx = 0.01, padding = 2 cells
```

**STL files** take grid dimensions. All dimensions include the padding cells.

```bash
SDFGen mesh.stl 128           # 128 cells in X; Y and Z are proportional
SDFGen mesh.stl 128 2         # the same, with padding = 2
SDFGen mesh.stl 128 256 64    # explicit 128 x 256 x 64 grid
SDFGen mesh.stl 128 256 64 2  # the same, with padding = 2
```

**Flags** (all modes):

```bash
SDFGen mesh.stl 128 -p 2    # padding (wins over a positional padding value)
SDFGen mesh.stl 128 -t 10   # use 10 CPU threads (0 = auto-detect, default)
SDFGen mesh.stl 128 --fix   # repair non-watertight meshes (fill holes)
SDFGen mesh.stl 128 --cpu   # force the CPU backend
```

Run `SDFGen --help` for the full option list.

**SDF to mesh conversion** (for debugging and visualization):

```bash
sdf_to_mesh input.sdf output.obj           # extract the zero isosurface
sdf_to_mesh input.sdf output.obj -i 0.5    # extract the isosurface at distance 0.5
```

**Watertightness** is always checked and reported:

```
Mesh Analysis:
  Total edges:        17
  Boundary edges:     4 (holes detected)
  Non-manifold edges: 0
  Number of holes:    1
  Is manifold:        yes
  Is watertight:      NO

  WARNING: Mesh is not watertight. SDF sign determination may be incorrect.
           Use --fix flag to attempt automatic hole filling.
```

### Python Usage

```python
import sdfgen

# Generate from a file
sdf, metadata = sdfgen.generate_from_file(
    "mesh.stl",
    nx=256,           # total grid size in X, including padding
    padding=2,
    backend="auto"    # uses the GPU when one is available
)

# Generate from arrays
vertices, triangles, bounds = sdfgen.load_mesh("mesh.obj")
sdf = sdfgen.generate_sdf(
    vertices, triangles,
    origin=(0, 0, 0),
    dx=0.01,
    nx=100, ny=100, nz=100
)

# Save to a file
sdfgen.save_sdf("output.sdf", sdf, origin=(0, 0, 0), dx=0.01)

# Check GPU availability
print(f"GPU available: {sdfgen.is_gpu_available()}")
```

**Note (changed in 2.2):** `nx`, `ny`, and `nz` are the total grid size, including the padding cells. This matches the CLI. Before 2.2 the Python API added padding on top of the requested size.

See [python/README.md](python/README.md) for the complete API documentation.

## Hardware Acceleration

SDFGen selects its backend automatically:

- **Build time**: CMake searches for the CUDA Toolkit. When it is found, GPU support is compiled in.
- **Run time**: The tool queries for a CUDA device. When one is present, the GPU backend runs. Otherwise the multi-threaded CPU backend runs.

The output names the backend that was used:

```
  Hardware: GPU acceleration available
  Implementation: GPU (CUDA)
```

or

```
  Hardware: No CUDA GPU detected
  Implementation: CPU (multi-threaded)
```

Pass `--cpu` (CLI) or `backend="cpu"` (Python) to force the CPU backend. When you request the GPU explicitly (`backend="gpu"`) and it fails, the error is reported instead of a silent fallback.

## Performance

Measured 2026-08-31 on an Intel Core i7-13700KF (16 cores / 24 threads) with an NVIDIA GeForce RTX 4090, CUDA 13.3, Windows 11. The benchmark sizes each grid proportionally from Nx over a 3:4:5 test mesh, so "Nx = 256" is a 256 x 340 x 424 grid.

| Nx  | Grid            | Cells | CPU (1 thread) | CPU (10 threads) | CPU (20 threads) | GPU    |
|-----|-----------------|-------|----------------|------------------|------------------|--------|
| 64  | 64 x 84 x 104   | 559K  | 698 ms         | 169 ms           | 206 ms           | 71 ms  |
| 128 | 128 x 169 x 211 | 4.6M  | 5.69 s         | 1.04 s           | 1.21 s           | 386 ms |
| 256 | 256 x 340 x 424 | 36.9M | 46.1 s         | 7.37 s           | 5.78 s           | 814 ms |

- The GPU was the fastest backend at every measured size: 2.4x to 7.1x faster than the best CPU time, and 9.9x to 56.6x faster than one thread.
- The GPU advantage grows with grid size.

See [Appendix A](#appendix-a-performance-benchmarks) for the full results and how to reproduce them.

## Output Format

The binary SDF file has a 36-byte header followed by the distance data.

**Header (36 bytes):**

```
[Nx, Ny, Nz]           (3 x int32)   - grid dimensions
[xmin, ymin, zmin]     (3 x float32) - grid bounds minimum
[xmax, ymax, zmax]     (3 x float32) - grid bounds maximum
```

The cell size is derived from the header: `dx = (xmax - xmin) / Nx`.

**Data (Nx x Ny x Nz x 4 bytes):**

float32 distance values in C order. The Z index varies fastest: the value for cell `(i, j, k)` is at index `(i * Ny + j) * Nz + k`.

**Distance convention:**

- Negative: inside the mesh
- Positive: outside the mesh
- Zero: on the surface

## Testing

**C++ tests** (15 suites, via CTest):

```bash
ctest --test-dir build-Release --output-on-failure
```

**Python tests** (51 tests):

```bash
pip install pytest
pytest python/tests -v
```

See [Appendix B](#appendix-b-testing-guide) for details.

## Documentation

- [BUILD.md](BUILD.md) - build instructions for C++ and Python
- [python/README.md](python/README.md) - Python API documentation and examples
- [CHANGELOG.md](CHANGELOG.md) - version history and changes relative to the original SDFGen
- [CONTRIBUTING.md](CONTRIBUTING.md) - how to contribute

## Project Structure

```
SDFGenFast/
├── app/              # CLI application
├── common/           # Shared code (unified API, mesh I/O, mesh repair, SDF I/O, grid sizing)
├── cpu_lib/          # Multi-threaded CPU implementation
├── gpu_lib/          # CUDA GPU implementation
├── python/           # Python bindings
│   ├── sdfgen_py.cpp # nanobind extension source
│   ├── sdfgen.py     # high-level Python API (installed as sdfgen/__init__.py)
│   ├── tests/        # Python test suite (51 tests)
│   └── README.md     # Python API docs
├── sdf_to_mesh/      # SDF-to-mesh converter (marching cubes)
├── tests/            # C++ test suite (15 suites)
├── tools/            # Build scripts (git submodule)
├── BUILD.md
├── CHANGELOG.md
├── CONTRIBUTING.md
└── README.md         # this file
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Ideas for future work:

- Additional output formats (OpenVDB, NanoVDB)
- GPU-accelerated mesh preprocessing
- Distance field gradients
- Multi-GPU support
- Batch processing API

## License and Citation

**License:** MIT (see [LICENSE](LICENSE))

**Original author:** Christopher Batty
**Fork enhancements:** Brad Chamberlain (2025)

If you use this software in academic work, please cite the original:

```
Batty, C. (2015). SDFGen. https://github.com/christopherbatty/SDFGen
```

And if you use the GPU-accelerated version:

```
Chamberlain, B. (2025). SDFGenFast. https://github.com/meet-radek/SDFGenFast
```

---

# Appendices

## Appendix A: Performance Benchmarks

**Test system:** Intel Core i7-13700KF (8 P-cores + 8 E-cores, 24 threads), NVIDIA GeForce RTX 4090, 128 GB RAM, Windows 11, Visual Studio 2026, CUDA 13.3.
**Measured:** 2026-08-31 with `benchmark_performance` (see below). The benchmark loads the 3:4:5 box test mesh and sizes each grid proportionally from Nx with padding 2.

### Full Results

| Nx  | Grid            | Total Cells | CPU (1) | CPU (10) | CPU (20) | CPU (24) | GPU    |
|-----|-----------------|-------------|---------|----------|----------|----------|--------|
| 64  | 64 x 84 x 104   | 559,104     | 698 ms  | 169 ms   | 206 ms   | 261 ms   | 71 ms  |
| 128 | 128 x 169 x 211 | 4,564,352   | 5688 ms | 1043 ms  | 1206 ms  | 1467 ms  | 386 ms |
| 256 | 256 x 340 x 424 | 36,904,960  | 46.1 s  | 7.37 s   | 5.78 s   | 7.04 s   | 814 ms |

### Multi-Threading

Speedup relative to 1 thread:

| Threads | Nx = 64 | Nx = 128 | Nx = 256 |
|---------|---------|----------|----------|
| 10      | 4.1x    | 5.5x     | 6.3x     |
| 20      | 3.4x    | 4.7x     | 8.0x     |
| 24      | 2.7x    | 3.9x     | 6.5x     |

On this CPU (8 performance cores plus 8 efficiency cores), 10 threads gave the best time at Nx = 64 and 128, and 20 threads at Nx = 256. Using all 24 threads was slower than 10 threads in every measured case.

### GPU Speedup

| Nx  | GPU vs 1 thread | GPU vs best CPU time |
|-----|-----------------|----------------------|
| 64  | 9.9x            | 2.4x                 |
| 128 | 14.7x           | 2.7x                 |
| 256 | 56.6x           | 7.1x                 |

The GPU was the fastest backend at every measured size on this system, and its advantage grows with grid size.

### Recommendations

- Use the GPU when one is available. The automatic backend selection does this for you.
- On the CPU, more threads is not always faster. Around 10 threads was the best general setting on this system; measure on yours.

Set the thread count with `-t` (default 0 = auto-detect):

```bash
SDFGen mesh.obj 0.1 -t 10   # 10 threads
SDFGen mesh.stl 128         # auto-detect
```

### Run the Benchmarks Yourself

The benchmark executable is not built by default. Enable it, then run it:

```bash
cmake -B build-Release -DSDFGEN_BUILD_BENCHMARKS=ON
cmake --build build-Release --target benchmark_performance
./build-Release/bin/benchmark_performance
```

---

## Appendix B: Testing Guide

### C++ Test Suites (15)

| Suite | Covers |
|-------|--------|
| `test_correctness` | CPU results on a known mesh; CPU/GPU agreement when a GPU is present |
| `test_file_io` | SDF file read and write |
| `test_stl_file_io`, `test_obj_file_io` | End-to-end STL and OBJ processing |
| `test_ascii_stl` | ASCII STL parsing |
| `test_mode1_legacy` | The original OBJ + dx mode |
| `test_cli_modes`, `test_cli_formats`, `test_cli_backend`, `test_cli_output`, `test_cli_errors`, `test_cli_threads` | CLI integration: every usage mode, format handling, backend selection, output files, error handling, and thread flags |
| `test_thread_slice_ratios` | Threading edge cases (more threads than slices, odd ratios) |
| `test_vtk_output` | SDF write/read round-trip losslessness |
| `test_mesh_repair` | Watertightness analysis and hole filling |

Run all suites with CTest:

```bash
ctest --test-dir build-Release --output-on-failure
```

Tests locate their resources and the SDFGen binary through paths baked in at build time, so CTest can run from any directory. GPU comparisons skip automatically when CUDA is not built or no GPU is present.

### Python Test Suite (51 tests)

```bash
pip install pytest
pytest python/tests -v                                    # all tests
pytest python/tests/test_sdfgen.py::TestBackends -v       # one class
```

| Test class | Tests | Covers |
|------------|-------|--------|
| TestBasicFunctionality | 5 | Core API functions |
| TestBackends | 4 | CPU/GPU backend selection |
| TestParameters | 5 | Parameter variations |
| TestErrorHandling | 3 | Error conditions |
| TestSDFProperties | 2 | SDF correctness |
| TestCriticalErrorHandling | 8 | Invalid inputs |
| TestHighLevelAPIParameters | 10 | High-level API |
| TestDataValidation | 6 | Data type handling |
| TestEdgeCases | 8 | Boundary conditions |

### Test Resources

Test meshes are in `tests/resources/`: binary STL, ASCII STL, OBJ (triangles and quads), and one reference SDF fixture.

### Writing New Tests

**C++:** add a `test_*.cpp` file in `tests/`, then add one line to the test table in `tests/CMakeLists.txt`. Use the helpers in `test_utils` (library tests) or `cli_test_utils` (CLI tests).

**Python:** add tests to `python/tests/test_sdfgen.py`. Use the existing fixtures (`simple_cube`, `temp_obj_file`, `temp_sdf_file`).
