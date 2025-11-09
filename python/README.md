# SDFGen Python API

High-performance Python bindings for SDFGenFast with NumPy integration and GPU acceleration.

## Overview

The Python bindings provide a fast, NumPy-based API for generating signed distance fields from triangle meshes. Built with **nanobind** for minimal overhead and native performance.

**Key Features:**
- NumPy array integration (zero-copy where possible)
- Automatic GPU acceleration (CUDA)
- Low-level control over SDF generation
- High-level convenience functions
- Full test coverage (51 tests)

## Installation

**See [BUILD.md](../BUILD.md#building-python-bindings) for complete build instructions.**

**Quick install:**
```bash
pip install .
```

**Verify installation:**
```python
import sdfgen
print(sdfgen.__version__)
print('GPU available:', sdfgen.is_gpu_available())
```

## API Reference

### Core Functions

#### `load_mesh(filename)`

Load a triangle mesh from file.

**Parameters:**
- `filename` (str): Path to mesh file (.obj or .stl)

**Returns:**
- `vertices` (ndarray): Vertex positions, shape (N, 3), dtype float32
- `triangles` (ndarray): Triangle indices, shape (M, 3), dtype uint32
- `bounds` (tuple): `((min_x, min_y, min_z), (max_x, max_y, max_z))`

**Example:**
```python
vertices, triangles, bounds = sdfgen.load_mesh("mesh.obj")
print(f"Loaded {len(vertices)} vertices, {len(triangles)} triangles")
print(f"Bounds: {bounds}")
```

---

#### `generate_sdf(vertices, triangles, origin, dx, nx, ny, nz, **kwargs)`

Generate signed distance field from mesh arrays.

**Parameters:**
- `vertices` (ndarray): Vertex positions, shape (N, 3), dtype float32
- `triangles` (ndarray): Triangle indices, shape (M, 3), dtype uint32
- `origin` (tuple): Grid origin (x, y, z) in world space
- `dx` (float): Grid cell spacing
- `nx, ny, nz` (int): Grid dimensions
- `exact_band` (int, optional): Distance band for exact computation (default: 1)
- `backend` (str, optional): Hardware backend: 'auto', 'cpu', or 'gpu' (default: 'auto')
- `num_threads` (int, optional): CPU threads, 0 for auto-detect (default: 0)

**Returns:**
- `sdf` (ndarray): Signed distance field, shape (nx, ny, nz), dtype float32

**Distance convention:**
- Negative: Inside mesh
- Positive: Outside mesh
- Zero: On surface

**Example:**
```python
sdf = sdfgen.generate_sdf(
    vertices, triangles,
    origin=(0, 0, 0),
    dx=0.01,
    nx=100, ny=100, nz=100,
    exact_band=1,
    backend="auto",    # Uses GPU if available
    num_threads=0      # Auto-detect CPU cores
)
```

---

#### `save_sdf(filename, sdf_array, origin, dx)`

Save SDF to binary file.

**Parameters:**
- `filename` (str): Output file path (.sdf)
- `sdf_array` (ndarray): Signed distance field, shape (nx, ny, nz), dtype float32
- `origin` (tuple): Grid origin (x, y, z)
- `dx` (float): Grid cell spacing

**Example:**
```python
sdfgen.save_sdf("output.sdf", sdf, origin=(0, 0, 0), dx=0.01)
```

---

#### `load_sdf(filename)`

Load SDF from binary file.

**Parameters:**
- `filename` (str): Input file path (.sdf)

**Returns:**
- `sdf` (ndarray): Signed distance field, shape (nx, ny, nz), dtype float32
- `origin` (tuple): Grid origin (x, y, z)
- `dx` (float): Grid cell spacing
- `bounds` (tuple): `((min_x, min_y, min_z), (max_x, max_y, max_z))`

**Example:**
```python
sdf, origin, dx, bounds = sdfgen.load_sdf("input.sdf")
print(f"Loaded SDF: {sdf.shape}, dx={dx}")
```

---

#### `is_gpu_available()`

Check if GPU acceleration (CUDA) is available.

**Returns:**
- `bool`: True if GPU is available, False otherwise

**Example:**
```python
if sdfgen.is_gpu_available():
    print("GPU acceleration enabled")
else:
    print("Using CPU backend")
```

---

### High-Level Convenience API

#### `generate_from_mesh(vertices, triangles, nx, **kwargs)`

Generate SDF from mesh arrays with automatic grid sizing.

**Parameters:**
- `vertices` (ndarray): Vertex positions, shape (N, 3), dtype float32
- `triangles` (ndarray): Triangle indices, shape (M, 3), dtype uint32
- `nx` (int): Grid size in X dimension (or all dimensions if ny, nz not specified)
- `ny` (int, optional): Grid size in Y dimension
- `nz` (int, optional): Grid size in Z dimension
- `dx` (float, optional): Grid cell spacing (computed from nx if not specified)
- `padding` (int, optional): Number of padding cells around mesh (default: 1)
- `exact_band` (int, optional): Distance band for exact computation (default: 1)
- `backend` (str, optional): 'auto', 'cpu', or 'gpu' (default: 'auto')
- `num_threads` (int, optional): CPU threads, 0=auto (default: 0)

**Returns:**
- `sdf` (ndarray): Signed distance field, shape (nx, ny, nz), dtype float32
- `metadata` (dict): Dictionary with keys: 'origin', 'dx', 'bounds', 'backend'

**Example:**
```python
# Proportional sizing (256³ for cube)
sdf, metadata = sdfgen.generate_from_mesh(
    vertices, triangles,
    nx=256,
    padding=2
)

# Explicit dimensions
sdf, metadata = sdfgen.generate_from_mesh(
    vertices, triangles,
    nx=128, ny=128, nz=256,
    padding=1
)
```

---

#### `generate_from_file(filename, **kwargs)`

Generate SDF directly from a mesh file.

**Parameters:**
- `filename` (str): Path to mesh file (.obj or .stl)
- `nx` (int, optional): Grid size (see generate_from_mesh)
- `ny, nz` (int, optional): Grid dimensions
- `dx` (float, optional): Cell spacing (alternative to nx)
- `padding` (int, optional): Padding cells (default: 1)
- `exact_band` (int, optional): Exact band (default: 1)
- `backend` (str, optional): 'auto', 'cpu', 'gpu' (default: 'auto')
- `num_threads` (int, optional): CPU threads (default: 0)

**Returns:**
- `sdf` (ndarray): Signed distance field
- `metadata` (dict): Grid metadata

**Examples:**
```python
# Proportional sizing
sdf, meta = sdfgen.generate_from_file("mesh.stl", nx=256)

# Explicit dimensions
sdf, meta = sdfgen.generate_from_file("mesh.stl", nx=128, ny=128, nz=256)

# Using cell size
sdf, meta = sdfgen.generate_from_file("mesh.obj", dx=0.01, padding=2)
```

---

## Usage Examples

### Example 1: Basic SDF Generation

```python
import sdfgen
import numpy as np

# Load mesh
vertices, triangles, bounds = sdfgen.load_mesh("bunny.obj")
print(f"Mesh: {len(vertices)} vertices, {len(triangles)} triangles")

# Generate SDF (auto-selects GPU if available)
sdf = sdfgen.generate_sdf(
    vertices, triangles,
    origin=bounds[0],
    dx=0.01,
    nx=256, ny=256, nz=256
)

print(f"SDF shape: {sdf.shape}")
print(f"Inside cells: {np.sum(sdf < 0)}")
print(f"GPU used: {sdfgen.is_gpu_available()}")
```

### Example 2: Backend Comparison

```python
import sdfgen
import time

vertices, triangles, bounds = sdfgen.load_mesh("mesh.stl")

# CPU benchmark
start = time.time()
sdf_cpu = sdfgen.generate_sdf(
    vertices, triangles,
    origin=bounds[0],
    dx=0.02,
    nx=128, ny=128, nz=128,
    backend="cpu",
    num_threads=0  # Auto-detect
)
cpu_time = time.time() - start

# GPU benchmark
if sdfgen.is_gpu_available():
    start = time.time()
    sdf_gpu = sdfgen.generate_sdf(
        vertices, triangles,
        origin=bounds[0],
        dx=0.02,
        nx=128, ny=128, nz=128,
        backend="gpu"
    )
    gpu_time = time.time() - start

    print(f"CPU: {cpu_time:.3f}s, GPU: {gpu_time:.3f}s")
    print(f"Speedup: {cpu_time/gpu_time:.2f}x")

    # Verify results match
    diff = np.abs(sdf_cpu - sdf_gpu)
    print(f"Max difference: {np.max(diff):.6f}")
```

### Example 3: High-Level API

```python
import sdfgen

# Simple one-liner
sdf, metadata = sdfgen.generate_from_file(
    "mesh.stl",
    nx=256,
    padding=2,
    backend="auto"
)

print(f"Generated {sdf.shape} SDF")
print(f"Origin: {metadata['origin']}")
print(f"Cell size: {metadata['dx']}")
print(f"Bounds: {metadata['bounds']}")

# Save to file
sdfgen.save_sdf("output.sdf", sdf, metadata['origin'], metadata['dx'])
```

### Example 4: Programmatic Mesh

```python
import sdfgen
import numpy as np

# Create cube mesh programmatically
vertices = np.array([
    [-0.5, -0.5, -0.5], [0.5, -0.5, -0.5],
    [0.5,  0.5, -0.5], [-0.5,  0.5, -0.5],
    [-0.5, -0.5,  0.5], [0.5, -0.5,  0.5],
    [0.5,  0.5,  0.5], [-0.5,  0.5,  0.5],
], dtype=np.float32)

triangles = np.array([
    [0, 1, 2], [0, 2, 3],  # Front
    [4, 6, 5], [4, 7, 6],  # Back
    [0, 3, 7], [0, 7, 4],  # Left
    [1, 5, 6], [1, 6, 2],  # Right
    [0, 4, 5], [0, 5, 1],  # Bottom
    [3, 2, 6], [3, 6, 7],  # Top
], dtype=np.uint32)

# Generate SDF
sdf = sdfgen.generate_sdf(
    vertices, triangles,
    origin=(-1, -1, -1),
    dx=0.05,
    nx=40, ny=40, nz=40
)

# Analyze results
inside = np.sum(sdf < 0)
outside = np.sum(sdf > 0)
surface = np.sum(np.abs(sdf) < 0.05)

print(f"Inside: {inside}, Outside: {outside}, Near surface: {surface}")
```

---

## Performance Notes

### GPU vs CPU

**Typical speedups (128³ grid):**
- Small meshes: GPU ~2-3x faster than multi-threaded CPU
- Large meshes: GPU ~4-5x faster than multi-threaded CPU

**When to use GPU:**
- Grid size ≥ 128³
- Complex meshes (many triangles)
- Batch processing multiple SDFs

**When CPU is sufficient:**
- Small grids (<64³)
- GPU not available
- Single SDF generation

### Memory Considerations

**Memory usage (approximate):**
- CPU: `4 * nx * ny * nz` bytes for SDF grid
- GPU: Additional `2x` for device memory during computation

**Example (256³ grid):**
- SDF array: 64 MB
- GPU working memory: ~128 MB additional
- Total: ~192 MB

---

## Troubleshooting

### ImportError: DLL load failed (Windows)

**Problem:** Missing CUDA runtime DLL

**Solution:**
```powershell
# Copy CUDA DLL to package directory
copy "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4\bin\cudart64_12.dll" sdfgen\
```

Or add CUDA to PATH:
```powershell
$env:PATH += ";C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4\bin"
```

### ImportError: undefined symbol (Linux)

**Problem:** Missing CUDA library path

**Solution:**
```bash
export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
# Add to ~/.bashrc to make permanent
```

### GPU Not Detected

**Check CUDA installation:**
```bash
# CUDA compiler
nvcc --version

# GPU driver (Linux)
nvidia-smi

# Test in Python
python -c "import sdfgen; print(sdfgen.is_gpu_available())"
```

**If False:**
1. Rebuild with CUDA: Build will auto-detect CUDA Toolkit
2. Verify NVIDIA driver installed (`nvidia-smi`)
3. Check CUDA runtime accessible (see DLL issues above)

### ModuleNotFoundError: No module named 'sdfgen'

**Problem:** Package not installed

**Solution:**
```bash
# Option 1: Install package
pip install .

# Option 2: Add to PYTHONPATH
export PYTHONPATH=/path/to/SDFGenFast:$PYTHONPATH
```

---

## Testing

**Run Python test suite (51 tests):**
```bash
pip install pytest
pytest python/tests/test_sdfgen.py -v
```

**Expected output:**
```
============================== 51 passed in 0.49s ==============================
```

**Test categories:**
- Basic functionality (5 tests)
- Backend selection (4 tests)
- Parameter variations (5 tests)
- Error handling (11 tests)
- SDF properties (2 tests)
- Data validation (6 tests)
- Edge cases (8 tests)
- High-level API (10 tests)

**See [main README Appendix B](../README.md#appendix-b-testing-guide) for details.**

---

## Advanced Options

### Custom Python Installation

```bash
cmake -DBUILD_PYTHON_BINDINGS=ON \
      -DPython_EXECUTABLE=/path/to/python3.10 \
      ..
```

### Force CPU-Only Build

```bash
cmake -DBUILD_PYTHON_BINDINGS=ON \
      -DSDFGEN_BUILD_GPU=OFF \
      ..
```

### Specify CUDA Architecture

```bash
cmake -DBUILD_PYTHON_BINDINGS=ON \
      -DCMAKE_CUDA_ARCHITECTURES="80;89" \
      ..
```

**See [BUILD.md](../BUILD.md#advanced-options) for complete build options.**

---

## File Locations

After building:

```
SDFGenFast/
├── build-Release/
│   └── lib/
│       └── sdfgen_ext.*.pyd/.so    # Compiled extension
├── python/
│   ├── __init__.py                  # High-level API
│   ├── sdfgen_py.cpp                # C++ bindings source
│   ├── tests/test_sdfgen.py        # Test suite (51 tests)
│   └── README.md                    # This file
└── sdfgen/                          # Installed package
    ├── __init__.py
    └── sdfgen_ext.*.pyd/.so
```

---

## Next Steps

- **Build Instructions:** See [BUILD.md](../BUILD.md#building-python-bindings)
- **Usage Examples:** See [main README](../README.md#python-usage)
- **Testing:** Run `pytest python/tests/test_sdfgen.py -v`

---

**Version:** 1.0.0
**Last Updated:** 2025-01-06
**Python:** 3.8+
**Dependencies:** NumPy 1.20+, nanobind 2.0+
