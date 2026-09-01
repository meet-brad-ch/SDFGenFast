# Building SDFGenFast

Build guide for the C++ library, CLI tool, and Python bindings.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Cloning the Repository](#cloning-the-repository)
- [Building the C++ Library and CLI](#building-the-c-library-and-cli)
- [Building the Python Bindings](#building-the-python-bindings)
- [Running Tests](#running-tests)
- [Build Options](#build-options)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required

| Component | Requirement |
|-----------|-------------|
| C++ compiler | C++17: Visual Studio 2022 or 2026 (Windows), GCC 9+ or Clang 10+ (Linux) |
| CMake | 3.23 or newer |
| Build system | Ninja (recommended) or Make; Ninja ships with Visual Studio |

### Optional

| Component | Requirement |
|-----------|-------------|
| CUDA Toolkit (GPU support) | 12.x with Visual Studio 2022 or GCC; **13.x required for Visual Studio 2026** (see [Troubleshooting](#troubleshooting)) |
| Python (bindings) | 3.8 or newer |

**Windows:** install the "Desktop development with C++" workload in the Visual Studio Installer.

**Ubuntu/Debian:**

```bash
sudo apt-get install build-essential cmake ninja-build git
```

**CUDA Toolkit:** download from https://developer.nvidia.com/cuda-downloads. CMake detects it automatically; no configuration is needed. Verify with `nvcc --version`.

---

## Cloning the Repository

The build scripts in `tools/` are a git submodule. Clone with:

```bash
git clone --recurse-submodules https://github.com/meet-radek/SDFGenFast
```

If you already cloned without submodules:

```bash
git submodule update --init
```

---

## Building the C++ Library and CLI

### Option A: Plain CMake (any platform)

Run from a shell where your compiler is available (on Windows: a "Developer Command Prompt" or a shell after `vcvarsall.bat x64`):

```bash
cmake -B build-Release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-Release
```

The build type defaults to `Release` when you omit `-DCMAKE_BUILD_TYPE`.

### Option B: Wrapper scripts

The `tools/` submodule provides scripts that locate Visual Studio (2026 or 2022) and set up the environment for you:

```
# Windows - run from cmd.exe or PowerShell, not from Git Bash
cd tools
configure.bat Release
build.bat all Release
```

```bash
# Linux
cd tools
./configure.sh Release
./build.sh all
```

**Windows note:** run the `.bat` scripts from a real `cmd.exe` or PowerShell window. Git Bash invokes them through a bridge that breaks their internal path resolution.

### Output

- `build-Release/bin/SDFGen` (`.exe` on Windows) - the CLI tool
- `build-Release/bin/sdf_to_mesh` - the SDF-to-mesh converter
- `build-Release/bin/test_*` - test executables
- `build-Release/lib/` - static libraries

During configuration, CMake reports whether CUDA was found and whether GPU support will be compiled.

---

## Building the Python Bindings

### Method 1: pip install (recommended)

```bash
pip install .
```

This builds the C++ extension and installs the `sdfgen` package. The build dependencies (scikit-build-core, nanobind) are downloaded automatically; the installed package depends on NumPy. On Windows with CUDA, the CUDA runtime DLL is bundled into the package.

### Method 2: In-tree CMake build

```bash
cmake -B build-Release -DCMAKE_BUILD_TYPE=Release -DBUILD_PYTHON_BINDINGS=ON -DPython_EXECUTABLE=/path/to/python
cmake --build build-Release
```

This builds the extension and assembles an importable package in `sdfgen/` at the project root (used by the test suite; it is not an installed package).

### Verify

```bash
python -c "import sdfgen; print(sdfgen.__version__)"
python -c "import sdfgen; print('GPU:', sdfgen.is_gpu_available())"
```

See [python/README.md](python/README.md) for the API documentation.

---

## Running Tests

### C++ (15 suites)

```bash
ctest --test-dir build-Release --output-on-failure
```

Tests locate their resources and the SDFGen binary through paths baked in at build time, so CTest can run from any directory. GPU comparisons skip automatically when CUDA is not built or no GPU is present at run time. You can also run a single test executable directly from `build-Release/bin/`.

### Python (51 tests)

```bash
pip install pytest
pytest python/tests -q
```

---

## Build Options

Pass these to the `cmake -B` configure step.

| Option | Default | Effect |
|--------|---------|--------|
| `-DSDFGEN_BUILD_GPU=OFF` | `ON` | Build without CUDA, even when a toolkit is installed |
| `-DBUILD_PYTHON_BINDINGS=ON` | `OFF` | Build the Python extension |
| `-DBUILD_TESTING=OFF` | `ON` | Skip building the test suite |
| `-DSDFGEN_BUILD_BENCHMARKS=ON` | `OFF` | Build the `benchmark_performance` executable |
| `-DSDFGEN_WARNINGS_AS_ERRORS=ON` | `OFF` | Treat compiler warnings as errors |
| `-DSDFGEN_CUDA_ARCHITECTURES=...` | `all-major` | CUDA architectures to compile for, e.g. `"80;89"` |
| `-DPython_EXECUTABLE=...` | auto | Python interpreter for the bindings |
| `-DCMAKE_BUILD_TYPE=Debug` | `Release` | Debug build with symbols and assertions |

Common CUDA architecture values: `75` Turing (RTX 20xx), `80` Ampere (RTX 30xx), `89` Ada Lovelace (RTX 40xx). The default `all-major` covers all major architectures the installed CUDA toolkit supports.

---

## Troubleshooting

### Visual Studio 2026 requires CUDA 13

CUDA 12.x cannot compile this project with the Visual Studio 2026 standard-library headers: `cudafe++` crashes while parsing them. Install CUDA 13.x for VS 2026. With VS 2022, CUDA 12.x works.

If CMake picks up the wrong CUDA installation, point it at the right one:

```bash
cmake -B build-Release -DCUDAToolkit_ROOT="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.3" -DCMAKE_CUDA_COMPILER="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.3/bin/nvcc.exe"
```

### CUDA not detected

```bash
nvcc --version       # is the toolkit on PATH?
nvidia-smi           # is the driver installed? (also shows the GPU)
```

On Windows, add the CUDA `bin` directory to `PATH`. On Linux, add to `~/.bashrc`:

```bash
export PATH=/usr/local/cuda/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
```

### Python extension import fails

- **Windows, `DLL load failed`:** the package needs the CUDA runtime DLL (`cudart64_12.dll` or `cudart64_13.dll`). `pip install .` bundles it; for other setups, add the CUDA `bin` directory (CUDA 13: `bin\x64`) to `PATH`.
- **Linux, `undefined symbol`:** export `LD_LIBRARY_PATH` as shown above.
- **`ModuleNotFoundError: No module named 'sdfgen'`:** run `pip install .` from the project root.

### Visual Studio not found by the wrapper scripts

- Ensure the "Desktop development with C++" workload is installed.
- Run the scripts from a real `cmd.exe` or PowerShell window, not from Git Bash.
- If detection still fails, use plain CMake from a Developer Command Prompt (Option A above).

### Slow builds on Windows

Add `build-Release/` to your antivirus exclusions; scanning build artifacts slows compilation considerably.

### Clean rebuild

```bash
rm -rf build-Release
cmake -B build-Release -DCMAKE_BUILD_TYPE=Release
cmake --build build-Release
```

---

## Verified Configurations

- **Windows 11, Visual Studio 2026 Community, CUDA 13.3.1, Python 3.10** - verified 2026-08-31: full build, 15/15 C++ suites, 51/51 Python tests, GPU enabled (RTX 4090).
- **Windows 11, Visual Studio 2022, CUDA 12.4** - used for earlier releases of this fork.
- **Ubuntu 22.04, GCC** - used for earlier releases of this fork; the current revision has not been re-verified on Linux.
