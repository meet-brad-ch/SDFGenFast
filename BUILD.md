# Building SDFGenFast

Complete build guide for the C++ library, CLI tool, and Python bindings.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Building C++ Library and CLI](#building-c-library-and-cli)
- [Building Python Bindings](#building-python-bindings)
- [Running Tests](#running-tests)
- [Troubleshooting](#troubleshooting)
- [Advanced Options](#advanced-options)

---

## Prerequisites

### Required Software

| Component | Version | Windows | Linux |
|-----------|---------|---------|-------|
| **C++ Compiler** | C++17 | Visual Studio 2022 | GCC 9+ / Clang 10+ |
| **CMake** | 3.20+ | ✓ | ✓ |
| **Build System** | | Ninja (recommended) | Ninja / Make |
| **Python** (optional) | 3.8+ | ✓ | ✓ |
| **CUDA Toolkit** (optional) | 11.0+ | Auto-detected | Auto-detected |

### Windows Prerequisites

**Visual Studio 2022:**
- Download from https://visualstudio.microsoft.com/downloads/
- Install "Desktop development with C++" workload
- Ninja build system included with VS 2022

**Verification:**
```powershell
cmake --version    # Should show 3.20+
ninja --version    # Should show 1.10+
```

### Linux Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build git
```

**Fedora/RHEL:**
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake ninja-build git
```

**Verification:**
```bash
cmake --version    # Should show 3.20+
g++ --version      # Should show GCC 9+
ninja --version    # Should show 1.10+
```

### Optional: GPU Acceleration

**CUDA Toolkit** (automatically detected if installed):
- Download from https://developer.nvidia.com/cuda-downloads
- Follow platform-specific installer instructions
- No manual configuration needed - CMake auto-detects CUDA

**Verify CUDA:**
```bash
nvcc --version     # Should show CUDA 11.0+
nvidia-smi         # Should show GPU info (Linux)
```

---

## Cloning the Repository

This project uses git submodules. Clone with:

```bash
git clone --recurse-submodules https://github.com/meet-brad-ch/SDFGenFast
```

If you already cloned without submodules:

```bash
git submodule update --init
```

---

## Quick Start

### Windows
```powershell
# Navigate to tools directory
cd tools

# Configure build
./configure.bat Release

# Build everything
./build.bat all Release

# Executables created in build-Release/bin/
```

### Linux
```bash
# Navigate to tools directory
cd tools

# Make scripts executable
chmod +x configure.sh build.sh

# Configure build
./configure.sh Release

# Build everything
./build.sh all

# Executables created in build-Release/bin/
```

---

## Building C++ Library and CLI

### Step 1: Configure the Build

The configuration phase generates build files and detects CUDA.

**Windows:**
```powershell
cd tools
./configure.bat Release
```

**Linux:**
```bash
cd tools
chmod +x configure.sh
./configure.sh Release
```

**What it does:**
- Detects Visual Studio (Windows) or GCC/Clang (Linux)
- Searches for CUDA Toolkit (auto-detected)
- Generates build files in `build-Release/`
- Creates Ninja or Make build scripts

**Output:**
- Creates `build-Release/` directory
- Displays detected compilers and CUDA status
- Reports whether GPU support will be compiled

### Step 2: Build

**Windows:**
```powershell
cd tools
./build.bat all Release
```

**Linux:**
```bash
cd tools
./build.sh all
```

**Optional: Build specific targets:**
```bash
# Windows
./build.bat SDFGen Release      # CLI only
./build.bat test_correctness    # Specific test

# Linux
./build.sh SDFGen                       # CLI only
./build.sh test_correctness             # Specific test
```

**Output:**
- `build-Release/bin/SDFGen.exe` (or `SDFGen` on Linux) - CLI tool
- `build-Release/lib/` - Static libraries
- `build-Release/bin/test_*.exe` - Test executables

### Step 3: Verify Build

**Run the CLI:**
```bash
# Windows
cd build-Release/bin
./SDFGen.exe mesh.stl 128

# Linux
cd build-Release/bin
./SDFGen mesh.stl 128
```

---

## Building Python Bindings

### Prerequisites

**Python packages:**
```bash
pip install numpy nanobind
```

### Method 1: pip install (Recommended)

```bash
# From project root
pip install .

# Or for development (editable install)
pip install -e .
```

This automatically:
- Runs CMake configuration
- Builds the C++ extension
- Installs the `sdfgen` package

### Method 2: Manual Build

**Windows:**
```powershell
# 1. Configure CMake (if not done already)
cd tools
./configure.bat Release

# 2. Enable Python bindings
cd ../build-Release
cmake -DBUILD_PYTHON_BINDINGS=ON -DPython_EXECUTABLE="C:/Python310/python.exe" .

# 3. Build Python extension
cd ../tools
./build.bat sdfgen_ext Release
```

**Linux:**
```bash
# 1. Configure CMake (if not done already)
cd tools
./configure.sh Release

# 2. Enable Python bindings
cd ../build-Release
cmake -DBUILD_PYTHON_BINDINGS=ON .

# 3. Build Python extension
cd ../tools
./build.sh sdfgen_ext
```

**Output:**
- Windows: `build-Release/lib/sdfgen_ext.cp310-win_amd64.pyd`
- Linux: `build-Release/lib/sdfgen_ext.cpython-310-x86_64-linux-gnu.so`

### Verify Python Build

```python
python -c "import sdfgen; print(sdfgen.__version__)"
python -c "import sdfgen; print('GPU:', sdfgen.is_gpu_available())"
```

**See `python/README.md` for detailed API documentation and usage examples.**

---

## Running Tests

### C++ Tests

**Run tests from build directory:**
```bash
# Windows
cd build-Release/bin
test_correctness.exe
test_file_io.exe
test_stl_file_io.exe
test_obj_file_io.exe
test_ascii_stl.exe
test_mode1_legacy.exe
test_thread_slice_ratios.exe
test_cli_modes.exe
test_cli_formats.exe
test_cli_backend.exe
test_cli_output.exe
test_cli_errors.exe
test_cli_threads.exe

# Linux
cd build-Release/bin
./test_correctness
./test_file_io
# ... etc
```

**All tests should output `✓ PASSED` or `✓ ALL TESTS PASSED`**

**Note:** Tests must be run from the `build-Release/bin/` directory where test resources are located.

**Note:** Tests work on CPU-only builds. GPU-specific tests automatically skip GPU validation when CUDA is not built or GPU is not available at runtime.
**Note:** Tests can be run from any directory - resource files are automatically located.

**Alternative: Run all tests via CTest:**
```bash
cd build-Release
ctest --output-on-failure
```
This runs all 14 tests and shows detailed output only for failures.

### Python Tests

```bash
# Install pytest if needed
pip install pytest

# Run Python test suite (from project root)
pytest python/tests/test_sdfgen.py -v

# Should show: "51 passed in X.XXs"
```

---

## Troubleshooting

### Common Issues (All Platforms)

**CMake not found:**
```bash
# Windows: Install Visual Studio 2022 with CMake component
# Linux: sudo apt-get install cmake
```

**CUDA not detected (when GPU is available):**
```bash
# Verify CUDA is in PATH
nvcc --version

# Windows: Add CUDA to PATH
# set PATH=%PATH%;C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4\bin

# Linux: Add to ~/.bashrc
# export PATH=/usr/local/cuda/bin:$PATH
# export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
```

**Build fails with "No such file or directory":**
- Ensure you're in the `tools/` directory when running build scripts
- Scripts expect `CMakeLists.txt` in parent directory

### Windows-Specific Issues

**Visual Studio not found:**
- Verify installation: `ls "C:\Program Files\Microsoft Visual Studio\2022\Community"`
- Ensure "Desktop development with C++" workload is installed
- Run scripts from VS Developer Command Prompt if auto-detection fails

**Python extension import fails (DLL not found):**
```powershell
# Copy CUDA runtime DLL to package directory
copy "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4\bin\cudart64_12.dll" sdfgen\
```

### Linux-Specific Issues

**Permission denied on scripts:**
```bash
chmod +x tools/configure.sh tools/build.sh
```

**GPU not detected at runtime:**
```bash
# Check NVIDIA driver
nvidia-smi

# Check CUDA libraries
ldd build-Release/bin/SDFGen | grep cuda
```

**Python import error (undefined symbol):**
```bash
export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
# Add to ~/.bashrc to make permanent
```

### Slow Builds

**Use Ninja instead of Make (Linux):**
```bash
sudo apt-get install ninja-build
./configure.sh Release  # Auto-detects Ninja
```

**Use parallel builds:**
```bash
# Windows: Already enabled (24 parallel jobs by default)
# Linux: ./build.sh uses all CPU cores automatically
```

**Exclude build directory from antivirus (Windows):**
- Scanning build artifacts significantly slows compilation
- Add `build-Release/` to Windows Defender exclusions

---

## Advanced Options

### Custom Python Installation

```bash
cmake -DBUILD_PYTHON_BINDINGS=ON \
      -DPython_EXECUTABLE=/path/to/python3.10 \
      ..
```

### Specify CUDA Architecture

```bash
cmake -DCMAKE_CUDA_ARCHITECTURES="75;80;89" ..
```

Common architectures:
- `75` - Turing (RTX 20xx, GTX 16xx)
- `80` - Ampere (RTX 30xx)
- `89` - Ada Lovelace (RTX 40xx)

### Force CPU-Only Build

```bash
cmake -DSDFGEN_BUILD_GPU=OFF ..
```

### Debug Build

```bash
# Windows
./configure.bat Debug
./build.bat all Debug

# Linux
./configure.sh Debug
./build.sh all Debug
```

Creates `build-Debug/` with debug symbols and assertions enabled.

### Clean Rebuild

```bash
# Option 1: Clean target
./build.bat clean  # Windows
./build.sh clean           # Linux

# Option 2: Delete build directory
rm -rf build-Release
./configure.bat Release  # Reconfigure from scratch
```

---

## File Locations

After building, your project structure will be:

```
SDFGenFast/
├── build-Release/
│   ├── bin/
│   │   ├── SDFGen.exe              # CLI tool
│   │   └── test_*.exe              # Test executables
│   └── lib/
│       ├── sdfgen_ext.*.pyd/.so    # Python extension (if built)
│       ├── sdfgen_common.lib       # Static libraries
│       ├── sdfgen_cpu.lib
│       └── sdfgen_gpu.lib
├── sdfgen/                         # Python package (if installed)
│   ├── __init__.py
│   └── sdfgen_ext.*.pyd/.so
└── tools/                          # Build scripts
    ├── configure.bat / configure.sh
    └── build.bat / build.sh
```

---

## Next Steps

- **Usage Guide:** See [README.md](README.md) for CLI and Python usage examples
- **Python API:** See [python/README.md](python/README.md) for detailed API documentation
- **Test Details:** See README.md Appendix B for test suite information
- **Performance Data:** See README.md Appendix A for benchmarks

---

## Support

**For build issues:**
1. Check this troubleshooting section above
2. Verify prerequisites are correctly installed
3. Try a clean rebuild (delete `build-Release/` and reconfigure)
4. Check CMake output for specific error messages

**For runtime issues:**
1. Verify GPU availability with `nvidia-smi` (if using GPU)
2. Check Python version compatibility (3.8+)
3. Run test suite to validate installation

---

**Last Updated:** 2025-11-07
**Tested Platforms:** Windows 11, Ubuntu 22.04 (CPU-only and CUDA 12.4 tested), Python 3.10-3.12
