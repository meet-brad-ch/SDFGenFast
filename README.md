# SDFGen

A high-performance command-line utility and Python library for generating grid-based signed distance fields (SDFs) from triangle meshes.

**Enhanced fork** of [Christopher Batty's original SDFGen](https://github.com/christopherbatty/SDFGen) with GPU acceleration, automatic hardware detection, Python bindings, and cross-platform build system.

## Features

- **Automatic GPU Acceleration**: Detects and uses CUDA-capable GPUs automatically (no manual flags needed)
- **Fast CPU Fallback**: Multi-threaded CPU implementation when GPU is unavailable
- **Mesh Watertightness Check**: Automatic detection of holes and non-manifold edges
- **Mesh Repair**: Optional hole-filling with `--fix` flag for non-watertight meshes
- **Python Bindings**: High-performance nanobind-based API with NumPy integration and GPU support
- **Multiple Input Formats**: Binary/ASCII STL and Wavefront OBJ (quads automatically triangulated)
- **Flexible Grid Sizing**: Proportional or manual dimension specification
- **Binary SDF Output**: Compact binary format with metadata header
- **Cross-Platform**: Windows (MSVC) and Linux (GCC/Clang) with automated build scripts
- **Comprehensive Testing**: C++ tests + 51 Python tests validating all functionality

## Quick Start

### Installation

**From source (see [BUILD.md](BUILD.md) for detailed instructions):**

```bash
# Linux
cd tools
./configure.sh Release && ./build.sh

# Windows
cd tools
configure.bat Release && build.bat all
```

**Python bindings:**
```bash
pip install .
```

### CLI Usage

**Mode 1: OBJ files with explicit cell size (dx)**
```bash
SDFGen mesh.obj 0.01 2
```
- `0.01` = grid cell size (meters)
- `2` = padding (cells around mesh)

**Mode 2a: STL files with proportional sizing (Nx only)**
```bash
SDFGen mesh.stl 128 1
```
- `128` = target X-dimension
- `1` = padding
- Y and Z dimensions calculated proportionally

**Mode 2b: STL files with explicit dimensions**
```bash
SDFGen mesh.stl 128 256 64 1
```
- Grid dimensions: 128 × 256 × 64
- `1` = padding

**Optional thread parameter (all modes):**
```bash
SDFGen mesh.obj 0.01 2 10    # Use 10 threads
SDFGen mesh.stl 128 1 0      # Auto-detect thread count
```

**Optional flags:**
```bash
SDFGen --fix mesh.stl 128    # Repair non-watertight meshes (fill holes)
SDFGen --cpu mesh.stl 128    # Force CPU backend (skip GPU)
SDFGen --fix --cpu mesh.stl 128  # Both flags
```

**SDF to Mesh conversion** (for debugging/visualization):
```bash
sdf_to_mesh input.sdf output.obj           # Extract surface mesh
sdf_to_mesh input.sdf output.obj -i 0.5    # Extract isosurface at distance 0.5
```

**Mesh watertightness** is always checked and reported:
```
Mesh Analysis:
  Boundary edges:     3632 (holes detected)
  Number of holes:    41
  Is watertight:      NO

  WARNING: Mesh is not watertight. SDF sign determination may be incorrect.
           Use --fix flag to attempt automatic hole filling.
```

### Python Usage

```python
import sdfgen
import numpy as np

# Generate from file
sdf, metadata = sdfgen.generate_from_file(
    "mesh.stl",
    nx=256,           # Proportional sizing
    padding=2,
    backend="auto"    # Uses GPU if available
)

# Generate from arrays
vertices, triangles, bounds = sdfgen.load_mesh("mesh.obj")
sdf = sdfgen.generate_sdf(
    vertices, triangles,
    origin=(0, 0, 0),
    dx=0.01,
    nx=100, ny=100, nz=100
)

# Save to file
sdfgen.save_sdf("output.sdf", sdf, origin=(0, 0, 0), dx=0.01)

# Check GPU availability
print(f"GPU available: {sdfgen.is_gpu_available()}")
```

**See [python/README.md](python/README.md) for complete API documentation.**

## Hardware Acceleration

SDFGen automatically detects and uses GPU acceleration — **no manual configuration required**.

**Build-time detection:**
- CMake searches for CUDA Toolkit during configuration
- GPU support compiled in automatically if CUDA found

**Runtime detection:**
```bash
./SDFGen mesh.stl 128
```

Output shows detected hardware:
```
Hardware: GPU acceleration available
Implementation: GPU (CUDA)
```

or

```
Hardware: No CUDA GPU detected
Implementation: CPU (multi-threaded)
```

**PyTorch-style simplicity**: No `--gpu` flags, no configuration files. It just works.

## Performance

**Quick Summary** (Intel i9-13900K + RTX 4090):

| Grid Size | CPU (1 thread) | CPU (20 threads) | GPU     | GPU Speedup |
|-----------|----------------|------------------|---------|-------------|
| 64³       | 738 ms         | 93 ms            | 111 ms  | 6.7x        |
| 128³      | 5.98 s         | 748 ms           | 358 ms  | 16.7x       |
| 256³      | 48.6 s         | 4.18 s           | 1.29 s  | 37.6x       |

- **Multi-threading scales well** to 10-20 threads
- **GPU advantage increases** with grid size
- **Best performance**: GPU for large grids, 10-20 threads for smaller grids

**See [Appendix A: Performance Benchmarks](#appendix-a-performance-benchmarks) for detailed results.**

## Output Format

Binary SDF file format:

**Header (36 bytes):**
```
[Nx, Ny, Nz]           (3 × int32)   - Grid dimensions
[xmin, ymin, zmin]     (3 × float32) - Bounding box minimum
[xmax, ymax, zmax]     (3 × float32) - Bounding box maximum
```

**Data (Nx × Ny × Nz × 4 bytes):**
```
float32 distance values in Z-major order
```

**Distance convention:**
- Negative: Inside the mesh
- Positive: Outside the mesh
- Zero: On the surface

## Testing

**C++ Tests (15 tests):**
```bash
# Build tests (if not already built)
cd tools && ./build.bat all  # or ./build.sh

# Run tests
cd ../build-Release/bin
./test_correctness.exe
./test_file_io.exe
# ... etc (all should show ✓ PASSED)
```

**Python Tests (51 tests):**
```bash
pip install pytest
pytest python/tests/test_sdfgen.py -v
# Should show: 51 passed
```

**See [Appendix B: Testing Guide](#appendix-b-testing-guide) for details.**

## Documentation

- **[BUILD.md](BUILD.md)** - Complete build instructions for C++ and Python
- **[python/README.md](python/README.md)** - Python API documentation and examples
- **README.md Appendices** (below) - Benchmarks, testing details, and changelog

## Project Structure

```
SDFGenFast/
├── app/              # CLI application
├── common/           # Shared utilities (unified API, I/O, mesh repair)
│   ├── mesh_io.*     # OBJ/STL file loading
│   ├── mesh_repair.* # Watertightness check and hole filling
│   ├── sdf_io.*      # SDF file I/O
│   └── sdfgen_unified.* # Unified CPU/GPU API
├── cpu_lib/          # Multi-threaded CPU implementation
├── gpu_lib/          # CUDA GPU implementation
├── python/           # Python bindings (nanobind + NumPy)
│   ├── sdfgen_py.cpp
│   ├── __init__.py
│   ├── tests/test_sdfgen.py    # 51 tests
│   └── README.md               # Python API docs
├── tests/            # C++ test suite
├── tools/            # Build scripts (external submodule)
├── sdf_to_mesh/      # SDF to mesh converter (marching cubes)
├── BUILD.md          # Build instructions
└── README.md         # This file
```

## Contributing

Ideas for future contributions:
- Additional output formats (OpenVDB, NanoVDB)
- GPU-accelerated mesh preprocessing
- Distance field gradients
- Multi-GPU support
- Batch processing API

## License and Citation

**License:** BSD 3-Clause (see LICENSE file)

**Original Author:** Christopher Batty
**Fork Enhancements:** Brad Chamberlain (2025)

If you use this software in academic work, please cite the original:
```
Batty, C. (2015). SDFGen. https://github.com/christopherbatty/SDFGen
```

And if you use the GPU-accelerated version:
```
Chamberlain, B. (2025). SDFGenFast. https://github.com/yourusername/SDFGenFast
```

---

# Appendices

## Appendix A: Performance Benchmarks

Comprehensive benchmarks showing multi-threading scaling and GPU acceleration.

**Test System:**
- CPU: Intel i9-13900K (24 cores / 32 threads)
- GPU: NVIDIA RTX 4090 (24GB VRAM)
- RAM: 64GB DDR5
- OS: Windows 11

### Full Benchmark Results

| Grid Size | Total Cells | CPU (1 thread) | CPU (10 threads) | CPU (20 threads) | GPU      |
|-----------|-------------|----------------|------------------|------------------|----------|
| 64³       | 559K        | 738 ms         | 127 ms           | 93 ms            | 111 ms   |
| 128³      | 4.6M        | 5.98 s         | 789 ms           | 748 ms           | 358 ms   |
| 256³      | 36.9M       | 48.6 s         | 6.95 s           | 4.18 s           | 1.29 s   |

### Multi-Threading Efficiency

**Speedup vs. 1 thread:**

| Threads | 64³ Grid | 128³ Grid | 256³ Grid | Avg Efficiency |
|---------|----------|-----------|-----------|----------------|
| 10      | 5.8x     | 7.6x      | 7.0x      | 58-76%         |
| 20      | 7.9x     | 8.0x      | 11.6x     | 40-58%         |

**Key insights:**
- Multi-threading scales well up to 10 threads (58-76% efficiency)
- Efficiency drops at 20 threads (40-58%) due to memory bandwidth saturation
- Best CPU performance typically at 10-15 threads

### GPU Acceleration

**Speedup vs. single-threaded CPU:**

| Grid Size | GPU Speedup |
|-----------|-------------|
| 64³       | 6.7x        |
| 128³      | 16.7x       |
| 256³      | 37.6x       |

**Speedup vs. 20-thread CPU:**

| Grid Size | GPU vs 20-thread |
|-----------|------------------|
| 64³       | 0.8x (slower)    |
| 128³      | 2.1x             |
| 256³      | 3.2x             |

**Key insights:**
- GPU advantage increases dramatically with grid size
- Small grids (< 128³): Multi-threaded CPU competitive
- Large grids (≥ 256³): GPU provides substantial speedup
- Memory transfer overhead visible on small grids

### Recommendations

**For maximum performance:**

| Grid Size | Recommended Configuration |
|-----------|--------------------------|
| < 64³     | CPU with 10 threads      |
| 64³-128³  | CPU with 10-20 threads   |
| > 128³    | GPU (if available)       |

**Thread Configuration:**
- Default behavior: Auto-detects CPU core count
- Manual override: Add thread count as last parameter
- Examples:
  ```bash
  SDFGen mesh.obj 0.1 2 10    # Force 10 threads
  SDFGen mesh.stl 128 1 0     # Auto-detect (default)
  ```

### Run Benchmarks Yourself

```bash
cd tests
../build-Release/bin/benchmark_performance
```

---

## Appendix B: Testing Guide

SDFGen includes comprehensive test coverage for both C++ and Python components.

### C++ Test Suite (15 tests)

**Test Categories:**

1. **Correctness Tests (3)**
   - `test_correctness` - Validates CPU implementation (and CPU/GPU agreement when GPU available)
   - `test_file_io` - Tests SDF file read/write operations
   - `test_mode1_legacy` - Validates legacy OBJ+dx mode

2. **CLI Integration Tests (6)**
   - `test_cli_modes` - All CLI usage modes
   - `test_cli_backend` - Auto backend detection
   - `test_cli_formats` - STL/OBJ format support
   - `test_cli_output` - Output file generation
   - `test_cli_errors` - Error handling
   - `test_cli_threads` - Thread parameter handling

3. **File Format Tests (3)**
   - `test_stl_file_io` - Binary STL processing
   - `test_obj_file_io` - OBJ file processing
   - `test_ascii_stl` - ASCII STL support

4. **Library Tests (1)**
   - `test_mesh_repair` - Mesh watertightness analysis and repair API

5. **Edge Case Tests (2)**
   - `test_thread_slice_ratios` - Threading edge cases
   - `test_vtk_output` - VTK format support (if compiled)

**Running C++ Tests:**

```bash
# Windows
cd build-Release/bin
test_correctness.exe
test_file_io.exe
test_cli_modes.exe
# ... run each test (all should show ✓ PASSED)

# Linux
cd build-Release/bin
./test_correctness
./test_file_io
./test_cli_modes
# ... etc
```

**Expected output:**
- Each test outputs `✓ PASSED` or `✓ ALL TESTS PASSED`
- Zero failures expected on supported platforms

**Note:** Tests must be run from the `build-Release/bin/` directory where test resources are located.

**Note:** All tests work on CPU-only builds. GPU-specific tests (like `test_correctness` CPU/GPU comparison) automatically skip GPU validation when CUDA is not available or no GPU is detected.

### Python Test Suite (51 tests)

**Test Coverage:**

| Test Class | Tests | Coverage |
|------------|-------|----------|
| TestBasicFunctionality | 5 | Core API functions |
| TestBackends | 4 | CPU/GPU backend selection |
| TestParameters | 5 | Parameter variations |
| TestErrorHandling | 3 | Error conditions |
| TestSDFProperties | 2 | SDF correctness |
| TestCriticalErrorHandling | 8 | Invalid inputs |
| TestHighLevelAPIParameters | 10 | High-level API |
| TestDataValidation | 6 | Data type handling |
| TestEdgeCases | 8 | Boundary conditions |

**Running Python Tests:**

```bash
# Install pytest
pip install pytest

# Run all tests
pytest python/tests/test_sdfgen.py -v

# Run specific test class
pytest python/tests/test_sdfgen.py::TestBackends -v

# Run specific test
pytest python/tests/test_sdfgen.py::TestBackends::test_cpu_backend -v
```

**Expected output:**
```
============================== 51 passed in 0.49s ==============================
```

### Test Resources

Test meshes located in `tests/resources/`:
- Binary STL files
- ASCII STL files
- OBJ files (triangles and quads)
- Reference SDF outputs

### Writing New Tests

**C++ tests:**
- Add to `tests/` directory
- Update `tests/CMakeLists.txt`
- Use `test_utils` library for common functionality

**Python tests:**
- Add to `python/tests/test_sdfgen.py`
- Use pytest fixtures: `simple_cube`, `temp_obj_file`, `temp_sdf_file`
- Follow existing test patterns

---

## Appendix C: What's New in This Fork

This enhanced version adds significant improvements over the [original SDFGen](https://github.com/christopherbatty/SDFGen).

### Comparison Table

| Feature | Original SDFGen | This Fork |
|---------|----------------|-----------|
| **Performance** | Single-threaded CPU | Multi-threaded CPU + GPU (CUDA) |
| **Input Formats** | OBJ only | OBJ + Binary/ASCII STL |
| **Grid Sizing** | Manual dx calculation | Auto-proportional + manual modes |
| **Build System** | Basic Makefile | CMake 3.20+ with auto-detection |
| **Platforms** | Linux/Mac | Windows + Linux with automated scripts |
| **GPU Support** | None | Automatic detection and usage |
| **Python API** | None | nanobind bindings with NumPy + GPU |
| **Testing** | None | 15 C++ + 51 Python tests |
| **Documentation** | Basic | Complete build guide + examples |
| **Speed (256³)** | ~20 seconds | 1.29s GPU / 4.18s CPU (20 threads) |

### Performance Enhancements

- **Multi-threaded CPU**: Parallel implementation (6-10x faster than single-threaded)
- **GPU Acceleration**: CUDA implementation with 1.1-5.4x additional speedup over multi-threaded CPU
- **Automatic Backend Selection**: PyTorch-style automatic GPU detection (no manual flags)
- **Fast Sweeping Algorithm**: Optimized GPU implementation of Eikonal solver

### New Features

- **Python Bindings**: High-performance nanobind API with NumPy integration and GPU support
- **STL Support**: Binary and ASCII STL file formats (original only supported OBJ)
- **Mesh Repair**: Automatic watertightness detection with optional hole-filling (`--fix`)
- **Flexible Grid Modes**: Proportional dimension calculation for STL files
- **Improved CLI**: Multiple usage modes with better parameter handling
- **Thread Control**: Configurable CPU thread count for optimal performance
- **Comprehensive Testing**: Full test coverage of all functionality

### Build System Improvements

- **Cross-Platform**: Automated build scripts for Windows (MSVC) and Linux (GCC/Clang)
- **CMake 3.20+**: Modern CMake with automatic CUDA detection
- **Professional Tooling**: Configuration scripts, test suite, and comprehensive documentation
- **Auto-Detection**: Build-time CUDA detection, runtime GPU detection
- **Better Errors**: Clear error messages with installation instructions

### Developer Experience

- **No Manual Configuration**: GPU detection happens automatically
- **Clear Documentation**: Complete build guide, API docs, and usage examples
- **Comprehensive Tests**: Validate correctness across all features and platforms
- **Simple API**: Both CLI and Python API designed for ease of use

---

**Version:** 2.2.0
**Last Updated:** 2025-12-26
**Tested On:** Windows 11, Ubuntu 22.04, Python 3.10-3.12, CUDA 12.4
