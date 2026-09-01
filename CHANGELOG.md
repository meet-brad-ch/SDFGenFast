# Changelog

All notable changes to SDFGenFast, relative to the [original SDFGen](https://github.com/christopherbatty/SDFGen) by Christopher Batty.

## 2.2.0 - 2026-08-31

Audit release: bug fixes, build modernization, and test consolidation. No new features.

### Breaking changes

- **Python: `nx`, `ny`, `nz` are now the total grid size, including padding cells.** This matches the CLI. Before this release, `generate_from_mesh` and `generate_from_file` added `2 * padding` on top of the requested size, so `nx=64, padding=2` produced a 68-cell axis. It now produces a 64-cell axis.
- **CLI: the positional thread-count argument is removed.** Use `-t/--threads` instead. The old grammar `SDFGen mesh.stl 128 1 10` (threads as a trailing positional) is no longer accepted.
- **CLI: the magic padding threshold is removed.** The old parser treated a second STL positional below 20 as padding and at 20 or above as a grid dimension. Now the argument count decides: two positionals are always `<Nx> <padding>`, three are `<Nx> <Ny> <Nz>`, four are `<Nx> <Ny> <Nz> <padding>`.

### Fixed

- **Data race in the multi-threaded CPU sweep.** Threads wrote to neighbor cells outside their slice range without synchronization, which made output at slice boundaries non-deterministic. Boundary slices are now swept serially after the threads join.
- **GPU/CPU precision divergence.** The GPU point-triangle distance path truncated intermediate values to `float` where the CPU path used `double`, which could flip inside/outside signs near the surface. Both paths now use `double`.
- **`sdf_to_mesh` scale error.** The converter divided the grid extent by `n - 1` where the writer uses `n`, which scaled every extracted mesh by `n / (n - 1)`. It now reads through the shared SDF I/O code.
- **CUDA errors killed the host process.** `CUDA_CHECK` called `exit()`, which terminated the Python interpreter on any GPU error. GPU errors now throw. With `backend="auto"` the library falls back to the CPU with a warning; with an explicit `backend="gpu"` the error propagates to the caller.
- **Tests could not fail on wrong values.** The comparison tolerance allowed differences up to 25 cell widths and ignored the sign-mismatch count. Tests now require zero sign mismatches and near-band agreement within half a cell.
- **`repair_mesh` return value.** It returned the number of hole loops attempted, not the number filled. It now counts only holes that produced triangles.
- **Parser hardening.** OBJ: vertex indices are bounds-checked and `stoi` failures are caught. STL: the triangle count in the binary header is validated against the file size before allocation. SDF: header dimensions are validated before allocation.
- **Wheel packaging.** `pip install .` did not ship the high-level Python API (`sdfgen.py`) and, on Windows, did not bundle the CUDA runtime DLL. Both are now installed into the wheel. The CUDA 13 DLL location (`bin/x64/`) is handled.

### Changed

- **CMake modernized.** Target-based configuration (`cmake_minimum_required 3.23`), strict warnings (`/W4` on MSVC, `-Wall -Wextra -Wpedantic` elsewhere), opt-in `SDFGEN_WARNINGS_AS_ERRORS`, `SDFGEN_CUDA_ARCHITECTURES` cache variable (default `all-major`), `BUILD_TESTING` guard, and a default `Release` build type.
- **Toolchain support.** Builds with Visual Studio 2026 and CUDA 13.3. CUDA 12.x with VS 2026 is not supported by NVIDIA's compiler frontend.
- **Test suite consolidated.** About 1,500 duplicated lines across the CLI tests replaced by table-driven helpers. Grid-sizing math shared between the CLI and the tests through `common/grid_sizing.h`.
- **Dead code removed.** Unused headers (`hashgrid.h`, `hashtable.h`, `array2.h`, most of `util.h`), an unreachable CPU `sweep()` function, disabled GPU debug code, and two never-read GPU buffers whose allocation and copies cost real time and VRAM.

## 2.0 - 2.1 (2025)

Fork enhancements by Brad Chamberlain over the original SDFGen:

- **Multi-threaded CPU implementation** with a configurable thread count.
- **CUDA GPU implementation** with automatic detection at build time and at run time.
- **Unified API** (`sdfgen::make_level_set3`) with `Auto`/`CPU`/`GPU` backend selection.
- **Python bindings** built on nanobind, with NumPy integration and a high-level API.
- **STL input** (binary and ASCII); the original supported only OBJ.
- **Mesh watertightness analysis** on every run, and optional hole filling via `--fix`.
- **Proportional grid sizing** for STL input (derive Y and Z from a target X dimension).
- **`sdf_to_mesh` tool**: marching-cubes isosurface extraction from SDF files, for debugging.
- **C++ and Python test suites** (the original had none).
- **CMake build with build scripts** for Windows (MSVC) and Linux (GCC/Clang).

## Original SDFGen (2015)

Single-threaded CPU SDF generation from OBJ meshes, by Christopher Batty, with VTK output support contributed by Daniel Canelhas.
