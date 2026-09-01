// SDFGen Python Bindings
// Copyright (c) 2025 Brad Chamberlain
// Licensed under the MIT License

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/tuple.h>

#include "sdfgen_unified.h"
#include "mesh_io.h"
#include "sdf_io.h"
#include "array3.h"
#include "vec.h"

namespace nb = nanobind;
using namespace nb::literals;

/**
 * @brief Convert an (N, 3) NumPy array to a vector of 3-component vectors
 *
 * Shared implementation for float32 vertices (Vec3f) and uint32 triangle
 * indices (Vec3ui).
 *
 * @tparam VecT Destination vector type (Vec3f or Vec3ui)
 * @tparam ScalarT Source NumPy scalar type (float or uint32_t)
 * @param arr NumPy ndarray with shape (N, 3), C-contiguous
 * @return std::vector containing N VecT values
 */
template <typename VecT, typename ScalarT>
static std::vector<VecT> numpy_to_vec3(
    const nb::ndarray<ScalarT, nb::shape<-1, 3>, nb::c_contig>& arr) {
    size_t n = arr.shape(0);
    std::vector<VecT> result(n);

    auto data = arr.data();
    for (size_t i = 0; i < n; ++i) {
        result[i] = VecT(data[i * 3 + 0], data[i * 3 + 1], data[i * 3 + 2]);
    }

    return result;
}

/**
 * @brief Convert a vector of 3-component vectors to an (N, 3) NumPy array
 *
 * Shared implementation for the vertex and triangle arrays returned by
 * load_mesh. The copy is owned by a capsule and freed by Python.
 */
template <typename ScalarT, typename VecT>
static nb::ndarray<nb::numpy, ScalarT, nb::shape<-1, 3>> vec3_to_numpy(
    const std::vector<VecT>& items) {
    size_t n = items.size();
    ScalarT* data = new ScalarT[n * 3];
    for (size_t i = 0; i < n; ++i) {
        data[i * 3 + 0] = items[i][0];
        data[i * 3 + 1] = items[i][1];
        data[i * 3 + 2] = items[i][2];
    }

    nb::capsule owner(data, [](void* p) noexcept {
        delete[] static_cast<ScalarT*>(p);
    });

    size_t shape[2] = {n, 3};
    return nb::ndarray<nb::numpy, ScalarT, nb::shape<-1, 3>>(data, 2, shape, owner);
}

/**
 * @brief Convert C++ Array3f SDF grid to NumPy array
 *
 * Copies 3D SDF grid data from internal Array3f format to NumPy ndarray with shape
 * (ni, nj, nk) and dtype float32. Returns ownership to Python for memory management.
 *
 * @param arr Input Array3f signed distance field with dimensions ni x nj x nk
 * @return NumPy ndarray with shape (ni, nj, nk), dtype float32, C-contiguous
 */
nb::ndarray<nb::numpy, float> array3f_to_numpy(const Array3f& arr) {
    size_t ni = arr.ni;
    size_t nj = arr.nj;
    size_t nk = arr.nk;

    // Create numpy array with shape (ni, nj, nk)
    float* data = new float[ni * nj * nk];

    // Copy data from Array3f (i, j, k indexing)
    for (size_t i = 0; i < ni; ++i) {
        for (size_t j = 0; j < nj; ++j) {
            for (size_t k = 0; k < nk; ++k) {
                data[i * nj * nk + j * nk + k] = arr(i, j, k);
            }
        }
    }

    // Create capsule for memory management
    nb::capsule owner(data, [](void* p) noexcept {
        delete[] static_cast<float*>(p);
    });

    return nb::ndarray<nb::numpy, float>(
        data, {ni, nj, nk}, owner
    );
}

// Load mesh from file
nb::tuple load_mesh(const std::string& filename) {
    std::vector<Vec3f> vertices;
    std::vector<Vec3ui> triangles;
    Vec3f min_box, max_box;

    bool success = meshio::load_mesh(filename.c_str(), vertices, triangles, min_box, max_box);

    if (!success) {
        throw std::runtime_error("Failed to load mesh: " + filename);
    }

    // Convert to numpy arrays
    auto vert_array = vec3_to_numpy<float>(vertices);
    auto tri_array = vec3_to_numpy<uint32_t>(triangles);

    // Bounding box as tuple
    auto bounds = nb::make_tuple(
        nb::make_tuple(min_box[0], min_box[1], min_box[2]),
        nb::make_tuple(max_box[0], max_box[1], max_box[2])
    );

    return nb::make_tuple(vert_array, tri_array, bounds);
}

// Generate SDF from numpy arrays
nb::ndarray<nb::numpy, float> generate_sdf(
    nb::ndarray<float, nb::shape<-1, 3>, nb::c_contig> vertices,
    nb::ndarray<uint32_t, nb::shape<-1, 3>, nb::c_contig> triangles,
    nb::tuple origin,
    float dx,
    int nx, int ny, int nz,
    int exact_band = 1,
    const std::string& backend = "auto",
    int num_threads = 0
) {
    // Validate mesh is not empty
    if (vertices.shape(0) == 0 || triangles.shape(0) == 0) {
        throw std::invalid_argument("Cannot generate SDF from empty mesh (vertices or triangles are empty)");
    }

    // Validate grid parameters
    if (nx <= 0 || ny <= 0 || nz <= 0) {
        throw std::invalid_argument("Grid dimensions must be positive (nx, ny, nz > 0)");
    }

    if (dx <= 0.0f) {
        throw std::invalid_argument("Cell spacing dx must be positive");
    }

    // Convert inputs
    auto verts = numpy_to_vec3<Vec3f>(vertices);
    auto tris = numpy_to_vec3<Vec3ui>(triangles);

    Vec3f origin_vec(
        nb::cast<float>(origin[0]),
        nb::cast<float>(origin[1]),
        nb::cast<float>(origin[2])
    );

    // Parse backend
    sdfgen::HardwareBackend hw_backend = sdfgen::HardwareBackend::Auto;
    if (backend == "cpu") {
        hw_backend = sdfgen::HardwareBackend::CPU;
    } else if (backend == "gpu") {
        hw_backend = sdfgen::HardwareBackend::GPU;
    } else if (backend != "auto") {
        throw std::invalid_argument("Invalid backend: " + backend + " (must be 'auto', 'cpu', or 'gpu')");
    }

    // Generate SDF. The computation can run for seconds on large grids
    // and touches no Python state, so release the GIL for its duration.
    Array3f phi;
    {
        nb::gil_scoped_release release;
        sdfgen::make_level_set3(
            tris, verts,
            origin_vec, dx,
            nx, ny, nz,
            phi,
            exact_band,
            hw_backend,
            num_threads
        );
    }

    // Convert to numpy
    return array3f_to_numpy(phi);
}

// Save SDF to binary file
void save_sdf(
    const std::string& filename,
    nb::ndarray<float, nb::shape<-1, -1, -1>, nb::c_contig> sdf_array,
    nb::tuple origin,
    float dx
) {
    // Validate array dimensions
    if (sdf_array.ndim() != 3) {
        throw std::invalid_argument("SDF array must be 3-dimensional");
    }

    size_t nx = sdf_array.shape(0);
    size_t ny = sdf_array.shape(1);
    size_t nz = sdf_array.shape(2);

    // Validate array is not empty
    if (nx == 0 || ny == 0 || nz == 0) {
        throw std::invalid_argument("SDF array dimensions cannot be zero");
    }

    // Convert numpy array to Array3f
    Array3f phi(nx, ny, nz);
    auto data = sdf_array.data();

    for (size_t i = 0; i < nx; ++i) {
        for (size_t j = 0; j < ny; ++j) {
            for (size_t k = 0; k < nz; ++k) {
                phi(i, j, k) = data[i * ny * nz + j * nz + k];
            }
        }
    }

    Vec3f origin_vec(
        nb::cast<float>(origin[0]),
        nb::cast<float>(origin[1]),
        nb::cast<float>(origin[2])
    );

    // Save using existing I/O function
    bool success = write_sdf_binary(
        filename,
        phi,
        origin_vec,
        dx
    );

    if (!success) {
        throw std::runtime_error("Failed to write SDF file: " + filename);
    }
}

// Load SDF from binary file
nb::tuple load_sdf(const std::string& filename) {
    Array3f phi;
    Vec3f min_box, max_box;

    bool success = read_sdf_binary(
        filename,
        phi,
        min_box,
        max_box
    );

    if (!success) {
        throw std::runtime_error("Failed to read SDF file: " + filename);
    }

    // Convert to numpy
    auto sdf_array = array3f_to_numpy(phi);

    // Compute metadata
    float dx = (max_box[0] - min_box[0]) / phi.ni;
    auto origin = nb::make_tuple(min_box[0], min_box[1], min_box[2]);
    auto bounds = nb::make_tuple(
        nb::make_tuple(min_box[0], min_box[1], min_box[2]),
        nb::make_tuple(max_box[0], max_box[1], max_box[2])
    );

    return nb::make_tuple(sdf_array, origin, dx, bounds);
}

// Query GPU availability
bool is_gpu_available() {
    return sdfgen::is_gpu_available();
}

// Module definition
NB_MODULE(sdfgen_ext, m) {
    m.doc() = "Python bindings for SDFGenFast - GPU-accelerated signed distance field generation";

    // Core functions
    m.def("load_mesh", &load_mesh,
        "filename"_a,
        "Load a triangle mesh from file (OBJ or STL)\n\n"
        "Parameters\n"
        "----------\n"
        "filename : str\n"
        "    Path to mesh file (.obj or .stl)\n\n"
        "Returns\n"
        "-------\n"
        "vertices : ndarray, shape (N, 3), dtype float32\n"
        "    Vertex positions\n"
        "triangles : ndarray, shape (M, 3), dtype uint32\n"
        "    Triangle indices\n"
        "bounds : tuple\n"
        "    ((min_x, min_y, min_z), (max_x, max_y, max_z))"
    );

    m.def("generate_sdf", &generate_sdf,
        "vertices"_a, "triangles"_a,
        "origin"_a, "dx"_a,
        "nx"_a, "ny"_a, "nz"_a,
        "exact_band"_a = 1,
        "backend"_a = "auto",
        "num_threads"_a = 0,
        "Generate a signed distance field from a triangle mesh\n\n"
        "Parameters\n"
        "----------\n"
        "vertices : ndarray, shape (N, 3), dtype float32\n"
        "    Vertex positions\n"
        "triangles : ndarray, shape (M, 3), dtype uint32\n"
        "    Triangle indices (zero-based)\n"
        "origin : tuple of float\n"
        "    Grid origin (x, y, z) in world space\n"
        "dx : float\n"
        "    Grid cell spacing\n"
        "nx, ny, nz : int\n"
        "    Grid dimensions\n"
        "exact_band : int, optional\n"
        "    Distance band for exact computation (default: 1)\n"
        "backend : str, optional\n"
        "    Hardware backend: 'auto', 'cpu', or 'gpu' (default: 'auto')\n"
        "num_threads : int, optional\n"
        "    Number of CPU threads, 0 for auto-detect (default: 0)\n\n"
        "Returns\n"
        "-------\n"
        "sdf : ndarray, shape (nx, ny, nz), dtype float32\n"
        "    Signed distance field (negative inside, positive outside, zero on surface)"
    );

    m.def("save_sdf", &save_sdf,
        "filename"_a, "sdf_array"_a, "origin"_a, "dx"_a,
        "Save SDF to binary file\n\n"
        "Parameters\n"
        "----------\n"
        "filename : str\n"
        "    Output file path (.sdf)\n"
        "sdf_array : ndarray, shape (nx, ny, nz), dtype float32\n"
        "    Signed distance field\n"
        "origin : tuple of float\n"
        "    Grid origin (x, y, z)\n"
        "dx : float\n"
        "    Grid cell spacing"
    );

    m.def("load_sdf", &load_sdf,
        "filename"_a,
        "Load SDF from binary file\n\n"
        "Parameters\n"
        "----------\n"
        "filename : str\n"
        "    Input file path (.sdf)\n\n"
        "Returns\n"
        "-------\n"
        "sdf : ndarray, shape (nx, ny, nz), dtype float32\n"
        "    Signed distance field\n"
        "origin : tuple of float\n"
        "    Grid origin (x, y, z)\n"
        "dx : float\n"
        "    Grid cell spacing\n"
        "bounds : tuple\n"
        "    ((min_x, min_y, min_z), (max_x, max_y, max_z))"
    );

    // Utility functions
    m.def("is_gpu_available", &is_gpu_available,
        "Check if GPU acceleration (CUDA) is available\n\n"
        "Returns\n"
        "-------\n"
        "bool\n"
        "    True if GPU is available, False otherwise"
    );
}
