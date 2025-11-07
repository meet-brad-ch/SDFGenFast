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

__version__ = "1.0.0"
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
from typing import Tuple, Optional, Union


def generate_from_mesh(
    vertices: np.ndarray,
    triangles: np.ndarray,
    nx: int,
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

    This is a convenience function that automatically computes grid parameters
    based on mesh bounds and desired resolution.

    Parameters
    ----------
    vertices : ndarray, shape (N, 3), dtype float32
        Vertex positions
    triangles : ndarray, shape (M, 3), dtype uint32
        Triangle indices
    nx : int
        Grid size in X dimension (or all dimensions if ny, nz not specified)
    ny : int, optional
        Grid size in Y dimension (computed proportionally if not specified)
    nz : int, optional
        Grid size in Z dimension (computed proportionally if not specified)
    dx : float, optional
        Grid cell spacing (computed from nx and bounds if not specified)
    padding : int, default=1
        Number of padding cells around mesh
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
    # Compute bounding box
    min_box = vertices.min(axis=0)
    max_box = vertices.max(axis=0)
    extents = max_box - min_box

    # Compute grid parameters
    if ny is None or nz is None:
        # Proportional sizing
        if dx is None:
            dx = extents[0] / nx
        ny = int(np.ceil(extents[1] / dx)) if ny is None else ny
        nz = int(np.ceil(extents[2] / dx)) if nz is None else nz
    else:
        # Manual sizing
        if dx is None:
            dx = max(extents[0] / nx, extents[1] / ny, extents[2] / nz)

    # Add padding
    nx += 2 * padding
    ny += 2 * padding
    nz += 2 * padding

    # Adjust origin for padding
    origin = min_box - padding * dx

    # Generate SDF
    sdf = generate_sdf(
        vertices,
        triangles,
        tuple(origin),
        dx,
        nx,
        ny,
        nz,
        exact_band=exact_band,
        backend=backend,
        num_threads=num_threads,
    )

    # Prepare metadata
    metadata = {
        "origin": tuple(origin),
        "dx": dx,
        "bounds": (tuple(min_box), tuple(max_box)),
        "backend": backend,
    }

    return sdf, metadata


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

    This is a high-level convenience function that combines mesh loading
    and SDF generation into a single call.

    Parameters
    ----------
    filename : str
        Path to mesh file (.obj or .stl)
    nx : int, optional
        Grid size in X dimension. If only nx is provided, grid is sized
        proportionally. If nx, ny, nz are all provided, grid uses exact
        dimensions. If dx is provided instead, nx is computed from bounds.
    ny : int, optional
        Grid size in Y dimension
    nz : int, optional
        Grid size in Z dimension
    dx : float, optional
        Grid cell spacing (alternative to nx/ny/nz)
    padding : int, default=1
        Number of padding cells around mesh
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
    >>> # Proportional sizing (256^3 for a cube)
    >>> sdf, meta = sdfgen.generate_from_file("mesh.stl", nx=256)
    >>>
    >>> # Exact dimensions
    >>> sdf, meta = sdfgen.generate_from_file("mesh.stl", nx=128, ny=128, nz=256)
    >>>
    >>> # Using cell size (like CLI)
    >>> sdf, meta = sdfgen.generate_from_file("mesh.obj", dx=0.01, padding=2)
    """
    # Load mesh
    vertices, triangles, bounds = load_mesh(filename)
    min_box = np.array(bounds[0], dtype=np.float32)
    max_box = np.array(bounds[1], dtype=np.float32)
    extents = max_box - min_box

    # Determine grid sizing mode
    if dx is not None:
        # Mode 1: Use cell size (like CLI for OBJ)
        if nx is None:
            nx = int(np.ceil(extents[0] / dx))
        if ny is None:
            ny = int(np.ceil(extents[1] / dx))
        if nz is None:
            nz = int(np.ceil(extents[2] / dx))
    elif nx is not None:
        # Mode 2: Use grid dimensions
        if ny is None or nz is None:
            # Proportional sizing
            if dx is None:
                dx = extents[0] / nx
            ny = int(np.ceil(extents[1] / dx)) if ny is None else ny
            nz = int(np.ceil(extents[2] / dx)) if nz is None else nz
        else:
            # Manual sizing - compute dx from largest dimension
            if dx is None:
                dx = max(extents[0] / nx, extents[1] / ny, extents[2] / nz)
    else:
        raise ValueError(
            "Must specify either 'dx' or 'nx' (or 'nx', 'ny', 'nz') for grid sizing"
        )

    # Add padding
    nx += 2 * padding
    ny += 2 * padding
    nz += 2 * padding

    # Adjust origin for padding
    origin = min_box - padding * dx

    # Generate SDF
    sdf = generate_sdf(
        vertices,
        triangles,
        tuple(origin),
        dx,
        nx,
        ny,
        nz,
        exact_band=exact_band,
        backend=backend,
        num_threads=num_threads,
    )

    # Prepare metadata
    metadata = {
        "origin": tuple(origin),
        "dx": dx,
        "bounds": (tuple(min_box), tuple(max_box)),
        "backend": backend,
    }

    return sdf, metadata


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
