"""
SDFGen - GPU-accelerated signed distance field generation from triangle meshes

This module provides Python bindings for SDFGenFast, a high-performance tool
for generating grid-based signed distance fields (SDFs) from triangle meshes.

Basic usage:
    >>> import sdfgen
    >>> # Load a mesh
    >>> vertices, triangles, bounds = sdfgen.load_mesh("mesh.obj")
    >>>
    >>> # Generate SDF
    >>> sdf = sdfgen.generate_sdf(
    ...     vertices, triangles,
    ...     origin=(0, 0, 0),
    ...     dx=0.01,
    ...     nx=100, ny=100, nz=100
    ... )
    >>>
    >>> # Save to file
    >>> sdfgen.save_sdf("output.sdf", sdf, origin=(0, 0, 0), dx=0.01)
"""

__version__ = "2.2.0"
__author__ = "Brad Chamberlain, Christopher Batty"
__license__ = "MIT"

# Import the compiled extension module
try:
    from .sdfgen_ext import (
        load_mesh,
        generate_sdf,
        save_sdf,
        load_sdf,
        is_gpu_available,
    )
except ImportError as e:
    raise ImportError(
        f"Failed to import sdfgen_ext extension module: {e}\n"
        "Make sure the package was built correctly with CMake and nanobind."
    ) from e

import numpy as np
from typing import Tuple, Optional


def _grid_parameters(min_box, max_box, nx, ny, nz, dx, padding):
    """Compute (origin, dx, nx, ny, nz) for a padded grid around the bounds.

    Matches the SDFGen CLI semantics: grid dimensions given by the caller
    are TOTAL sizes including padding cells. The cell size is derived so
    the mesh plus padding fits the requested dimensions exactly.
    """
    extents = max_box - min_box

    if dx is not None:
        # Cell-size mode (CLI OBJ mode): dimensions grow to fit the mesh
        # plus padding at the given cell size.
        if nx is None:
            nx = int(np.ceil(extents[0] / dx)) + 2 * padding
        if ny is None:
            ny = int(np.ceil(extents[1] / dx)) + 2 * padding
        if nz is None:
            nz = int(np.ceil(extents[2] / dx)) + 2 * padding
    elif nx is not None:
        if nx <= 2 * padding:
            raise ValueError(
                f"nx={nx} must exceed 2*padding={2 * padding}; "
                "nx is the total grid size including padding cells"
            )
        if ny is None or nz is None:
            # Proportional mode (CLI STL mode with Nx only)
            dx = extents[0] / (nx - 2 * padding)
            if ny is None:
                ny = int(np.ceil(extents[1] / dx)) + 2 * padding
            if nz is None:
                nz = int(np.ceil(extents[2] / dx)) + 2 * padding
        else:
            # Manual mode (CLI STL mode with Nx Ny Nz): the largest
            # per-axis cell size wins so the mesh always fits.
            if min(ny, nz) <= 2 * padding:
                raise ValueError(
                    "ny and nz must exceed 2*padding; grid dimensions are "
                    "total sizes including padding cells"
                )
            dx = max(
                extents[0] / (nx - 2 * padding),
                extents[1] / (ny - 2 * padding),
                extents[2] / (nz - 2 * padding),
            )
    else:
        raise ValueError(
            "Must specify either 'dx' or 'nx' (or 'nx', 'ny', 'nz') for grid sizing"
        )

    origin = min_box - padding * dx
    return tuple(origin), float(dx), int(nx), int(ny), int(nz)


def _generate(vertices, triangles, min_box, max_box, nx, ny, nz, dx,
              padding, exact_band, backend, num_threads):
    """Shared implementation behind generate_from_mesh / generate_from_file."""
    origin, dx, nx, ny, nz = _grid_parameters(
        min_box, max_box, nx, ny, nz, dx, padding
    )

    sdf = generate_sdf(
        vertices,
        triangles,
        origin,
        dx,
        nx,
        ny,
        nz,
        exact_band=exact_band,
        backend=backend,
        num_threads=num_threads,
    )

    metadata = {
        "origin": origin,
        "dx": dx,
        "bounds": (tuple(min_box), tuple(max_box)),
        "backend": backend,
    }
    return sdf, metadata


def generate_from_mesh(
    vertices: np.ndarray,
    triangles: np.ndarray,
    nx: Optional[int] = None,
    ny: Optional[int] = None,
    nz: Optional[int] = None,
    dx: Optional[float] = None,
    padding: int = 1,
    exact_band: int = 1,
    backend: str = "auto",
    num_threads: int = 0,
) -> Tuple[np.ndarray, dict]:
    """
    Generate SDF from mesh arrays with automatic grid sizing.

    Grid dimensions are TOTAL sizes including padding cells, matching the
    SDFGen command line: ``generate_from_mesh(v, t, nx=128)`` and
    ``SDFGen mesh.stl 128`` produce the same grid.

    .. versionchanged:: 2.2
        ``nx``/``ny``/``nz`` now include the padding cells (CLI-consistent).
        Previously padding was added on top of the requested dimensions.

    Parameters
    ----------
    vertices : ndarray, shape (N, 3), dtype float32
        Vertex positions
    triangles : ndarray, shape (M, 3), dtype uint32
        Triangle indices
    nx : int, optional
        Total grid size in X, including padding (or all dimensions if
        ny, nz are not given). Alternative to dx.
    ny : int, optional
        Total grid size in Y (computed proportionally if not specified)
    nz : int, optional
        Total grid size in Z (computed proportionally if not specified)
    dx : float, optional
        Grid cell spacing (alternative to nx; dimensions grow to fit)
    padding : int, default=1
        Number of padding cells around the mesh on each side
    exact_band : int, default=1
        Distance band for exact computation
    backend : str, default="auto"
        Hardware backend: "auto", "cpu", or "gpu"
    num_threads : int, default=0
        Number of CPU threads (0 = auto-detect)

    Returns
    -------
    sdf : ndarray, shape (nx, ny, nz), dtype float32
        Signed distance field
    metadata : dict
        Dictionary with keys: origin, dx, bounds, backend
    """
    min_box = vertices.min(axis=0)
    max_box = vertices.max(axis=0)
    return _generate(vertices, triangles, min_box, max_box,
                     nx, ny, nz, dx, padding, exact_band, backend, num_threads)


def generate_from_file(
    filename: str,
    nx: Optional[int] = None,
    ny: Optional[int] = None,
    nz: Optional[int] = None,
    dx: Optional[float] = None,
    padding: int = 1,
    exact_band: int = 1,
    backend: str = "auto",
    num_threads: int = 0,
) -> Tuple[np.ndarray, dict]:
    """
    Generate SDF directly from a mesh file.

    Grid dimensions are TOTAL sizes including padding cells, matching the
    SDFGen command line (see generate_from_mesh).

    .. versionchanged:: 2.2
        ``nx``/``ny``/``nz`` now include the padding cells (CLI-consistent).

    Parameters
    ----------
    filename : str
        Path to mesh file (.obj or .stl)
    nx : int, optional
        Total grid size in X, including padding. If only nx is given the
        grid is sized proportionally. Alternative to dx.
    ny : int, optional
        Total grid size in Y
    nz : int, optional
        Total grid size in Z
    dx : float, optional
        Grid cell spacing (alternative to nx/ny/nz)
    padding : int, default=1
        Number of padding cells around the mesh on each side
    exact_band : int, default=1
        Distance band for exact computation
    backend : str, default="auto"
        Hardware backend: "auto", "cpu", or "gpu"
    num_threads : int, default=0
        Number of CPU threads (0 = auto-detect)

    Returns
    -------
    sdf : ndarray, shape (nx, ny, nz), dtype float32
        Signed distance field
    metadata : dict
        Dictionary with keys: origin, dx, bounds, backend

    Examples
    --------
    >>> # Proportional sizing, same grid as: SDFGen mesh.stl 256
    >>> sdf, meta = sdfgen.generate_from_file("mesh.stl", nx=256)
    >>>
    >>> # Exact dimensions
    >>> sdf, meta = sdfgen.generate_from_file("mesh.stl", nx=128, ny=128, nz=256)
    >>>
    >>> # Using cell size, same grid as: SDFGen mesh.obj 0.01 2
    >>> sdf, meta = sdfgen.generate_from_file("mesh.obj", dx=0.01, padding=2)
    """
    vertices, triangles, bounds = load_mesh(filename)
    min_box = np.array(bounds[0], dtype=np.float32)
    max_box = np.array(bounds[1], dtype=np.float32)
    return _generate(vertices, triangles, min_box, max_box,
                     nx, ny, nz, dx, padding, exact_band, backend, num_threads)


# Export public API
__all__ = [
    # Core functions from C++ extension
    "load_mesh",
    "generate_sdf",
    "save_sdf",
    "load_sdf",
    "is_gpu_available",
    # High-level Python convenience functions
    "generate_from_mesh",
    "generate_from_file",
]
