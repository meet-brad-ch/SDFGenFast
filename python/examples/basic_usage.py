"""
Basic usage examples for SDFGen Python bindings
"""

import sdfgen
import numpy as np


def example_1_load_and_generate():
    """Example 1: Load mesh and generate SDF"""
    print("Example 1: Load mesh and generate SDF")
    print("=" * 50)

    # Load a mesh file (OBJ or STL)
    mesh_file = "bunny.obj"  # Replace with your mesh file

    try:
        vertices, triangles, bounds = sdfgen.load_mesh(mesh_file)
        print(f"Loaded mesh: {vertices.shape[0]} vertices, {triangles.shape[0]} triangles")
        print(f"Bounds: {bounds}")

        # Generate SDF with 64x64x64 grid
        sdf = sdfgen.generate_sdf(
            vertices,
            triangles,
            origin=bounds[0],
            dx=0.01,
            nx=64,
            ny=64,
            nz=64,
            backend="auto",  # Try GPU, fall back to CPU
        )

        print(f"Generated SDF: {sdf.shape}")
        print(f"Min distance: {sdf.min():.4f}, Max distance: {sdf.max():.4f}")
        print(f"Backend used: {sdfgen.get_active_backend()}")

    except Exception as e:
        print(f"Error: {e}")
        print("Make sure to replace 'bunny.obj' with a valid mesh file path")

    print()


def example_2_high_level_api():
    """Example 2: Using high-level API"""
    print("Example 2: High-level API (generate_from_file)")
    print("=" * 50)

    mesh_file = "dragon.stl"  # Replace with your mesh file

    try:
        # Generate SDF with automatic grid sizing
        sdf, metadata = sdfgen.generate_from_file(
            mesh_file,
            nx=256,  # Will create proportional grid
            padding=2,
            backend="auto",
        )

        print(f"Generated SDF: {sdf.shape}")
        print(f"Cell size: {metadata['dx']:.6f}")
        print(f"Origin: {metadata['origin']}")
        print(f"Bounds: {metadata['bounds']}")
        print(f"Backend: {metadata['backend']}")

    except Exception as e:
        print(f"Error: {e}")
        print("Make sure to replace 'dragon.stl' with a valid mesh file path")

    print()


def example_3_programmatic_mesh():
    """Example 3: Generate SDF from programmatically created mesh"""
    print("Example 3: Programmatic mesh (cube)")
    print("=" * 50)

    # Create a simple cube mesh
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

    triangles = np.array(
        [
            [0, 1, 2], [0, 2, 3],  # Front
            [4, 6, 5], [4, 7, 6],  # Back
            [0, 3, 7], [0, 7, 4],  # Left
            [1, 5, 6], [1, 6, 2],  # Right
            [0, 4, 5], [0, 5, 1],  # Bottom
            [3, 2, 6], [3, 6, 7],  # Top
        ],
        dtype=np.uint32,
    )

    # Generate SDF
    sdf, metadata = sdfgen.generate_from_mesh(
        vertices,
        triangles,
        nx=32,
        padding=2,
        backend="cpu",
        num_threads=4,
    )

    print(f"Generated SDF: {sdf.shape}")
    print(f"Cell size: {metadata['dx']:.6f}")
    print(f"Backend: {metadata['backend']}")

    # Analyze the SDF
    center_idx = tuple(s // 2 for s in sdf.shape)
    center_value = sdf[center_idx]
    print(f"Value at center: {center_value:.6f} (should be negative)")

    # Find zero crossing (surface)
    zero_crossings = np.sum(np.abs(sdf) < 0.01)
    print(f"Cells near surface (|sdf| < 0.01): {zero_crossings}")

    print()


def example_4_save_and_load():
    """Example 4: Save and load SDF files"""
    print("Example 4: Save and load SDF")
    print("=" * 50)

    # Create a simple mesh
    vertices = np.array(
        [[0, 0, 0], [1, 0, 0], [0.5, 1, 0]], dtype=np.float32
    )
    triangles = np.array([[0, 1, 2]], dtype=np.uint32)

    # Generate SDF
    sdf = sdfgen.generate_sdf(
        vertices,
        triangles,
        origin=(-0.5, -0.5, -0.5),
        dx=0.05,
        nx=30,
        ny=30,
        nz=30,
    )

    # Save to file
    output_file = "triangle.sdf"
    sdfgen.save_sdf(output_file, sdf, origin=(-0.5, -0.5, -0.5), dx=0.05)
    print(f"Saved SDF to {output_file}")

    # Load back
    loaded_sdf, origin, dx, bounds = sdfgen.load_sdf(output_file)
    print(f"Loaded SDF: {loaded_sdf.shape}")
    print(f"Origin: {origin}")
    print(f"Cell size: {dx}")
    print(f"Bounds: {bounds}")

    # Verify they match
    if np.allclose(sdf, loaded_sdf):
        print("✓ Loaded SDF matches original")
    else:
        print("✗ Mismatch between original and loaded SDF")

    print()


def example_5_backend_comparison():
    """Example 5: Compare CPU and GPU backends"""
    print("Example 5: Backend comparison")
    print("=" * 50)

    # Check GPU availability
    gpu_available = sdfgen.is_gpu_available()
    print(f"GPU available: {gpu_available}")

    if not gpu_available:
        print("Skipping GPU comparison (no GPU available)")
        print()
        return

    # Create test mesh
    vertices = np.array(
        [[-1, -1, -1], [1, -1, -1], [0, 1, -1], [0, 0, 1]], dtype=np.float32
    )
    triangles = np.array(
        [[0, 1, 2], [0, 1, 3], [1, 2, 3], [2, 0, 3]], dtype=np.uint32
    )

    import time

    # CPU benchmark
    start = time.time()
    sdf_cpu = sdfgen.generate_sdf(
        vertices,
        triangles,
        origin=(-2, -2, -2),
        dx=0.02,
        nx=100,
        ny=100,
        nz=100,
        backend="cpu",
    )
    cpu_time = time.time() - start

    # GPU benchmark
    start = time.time()
    sdf_gpu = sdfgen.generate_sdf(
        vertices,
        triangles,
        origin=(-2, -2, -2),
        dx=0.02,
        nx=100,
        ny=100,
        nz=100,
        backend="gpu",
    )
    gpu_time = time.time() - start

    print(f"CPU time: {cpu_time:.3f} seconds")
    print(f"GPU time: {gpu_time:.3f} seconds")
    print(f"Speedup: {cpu_time / gpu_time:.2f}x")

    # Check consistency
    max_diff = np.abs(sdf_cpu - sdf_gpu).max()
    print(f"Max difference: {max_diff:.6e}")

    if np.allclose(sdf_cpu, sdf_gpu, rtol=1e-5):
        print("✓ CPU and GPU results match")
    else:
        print("✗ CPU and GPU results differ")

    print()


def example_6_different_resolutions():
    """Example 6: Generate SDFs at different resolutions"""
    print("Example 6: Multi-resolution SDF generation")
    print("=" * 50)

    # Create a simple sphere-like mesh (octahedron)
    vertices = np.array(
        [[1, 0, 0], [-1, 0, 0], [0, 1, 0], [0, -1, 0], [0, 0, 1], [0, 0, -1]],
        dtype=np.float32,
    )

    triangles = np.array(
        [
            [0, 2, 4], [0, 4, 3], [0, 3, 5], [0, 5, 2],
            [1, 4, 2], [1, 3, 4], [1, 5, 3], [1, 2, 5],
        ],
        dtype=np.uint32,
    )

    resolutions = [16, 32, 64, 128]

    for res in resolutions:
        sdf = sdfgen.generate_sdf(
            vertices,
            triangles,
            origin=(-2, -2, -2),
            dx=4.0 / res,
            nx=res,
            ny=res,
            nz=res,
        )

        # Analyze surface quality
        near_surface = np.sum(np.abs(sdf) < 0.1)
        print(f"Resolution {res}³: {near_surface} cells near surface")

    print()


if __name__ == "__main__":
    print("SDFGen Python Bindings - Examples")
    print("=" * 50)
    print()

    # Run examples
    example_1_load_and_generate()
    example_2_high_level_api()
    example_3_programmatic_mesh()
    example_4_save_and_load()
    example_5_backend_comparison()
    example_6_different_resolutions()

    print("Done!")
