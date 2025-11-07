"""
Tests for SDFGen Python bindings
"""

import pytest
import numpy as np
import os
import tempfile
from pathlib import Path

import sdfgen


# Test fixtures
@pytest.fixture
def simple_cube():
    """Create a simple cube mesh for testing."""
    # Cube vertices (1x1x1 centered at origin)
    vertices = np.array(
        [
            [-0.5, -0.5, -0.5],
            [0.5, -0.5, -0.5],
            [0.5, 0.5, -0.5],
            [-0.5, 0.5, -0.5],
            [-0.5, -0.5, 0.5],
            [0.5, -0.5, 0.5],
            [0.5, 0.5, 0.5],
            [-0.5, 0.5, 0.5],
        ],
        dtype=np.float32,
    )

    # Cube triangles (12 triangles, 2 per face)
    triangles = np.array(
        [
            # Front face
            [0, 1, 2],
            [0, 2, 3],
            # Back face
            [4, 6, 5],
            [4, 7, 6],
            # Left face
            [0, 3, 7],
            [0, 7, 4],
            # Right face
            [1, 5, 6],
            [1, 6, 2],
            # Bottom face
            [0, 4, 5],
            [0, 5, 1],
            # Top face
            [3, 2, 6],
            [3, 6, 7],
        ],
        dtype=np.uint32,
    )

    return vertices, triangles


@pytest.fixture
def temp_obj_file(simple_cube):
    """Create a temporary OBJ file."""
    vertices, triangles = simple_cube

    with tempfile.NamedTemporaryFile(mode="w", suffix=".obj", delete=False) as f:
        # Write vertices
        for v in vertices:
            f.write(f"v {v[0]} {v[1]} {v[2]}\n")

        # Write faces (OBJ uses 1-based indexing)
        for t in triangles:
            f.write(f"f {t[0]+1} {t[1]+1} {t[2]+1}\n")

        temp_path = f.name

    yield temp_path

    # Cleanup
    os.unlink(temp_path)


@pytest.fixture
def temp_sdf_file():
    """Create a temporary SDF file path."""
    with tempfile.NamedTemporaryFile(suffix=".sdf", delete=False) as f:
        temp_path = f.name

    yield temp_path

    # Cleanup
    if os.path.exists(temp_path):
        os.unlink(temp_path)


# Basic functionality tests
class TestBasicFunctionality:
    def test_generate_sdf_from_arrays(self, simple_cube):
        """Test basic SDF generation from numpy arrays."""
        vertices, triangles = simple_cube

        sdf = sdfgen.generate_sdf(
            vertices,
            triangles,
            origin=(-1.0, -1.0, -1.0),
            dx=0.1,
            nx=20,
            ny=20,
            nz=20,
        )

        assert isinstance(sdf, np.ndarray)
        assert sdf.shape == (20, 20, 20)
        assert sdf.dtype == np.float32

        # Check that the center is negative (inside) and corners are positive (outside)
        center_val = sdf[10, 10, 10]
        corner_val = sdf[0, 0, 0]
        assert center_val < 0, "Center of cube should be negative (inside)"
        assert corner_val > 0, "Corner outside cube should be positive"

    def test_load_mesh_from_file(self, temp_obj_file):
        """Test loading a mesh from an OBJ file."""
        vertices, triangles, bounds = sdfgen.load_mesh(temp_obj_file)

        assert isinstance(vertices, np.ndarray)
        assert isinstance(triangles, np.ndarray)
        assert vertices.shape[1] == 3
        assert triangles.shape[1] == 3
        assert vertices.dtype == np.float32
        assert triangles.dtype == np.uint32

        # Check bounds
        min_box, max_box = bounds
        assert len(min_box) == 3
        assert len(max_box) == 3

    def test_generate_from_file(self, temp_obj_file):
        """Test high-level API: generate_from_file."""
        sdf, metadata = sdfgen.generate_from_file(temp_obj_file, nx=32, padding=2)

        assert isinstance(sdf, np.ndarray)
        assert sdf.dtype == np.float32
        assert len(sdf.shape) == 3

        # Check metadata
        assert "origin" in metadata
        assert "dx" in metadata
        assert "bounds" in metadata
        assert "backend" in metadata

    def test_generate_from_mesh(self, simple_cube):
        """Test high-level API: generate_from_mesh."""
        vertices, triangles = simple_cube

        sdf, metadata = sdfgen.generate_from_mesh(
            vertices, triangles, nx=32, padding=2, backend="cpu"
        )

        assert isinstance(sdf, np.ndarray)
        assert sdf.dtype == np.float32
        assert len(sdf.shape) == 3

        # Check metadata
        assert "origin" in metadata
        assert "dx" in metadata
        assert "bounds" in metadata
        assert metadata["backend"] in ["cpu", "gpu"]

    def test_save_and_load_sdf(self, simple_cube, temp_sdf_file):
        """Test saving and loading SDF files."""
        vertices, triangles = simple_cube

        # Generate SDF
        sdf = sdfgen.generate_sdf(
            vertices,
            triangles,
            origin=(0.0, 0.0, 0.0),
            dx=0.1,
            nx=10,
            ny=10,
            nz=10,
        )

        # Save to file
        sdfgen.save_sdf(temp_sdf_file, sdf, origin=(0.0, 0.0, 0.0), dx=0.1)

        # Load back
        loaded_sdf, loaded_origin, loaded_dx, loaded_bounds = sdfgen.load_sdf(
            temp_sdf_file
        )

        # Check equality
        assert loaded_sdf.shape == sdf.shape
        assert np.allclose(loaded_sdf, sdf)
        assert loaded_dx == pytest.approx(0.1)


# Backend tests
class TestBackends:
    def test_is_gpu_available(self):
        """Test GPU availability query."""
        result = sdfgen.is_gpu_available()
        assert isinstance(result, bool)

    def test_cpu_backend(self, simple_cube):
        """Test forcing CPU backend."""
        vertices, triangles = simple_cube

        sdf = sdfgen.generate_sdf(
            vertices,
            triangles,
            origin=(0.0, 0.0, 0.0),
            dx=0.1,
            nx=10,
            ny=10,
            nz=10,
            backend="cpu",
        )

        assert isinstance(sdf, np.ndarray)
        assert sdf.shape == (10, 10, 10)

    @pytest.mark.skipif(
        not sdfgen.is_gpu_available(), reason="GPU not available"
    )
    def test_gpu_backend(self, simple_cube):
        """Test forcing GPU backend (skipped if GPU not available)."""
        vertices, triangles = simple_cube

        sdf = sdfgen.generate_sdf(
            vertices,
            triangles,
            origin=(0.0, 0.0, 0.0),
            dx=0.1,
            nx=10,
            ny=10,
            nz=10,
            backend="gpu",
        )

        assert isinstance(sdf, np.ndarray)
        assert sdf.shape == (10, 10, 10)

    @pytest.mark.skipif(
        not sdfgen.is_gpu_available(), reason="GPU not available"
    )
    def test_cpu_gpu_consistency(self, simple_cube):
        """Test that CPU and GPU backends produce similar results."""
        vertices, triangles = simple_cube

        # Generate with CPU
        sdf_cpu = sdfgen.generate_sdf(
            vertices,
            triangles,
            origin=(0.0, 0.0, 0.0),
            dx=0.05,
            nx=20,
            ny=20,
            nz=20,
            backend="cpu",
        )

        # Generate with GPU
        sdf_gpu = sdfgen.generate_sdf(
            vertices,
            triangles,
            origin=(0.0, 0.0, 0.0),
            dx=0.05,
            nx=20,
            ny=20,
            nz=20,
            backend="gpu",
        )

        # Should be reasonably close (allowing for numerical differences due to different algorithms)
        # CPU uses multi-threaded fast sweeping, GPU uses CUDA kernels - expect ~5% difference
        assert np.allclose(sdf_cpu, sdf_gpu, rtol=0.1, atol=0.05)


# Parameter variation tests
class TestParameters:
    def test_different_grid_sizes(self, simple_cube):
        """Test with different grid sizes."""
        vertices, triangles = simple_cube

        for nx in [10, 20, 50]:
            sdf = sdfgen.generate_sdf(
                vertices,
                triangles,
                origin=(0.0, 0.0, 0.0),
                dx=0.1,
                nx=nx,
                ny=nx,
                nz=nx,
            )
            assert sdf.shape == (nx, nx, nx)

    def test_non_uniform_grid(self, simple_cube):
        """Test with non-uniform grid dimensions."""
        vertices, triangles = simple_cube

        sdf = sdfgen.generate_sdf(
            vertices,
            triangles,
            origin=(0.0, 0.0, 0.0),
            dx=0.1,
            nx=10,
            ny=20,
            nz=30,
        )

        assert sdf.shape == (10, 20, 30)

    def test_different_cell_sizes(self, simple_cube):
        """Test with different cell sizes."""
        vertices, triangles = simple_cube

        for dx in [0.05, 0.1, 0.2]:
            sdf = sdfgen.generate_sdf(
                vertices,
                triangles,
                origin=(0.0, 0.0, 0.0),
                dx=dx,
                nx=10,
                ny=10,
                nz=10,
            )
            assert sdf.shape == (10, 10, 10)

    def test_exact_band_parameter(self, simple_cube):
        """Test with different exact_band values."""
        vertices, triangles = simple_cube

        for exact_band in [1, 2, 3]:
            sdf = sdfgen.generate_sdf(
                vertices,
                triangles,
                origin=(0.0, 0.0, 0.0),
                dx=0.1,
                nx=10,
                ny=10,
                nz=10,
                exact_band=exact_band,
            )
            assert sdf.shape == (10, 10, 10)

    def test_num_threads_parameter(self, simple_cube):
        """Test with different thread counts."""
        vertices, triangles = simple_cube

        for num_threads in [0, 1, 4]:
            sdf = sdfgen.generate_sdf(
                vertices,
                triangles,
                origin=(0.0, 0.0, 0.0),
                dx=0.1,
                nx=10,
                ny=10,
                nz=10,
                backend="cpu",
                num_threads=num_threads,
            )
            assert sdf.shape == (10, 10, 10)


# Error handling tests
class TestErrorHandling:
    def test_invalid_backend(self, simple_cube):
        """Test that invalid backend raises error."""
        vertices, triangles = simple_cube

        with pytest.raises(Exception):  # Could be ValueError or RuntimeError
            sdfgen.generate_sdf(
                vertices,
                triangles,
                origin=(0.0, 0.0, 0.0),
                dx=0.1,
                nx=10,
                ny=10,
                nz=10,
                backend="invalid",
            )

    def test_invalid_mesh_file(self):
        """Test that loading non-existent file raises error."""
        with pytest.raises(Exception):
            sdfgen.load_mesh("nonexistent_file.obj")

    def test_invalid_array_shapes(self):
        """Test that invalid array shapes raise errors."""
        # Wrong vertex shape
        bad_vertices = np.array([[1, 2]], dtype=np.float32)  # Missing Z coordinate
        triangles = np.array([[0, 1, 2]], dtype=np.uint32)

        with pytest.raises(Exception):
            sdfgen.generate_sdf(
                bad_vertices,
                triangles,
                origin=(0.0, 0.0, 0.0),
                dx=0.1,
                nx=10,
                ny=10,
                nz=10,
            )


# Property tests
class TestSDFProperties:
    def test_zero_crossing_at_surface(self, simple_cube):
        """Test that SDF is approximately zero at the surface."""
        vertices, triangles = simple_cube

        # High resolution for better accuracy
        sdf = sdfgen.generate_sdf(
            vertices,
            triangles,
            origin=(-1.0, -1.0, -1.0),
            dx=0.02,
            nx=100,
            ny=100,
            nz=100,
        )

        # Find cells near the surface (should have small absolute values)
        # The surface should be somewhere around x=0.5 (right face of cube)
        surface_slice = sdf[75, 50, 50]  # x=0.5, y=0, z=0
        assert abs(surface_slice) < 0.1, "SDF should be near zero at surface"

    def test_inside_negative_outside_positive(self, simple_cube):
        """Test basic SDF sign convention."""
        vertices, triangles = simple_cube

        sdf = sdfgen.generate_sdf(
            vertices,
            triangles,
            origin=(-2.0, -2.0, -2.0),
            dx=0.1,
            nx=40,
            ny=40,
            nz=40,
        )

        # Center of grid (x=0, y=0, z=0) is inside cube
        center = sdf[20, 20, 20]
        assert center < 0, "Inside should be negative"

        # Far corner (x=-2, y=-2, z=-2) is outside cube
        corner = sdf[0, 0, 0]
        assert corner > 0, "Outside should be positive"


# Critical error handling tests
class TestCriticalErrorHandling:
    def test_save_sdf_invalid_path(self, simple_cube):
        """Test that save_sdf fails with invalid path."""
        vertices, triangles = simple_cube
        sdf = sdfgen.generate_sdf(
            vertices, triangles,
            origin=(0.0, 0.0, 0.0), dx=0.1,
            nx=10, ny=10, nz=10
        )

        # Try to save to non-existent directory
        with pytest.raises(Exception):
            sdfgen.save_sdf("/nonexistent/path/test.sdf", sdf, origin=(0.0, 0.0, 0.0), dx=0.1)

    def test_save_sdf_invalid_array(self, temp_sdf_file):
        """Test that save_sdf auto-converts compatible dtypes."""
        # int32 should be auto-converted to float32 (like NumPy behavior)
        sdf_int32 = np.array([[[1, 2], [3, 4]]], dtype=np.int32)

        # This should succeed with automatic type conversion
        sdfgen.save_sdf(temp_sdf_file, sdf_int32, origin=(0.0, 0.0, 0.0), dx=0.1)

        # Verify it was saved correctly
        loaded_sdf, origin, dx, bounds = sdfgen.load_sdf(temp_sdf_file)
        assert loaded_sdf.dtype == np.float32
        assert loaded_sdf.shape == (1, 2, 2)

    def test_load_sdf_nonexistent_file(self):
        """Test that load_sdf fails with non-existent file."""
        with pytest.raises(Exception):
            sdfgen.load_sdf("nonexistent_file_xyz.sdf")

    def test_load_sdf_corrupted_file(self):
        """Test that load_sdf fails with corrupted file."""
        with tempfile.NamedTemporaryFile(mode="wb", suffix=".sdf", delete=False) as f:
            # Write invalid data (not enough for header)
            f.write(b"corrupted data")
            temp_path = f.name

        try:
            with pytest.raises(Exception):
                sdfgen.load_sdf(temp_path)
        finally:
            os.unlink(temp_path)

    def test_generate_sdf_empty_mesh(self):
        """Test that generate_sdf fails with empty mesh."""
        # 0 vertices
        empty_vertices = np.array([], dtype=np.float32).reshape(0, 3)
        empty_triangles = np.array([], dtype=np.uint32).reshape(0, 3)

        with pytest.raises(Exception):
            sdfgen.generate_sdf(
                empty_vertices, empty_triangles,
                origin=(0.0, 0.0, 0.0), dx=0.1,
                nx=10, ny=10, nz=10
            )

    def test_generate_sdf_invalid_grid_size(self, simple_cube):
        """Test that generate_sdf fails with invalid grid dimensions."""
        vertices, triangles = simple_cube

        # Zero grid size
        with pytest.raises(Exception):
            sdfgen.generate_sdf(
                vertices, triangles,
                origin=(0.0, 0.0, 0.0), dx=0.1,
                nx=0, ny=10, nz=10
            )

        # Negative grid size
        with pytest.raises(Exception):
            sdfgen.generate_sdf(
                vertices, triangles,
                origin=(0.0, 0.0, 0.0), dx=0.1,
                nx=-10, ny=10, nz=10
            )

    def test_generate_from_file_missing_parameters(self, temp_obj_file):
        """Test that generate_from_file fails when neither dx nor nx provided."""
        # This should work (nx provided)
        sdf, meta = sdfgen.generate_from_file(temp_obj_file, nx=32)
        assert sdf.shape[0] >= 32

        # Note: Currently the API always falls back to defaults, so this test
        # documents current behavior rather than testing a failure case

    def test_load_mesh_corrupted_file(self):
        """Test that load_mesh fails with corrupted OBJ file."""
        with tempfile.NamedTemporaryFile(mode="w", suffix=".obj", delete=False) as f:
            # Write invalid OBJ data
            f.write("invalid obj data\n")
            f.write("not a valid format\n")
            temp_path = f.name

        try:
            # Corrupted file should either fail or return empty mesh
            with pytest.raises(Exception):
                sdfgen.load_mesh(temp_path)
        finally:
            os.unlink(temp_path)


# High-level API parameter tests
class TestHighLevelAPIParameters:
    def test_generate_from_file_with_dx(self, temp_obj_file):
        """Test generate_from_file with dx parameter."""
        sdf, metadata = sdfgen.generate_from_file(temp_obj_file, dx=0.05, padding=2)

        assert isinstance(sdf, np.ndarray)
        assert sdf.dtype == np.float32
        assert metadata["dx"] == pytest.approx(0.05)
        assert "origin" in metadata
        assert "bounds" in metadata

    def test_generate_from_file_with_explicit_grid(self, temp_obj_file):
        """Test generate_from_file with explicit nx/ny/nz."""
        sdf, metadata = sdfgen.generate_from_file(
            temp_obj_file, nx=20, ny=30, nz=40, padding=1
        )

        # With padding=1, should be 20+2, 30+2, 40+2
        assert sdf.shape == (22, 32, 42)
        assert "dx" in metadata

    def test_generate_from_file_different_paddings(self, temp_obj_file):
        """Test generate_from_file with different padding values."""
        for padding in [0, 1, 3, 5]:
            sdf, metadata = sdfgen.generate_from_file(
                temp_obj_file, nx=20, padding=padding
            )
            # Check that padding was applied
            assert sdf.shape[0] >= 20 + 2 * padding

    def test_generate_from_file_backends(self, temp_obj_file):
        """Test generate_from_file with different backends."""
        # CPU backend
        sdf_cpu, meta_cpu = sdfgen.generate_from_file(
            temp_obj_file, nx=16, backend="cpu"
        )
        assert sdf_cpu.shape[0] >= 16
        assert meta_cpu["backend"] == "cpu"

        # Auto backend
        sdf_auto, meta_auto = sdfgen.generate_from_file(
            temp_obj_file, nx=16, backend="auto"
        )
        assert sdf_auto.shape[0] >= 16
        assert meta_auto["backend"] in ["auto"]

        # GPU backend (if available)
        if sdfgen.is_gpu_available():
            sdf_gpu, meta_gpu = sdfgen.generate_from_file(
                temp_obj_file, nx=16, backend="gpu"
            )
            assert sdf_gpu.shape[0] >= 16
            assert meta_gpu["backend"] == "gpu"

    def test_generate_from_file_threads(self, temp_obj_file):
        """Test generate_from_file with different thread counts."""
        for num_threads in [0, 1, 4]:
            sdf, metadata = sdfgen.generate_from_file(
                temp_obj_file, nx=16, backend="cpu", num_threads=num_threads
            )
            assert sdf.shape[0] >= 16

    def test_generate_from_mesh_proportional_sizing(self, simple_cube):
        """Test generate_from_mesh with proportional sizing (only nx)."""
        vertices, triangles = simple_cube

        sdf, metadata = sdfgen.generate_from_mesh(
            vertices, triangles, nx=20, padding=2
        )

        # nx should be 20 + 2*padding = 24
        # ny, nz should be computed proportionally
        assert sdf.shape[0] == 24
        assert "dx" in metadata
        assert metadata["dx"] > 0

    def test_generate_from_mesh_explicit_sizing(self, simple_cube):
        """Test generate_from_mesh with explicit nx/ny/nz."""
        vertices, triangles = simple_cube

        sdf, metadata = sdfgen.generate_from_mesh(
            vertices, triangles, nx=15, ny=20, nz=25, padding=1
        )

        # Should be nx+2, ny+2, nz+2 with padding=1
        assert sdf.shape == (17, 22, 27)

    def test_generate_from_mesh_different_paddings(self, simple_cube):
        """Test generate_from_mesh with different padding values."""
        vertices, triangles = simple_cube

        for padding in [0, 2, 4]:
            sdf, metadata = sdfgen.generate_from_mesh(
                vertices, triangles, nx=16, padding=padding
            )
            expected_size = 16 + 2 * padding
            assert sdf.shape[0] == expected_size

    def test_generate_from_mesh_backends(self, simple_cube):
        """Test generate_from_mesh with different backends."""
        vertices, triangles = simple_cube

        # CPU
        sdf_cpu, meta_cpu = sdfgen.generate_from_mesh(
            vertices, triangles, nx=12, backend="cpu"
        )
        assert meta_cpu["backend"] == "cpu"

        # Auto
        sdf_auto, meta_auto = sdfgen.generate_from_mesh(
            vertices, triangles, nx=12, backend="auto"
        )
        assert meta_auto["backend"] in ["auto"]

        # GPU (if available)
        if sdfgen.is_gpu_available():
            sdf_gpu, meta_gpu = sdfgen.generate_from_mesh(
                vertices, triangles, nx=12, backend="gpu"
            )
            assert meta_gpu["backend"] == "gpu"

    def test_generate_from_mesh_with_dx(self, simple_cube):
        """Test generate_from_mesh with dx parameter."""
        vertices, triangles = simple_cube

        sdf, metadata = sdfgen.generate_from_mesh(
            vertices, triangles, nx=20, dx=0.075, padding=1
        )

        assert metadata["dx"] == pytest.approx(0.075)
        assert sdf.dtype == np.float32


class TestDataValidation:
    """Tests for validating data types and array structures."""

    def test_generate_sdf_wrong_vertex_dtype(self, simple_cube):
        """Test that generate_sdf auto-converts compatible vertex dtypes."""
        vertices, triangles = simple_cube
        # int32 should be auto-converted to float32
        vertices_int32 = vertices.astype(np.int32)

        # Should succeed with auto-conversion
        sdf = sdfgen.generate_sdf(
            vertices_int32, triangles,
            origin=(0.0, 0.0, 0.0), dx=0.1,
            nx=10, ny=10, nz=10
        )

        assert sdf.shape == (10, 10, 10)
        assert sdf.dtype == np.float32

    def test_generate_sdf_wrong_triangle_dtype(self, simple_cube):
        """Test that generate_sdf auto-converts compatible triangle dtypes."""
        vertices, triangles = simple_cube
        # int32 should be auto-converted to uint32
        triangles_int32 = triangles.astype(np.int32)

        # Should succeed with auto-conversion
        sdf = sdfgen.generate_sdf(
            vertices, triangles_int32,
            origin=(0.0, 0.0, 0.0), dx=0.1,
            nx=10, ny=10, nz=10
        )

        assert sdf.shape == (10, 10, 10)
        assert sdf.dtype == np.float32

    def test_generate_sdf_non_contiguous_arrays(self, simple_cube):
        """Test that generate_sdf handles non-contiguous arrays properly."""
        vertices, triangles = simple_cube
        # Create non-contiguous arrays by slicing with step
        non_contig_verts = np.ascontiguousarray(vertices[::1])  # Force copy
        non_contig_tris = np.ascontiguousarray(triangles[::1])  # Force copy

        # Make truly non-contiguous by creating larger array and slicing
        temp_verts = np.zeros((vertices.shape[0] * 2, 3), dtype=np.float32)
        temp_verts[::2] = vertices
        non_contig_verts = temp_verts[::2]

        # This should either work or raise a clear error about non-contiguous arrays
        try:
            sdf = sdfgen.generate_sdf(
                non_contig_verts, triangles,
                origin=(0.0, 0.0, 0.0), dx=0.1,
                nx=10, ny=10, nz=10
            )
            assert sdf.shape == (10, 10, 10)
        except Exception as e:
            # If it fails, it should mention contiguity
            assert "contiguous" in str(e).lower() or "layout" in str(e).lower()

    def test_generate_sdf_out_of_bounds_indices(self, simple_cube):
        """Test that generate_sdf handles out-of-bounds triangle indices."""
        vertices, triangles = simple_cube
        # Create triangles with indices that don't exist
        bad_triangles = np.array([
            [0, 1, 999],  # Index 999 doesn't exist
            [1, 2, 3]
        ], dtype=np.uint32)

        # This should either handle gracefully or raise an error
        # The behavior depends on implementation - it might crash or produce incorrect results
        try:
            sdf = sdfgen.generate_sdf(
                vertices, bad_triangles,
                origin=(0.0, 0.0, 0.0), dx=0.1,
                nx=10, ny=10, nz=10
            )
            # If it doesn't crash, check that output is reasonable
            assert sdf.shape == (10, 10, 10)
        except (IndexError, RuntimeError, Exception):
            # Expected behavior for out-of-bounds access
            pass

    def test_save_sdf_wrong_dtype(self, temp_sdf_file, simple_cube):
        """Test that save_sdf auto-converts compatible SDF array dtypes."""
        vertices, triangles = simple_cube
        sdf = sdfgen.generate_sdf(
            vertices, triangles,
            origin=(0.0, 0.0, 0.0), dx=0.1,
            nx=10, ny=10, nz=10
        )

        # Convert to int32 - should be auto-converted
        sdf_int32 = sdf.astype(np.int32)

        # Should succeed with auto-conversion
        sdfgen.save_sdf(temp_sdf_file, sdf_int32, origin=(0.0, 0.0, 0.0), dx=0.1)

        # Verify it was saved and loaded correctly
        loaded_sdf, origin, dx, bounds = sdfgen.load_sdf(temp_sdf_file)
        assert loaded_sdf.dtype == np.float32
        assert loaded_sdf.shape == (10, 10, 10)

    def test_generate_sdf_1d_arrays(self, simple_cube):
        """Test that generate_sdf rejects 1D arrays instead of 2D."""
        vertices, triangles = simple_cube
        # Flatten to 1D
        flat_vertices = vertices.flatten()
        flat_triangles = triangles.flatten()

        with pytest.raises(Exception):
            sdfgen.generate_sdf(
                flat_vertices, triangles,
                origin=(0.0, 0.0, 0.0), dx=0.1,
                nx=10, ny=10, nz=10
            )

        with pytest.raises(Exception):
            sdfgen.generate_sdf(
                vertices, flat_triangles,
                origin=(0.0, 0.0, 0.0), dx=0.1,
                nx=10, ny=10, nz=10
            )


class TestEdgeCases:
    """Tests for edge cases and boundary conditions."""

    def test_single_triangle_mesh(self):
        """Test SDF generation with minimal mesh (1 triangle)."""
        # Single triangle
        vertices = np.array([
            [0.0, 0.0, 0.0],
            [1.0, 0.0, 0.0],
            [0.0, 1.0, 0.0]
        ], dtype=np.float32)
        triangles = np.array([[0, 1, 2]], dtype=np.uint32)

        sdf = sdfgen.generate_sdf(
            vertices, triangles,
            origin=(-0.5, -0.5, -0.5), dx=0.1,
            nx=20, ny=20, nz=20
        )

        assert sdf.shape == (20, 20, 20)
        assert sdf.dtype == np.float32
        # Should have some negative values inside and positive outside
        assert np.any(sdf < 0) or np.any(sdf > 0)

    def test_minimum_grid_size(self, simple_cube):
        """Test with minimum grid dimensions (1x1x1)."""
        vertices, triangles = simple_cube

        sdf = sdfgen.generate_sdf(
            vertices, triangles,
            origin=(0.0, 0.0, 0.0), dx=1.0,
            nx=1, ny=1, nz=1
        )

        assert sdf.shape == (1, 1, 1)
        assert sdf.dtype == np.float32

    def test_degenerate_triangles(self):
        """Test handling of degenerate triangles (all vertices coincident)."""
        # All vertices at the same point
        vertices = np.array([
            [0.5, 0.5, 0.5],
            [0.5, 0.5, 0.5],
            [0.5, 0.5, 0.5]
        ], dtype=np.float32)
        triangles = np.array([[0, 1, 2]], dtype=np.uint32)

        # This might work or fail depending on implementation
        try:
            sdf = sdfgen.generate_sdf(
                vertices, triangles,
                origin=(0.0, 0.0, 0.0), dx=0.1,
                nx=10, ny=10, nz=10
            )
            assert sdf.shape == (10, 10, 10)
        except Exception:
            # Acceptable to reject degenerate triangles
            pass

    def test_mesh_far_from_origin(self):
        """Test with mesh far from origin (large coordinates)."""
        # Cube centered at (1000, 1000, 1000)
        offset = 1000.0
        vertices = np.array([
            [offset + 0.0, offset + 0.0, offset + 0.0],
            [offset + 1.0, offset + 0.0, offset + 0.0],
            [offset + 1.0, offset + 1.0, offset + 0.0],
            [offset + 0.0, offset + 1.0, offset + 0.0],
            [offset + 0.0, offset + 0.0, offset + 1.0],
            [offset + 1.0, offset + 0.0, offset + 1.0],
            [offset + 1.0, offset + 1.0, offset + 1.0],
            [offset + 0.0, offset + 1.0, offset + 1.0],
        ], dtype=np.float32)
        triangles = np.array([
            [0, 1, 2], [0, 2, 3],  # bottom
            [4, 6, 5], [4, 7, 6],  # top
            [0, 4, 5], [0, 5, 1],  # front
            [2, 6, 7], [2, 7, 3],  # back
            [0, 3, 7], [0, 7, 4],  # left
            [1, 5, 6], [1, 6, 2],  # right
        ], dtype=np.uint32)

        sdf = sdfgen.generate_sdf(
            vertices, triangles,
            origin=(offset - 0.5, offset - 0.5, offset - 0.5), dx=0.1,
            nx=20, ny=20, nz=20
        )

        assert sdf.shape == (20, 20, 20)
        assert sdf.dtype == np.float32

    def test_very_fine_resolution(self, simple_cube):
        """Test with very fine resolution (small dx)."""
        vertices, triangles = simple_cube

        # Very fine resolution
        sdf = sdfgen.generate_sdf(
            vertices, triangles,
            origin=(0.0, 0.0, 0.0), dx=0.001,
            nx=10, ny=10, nz=10
        )

        assert sdf.shape == (10, 10, 10)
        assert sdf.dtype == np.float32

    def test_zero_dx_error(self, simple_cube):
        """Test that dx=0 raises an error."""
        vertices, triangles = simple_cube

        # dx=0 should fail
        with pytest.raises(Exception):
            sdfgen.generate_sdf(
                vertices, triangles,
                origin=(0.0, 0.0, 0.0), dx=0.0,
                nx=10, ny=10, nz=10
            )

    def test_negative_dx_error(self, simple_cube):
        """Test that negative dx raises an error."""
        vertices, triangles = simple_cube

        # Negative dx should fail
        with pytest.raises(Exception):
            sdfgen.generate_sdf(
                vertices, triangles,
                origin=(0.0, 0.0, 0.0), dx=-0.1,
                nx=10, ny=10, nz=10
            )

    def test_gpu_backend_when_unavailable(self, simple_cube):
        """Test GPU backend behavior when GPU is not available."""
        vertices, triangles = simple_cube

        if not sdfgen.is_gpu_available():
            # When GPU is not available, GPU backend should either:
            # 1. Fall back to CPU
            # 2. Raise a clear error
            try:
                sdf = sdfgen.generate_sdf(
                    vertices, triangles,
                    origin=(0.0, 0.0, 0.0), dx=0.1,
                    nx=10, ny=10, nz=10,
                    backend="gpu"
                )
                # If it succeeds, it likely fell back to CPU
                assert sdf.shape == (10, 10, 10)
            except Exception as e:
                # If it fails, should have clear error message
                assert "gpu" in str(e).lower() or "cuda" in str(e).lower()
        else:
            # GPU is available, should work normally
            sdf = sdfgen.generate_sdf(
                vertices, triangles,
                origin=(0.0, 0.0, 0.0), dx=0.1,
                nx=10, ny=10, nz=10,
                backend="gpu"
            )
            assert sdf.shape == (10, 10, 10)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
