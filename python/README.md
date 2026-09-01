# SDFGen Python API

Python bindings for SDFGenFast with NumPy integration and GPU acceleration, built with nanobind.

## Overview

The bindings provide a NumPy-based API for generating signed distance fields from triangle meshes.

**Key features:**

- NumPy array integration
- Automatic GPU acceleration (CUDA), with CPU fallback
- Low-level control (`generate_sdf`) and high-level convenience functions (`generate_from_file`, `generate_from_mesh`)
- The GIL is released during computation, so other Python threads keep running
- 51 tests

**Changed in 2.2:** `nx`, `ny`, and `nz` are the total grid size, including the padding cells. This matches the CLI. Before 2.2, `generate_from_mesh` and `generate_from_file` added `2 * padding` on top of the requested size.

## Installation

See [BUILD.md](../BUILD.md#building-the-python-bindings) for build details.

```bash
pip install .
```

Verify:

```python
import sdfgen
print(sdfgen.__version__)
print('GPU available:', sdfgen.is_gpu_available())
```

## API Reference

### Core Functions

#### `load_mesh(filename)`

Load a triangle mesh from a file.

**Parameters:**
- `filename` (str): path to the mesh file (`.obj` or `.stl`)

**Returns:**
- `vertices` (ndarray): vertex positions, shape (N, 3), dtype float32
- `triangles` (ndarray): triangle indices, shape (M, 3), dtype uint32
- `bounds` (tuple): `((min_x, min_y, min_z), (max_x, max_y, max_z))`

```python
vertices, triangles, bounds = sdfgen.load_mesh("mesh.obj")
```

---

#### `generate_sdf(vertices, triangles, origin, dx, nx, ny, nz, **kwargs)`

Generate a signed distance field from mesh arrays. This is the low-level entry point: you specify the grid placement yourself.

**Parameters:**
- `vertices` (ndarray): vertex positions, shape (N, 3), dtype float32
- `triangles` (ndarray): triangle indices, shape (M, 3), dtype uint32
- `origin` (tuple): grid origin (x, y, z) in world space
- `dx` (float): grid cell spacing
- `nx, ny, nz` (int): grid dimensions
- `exact_band` (int, optional): width of the exactly-computed band in cells (default 1). A larger band computes more cells exactly and costs more time.
- `backend` (str, optional): `'auto'`, `'cpu'`, or `'gpu'` (default `'auto'`)
- `num_threads` (int, optional): CPU threads, 0 = auto-detect (default 0)

**Returns:**
- `sdf` (ndarray): signed distance field, shape (nx, ny, nz), dtype float32

**Distance convention:** negative inside the mesh, positive outside, zero on the surface.

**Backend behavior:** with `'auto'`, the GPU is used when available; a GPU failure falls back to the CPU with a warning. With `'gpu'`, a GPU failure raises an exception.

```python
sdf = sdfgen.generate_sdf(
    vertices, triangles,
    origin=(0, 0, 0),
    dx=0.01,
    nx=100, ny=100, nz=100,
)
```

---

#### `save_sdf(filename, sdf_array, origin, dx)`

Save an SDF to the binary file format (see the main README for the format definition).

```python
sdfgen.save_sdf("output.sdf", sdf, origin=(0, 0, 0), dx=0.01)
```

---

#### `load_sdf(filename)`

Load an SDF from a binary file.

**Returns:**
- `sdf` (ndarray): signed distance field, shape (nx, ny, nz), dtype float32
- `origin` (tuple): grid origin (x, y, z)
- `dx` (float): grid cell spacing
- `bounds` (tuple): `((min_x, min_y, min_z), (max_x, max_y, max_z))`

```python
sdf, origin, dx, bounds = sdfgen.load_sdf("input.sdf")
```

---

#### `is_gpu_available()`

Return `True` when a CUDA GPU is available at run time and GPU support was compiled in.

---

### High-Level API

#### `generate_from_mesh(vertices, triangles, nx=None, ny=None, nz=None, dx=None, padding=1, exact_band=1, backend="auto", num_threads=0)`

Generate an SDF from mesh arrays with automatic grid placement. The grid is centered on the mesh. Choose one sizing mode:

- `nx` only: proportional sizing. `ny` and `nz` are derived from the mesh aspect ratio.
- `nx`, `ny`, `nz`: explicit dimensions. The cell size is chosen so the mesh fits.
- `dx` only: cell-size mode. The dimensions are derived from the mesh extent plus padding.

All dimensions are the total grid size, including the padding cells (changed in 2.2).

**Returns:**
- `sdf` (ndarray): signed distance field, shape (nx, ny, nz), dtype float32
- `metadata` (dict): keys `'origin'`, `'dx'`, `'bounds'`, `'backend'`

```python
# Proportional sizing
sdf, metadata = sdfgen.generate_from_mesh(vertices, triangles, nx=256, padding=2)

# Explicit dimensions
sdf, metadata = sdfgen.generate_from_mesh(vertices, triangles, nx=128, ny=128, nz=256)

# Cell-size mode
sdf, metadata = sdfgen.generate_from_mesh(vertices, triangles, dx=0.01, padding=2)
```

---

#### `generate_from_file(filename, nx=None, ny=None, nz=None, dx=None, padding=1, exact_band=1, backend="auto", num_threads=0)`

The same as `generate_from_mesh`, but loads the mesh from a file first.

```python
sdf, meta = sdfgen.generate_from_file("mesh.stl", nx=256)
sdf, meta = sdfgen.generate_from_file("mesh.stl", nx=128, ny=128, nz=256)
sdf, meta = sdfgen.generate_from_file("mesh.obj", dx=0.01, padding=2)
```

---

## Usage Examples

### Basic SDF generation

```python
import sdfgen
import numpy as np

vertices, triangles, bounds = sdfgen.load_mesh("mesh.obj")

sdf = sdfgen.generate_sdf(
    vertices, triangles,
    origin=bounds[0],
    dx=0.01,
    nx=256, ny=256, nz=256,
)

print(f"SDF shape: {sdf.shape}")
print(f"Inside cells: {np.sum(sdf < 0)}")
```

### Backend comparison

```python
import sdfgen
import numpy as np
import time

vertices, triangles, bounds = sdfgen.load_mesh("mesh.stl")
args = dict(origin=bounds[0], dx=0.02, nx=128, ny=128, nz=128)

start = time.time()
sdf_cpu = sdfgen.generate_sdf(vertices, triangles, backend="cpu", **args)
cpu_time = time.time() - start

if sdfgen.is_gpu_available():
    start = time.time()
    sdf_gpu = sdfgen.generate_sdf(vertices, triangles, backend="gpu", **args)
    gpu_time = time.time() - start

    print(f"CPU: {cpu_time:.3f}s, GPU: {gpu_time:.3f}s")
    print(f"Max difference: {np.max(np.abs(sdf_cpu - sdf_gpu)):.6f}")
```

### Programmatic mesh

```python
import sdfgen
import numpy as np

vertices = np.array([
    [-0.5, -0.5, -0.5], [0.5, -0.5, -0.5],
    [0.5,  0.5, -0.5], [-0.5,  0.5, -0.5],
    [-0.5, -0.5,  0.5], [0.5, -0.5,  0.5],
    [0.5,  0.5,  0.5], [-0.5,  0.5,  0.5],
], dtype=np.float32)

triangles = np.array([
    [0, 1, 2], [0, 2, 3],  # front
    [4, 6, 5], [4, 7, 6],  # back
    [0, 3, 7], [0, 7, 4],  # left
    [1, 5, 6], [1, 6, 2],  # right
    [0, 4, 5], [0, 5, 1],  # bottom
    [3, 2, 6], [3, 6, 7],  # top
], dtype=np.uint32)

sdf = sdfgen.generate_sdf(
    vertices, triangles,
    origin=(-1, -1, -1),
    dx=0.05,
    nx=40, ny=40, nz=40,
)
```

---

## Performance Notes

- Measured performance for both backends is in the [main README, Appendix A](../README.md#appendix-a-performance-benchmarks). On the test system (RTX 4090, CUDA 13.3), the GPU was 2.4x to 7.1x faster than the best multi-threaded CPU time, and the advantage grows with grid size.
- **Host memory:** the SDF array is `4 * nx * ny * nz` bytes (float32).
- **GPU memory:** the kernel allocates about 20 bytes per grid cell (distance/triangle pairs, intersection counts, and two sweep buffers), plus the mesh data.

---

## Troubleshooting

### `ImportError: DLL load failed` (Windows)

The extension needs the CUDA runtime DLL. `pip install .` bundles it into the package. For other setups, add the CUDA `bin` directory to `PATH` (CUDA 13 keeps the DLLs in `bin\x64`).

### `ImportError: undefined symbol` (Linux)

```bash
export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
```

### `is_gpu_available()` returns False

1. Check the driver: `nvidia-smi`.
2. Check that the build found CUDA: rebuild and read the CMake output.
3. Check that the CUDA runtime is loadable (see the DLL issues above).

### `ModuleNotFoundError: No module named 'sdfgen'`

Run `pip install .` from the project root.

---

## Testing

```bash
pip install pytest
pytest python/tests -v
```

The suite has 51 tests; see the [main README, Appendix B](../README.md#appendix-b-testing-guide) for the class-by-class breakdown.

---

## Files

```
python/
├── sdfgen.py          # high-level API (installed as sdfgen/__init__.py)
├── sdfgen_py.cpp      # nanobind extension source
├── tests/             # test suite (51 tests)
└── README.md          # this file
```

---

**Requires:** Python 3.8+, NumPy 1.20+. Version: 2.2.0 (see [CHANGELOG.md](../CHANGELOG.md)).
