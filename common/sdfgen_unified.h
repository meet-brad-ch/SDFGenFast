// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#pragma once

#include "array3.h"
#include "vec.h"
#include <vector>

namespace sdfgen {

// Hardware backend selection for SDF generation
enum class HardwareBackend {
    Auto,  // Try GPU first, fall back to CPU
    CPU,   // Force CPU implementation
    GPU    // Force GPU implementation (fails if CUDA not available)
};

// Unified interface for SDF generation
// This function abstracts away CPU/GPU selection and provides a single entry point
// for all SDF generation operations.
//
// Parameters:
//   - tri: Triangle indices (mesh topology)
//   - x: Vertex positions (mesh geometry)
//   - origin: Grid origin point in world space
//   - dx: Grid cell spacing
//   - nx, ny, nz: Grid dimensions
//   - phi: Output SDF grid (will be resized)
//   - exact_band: Distance band for exact computation (default: 1)
//   - backend: Hardware selection (default: Auto)
//   - num_threads: CPU thread count (0 = auto-detect, only used for CPU backend)
//
// Returns: void (result stored in phi parameter)
void make_level_set3(
    const std::vector<Vec3ui>& tri,
    const std::vector<Vec3f>& x,
    const Vec3f& origin,
    float dx,
    int nx, int ny, int nz,
    Array3f& phi,
    int exact_band = 1,
    HardwareBackend backend = HardwareBackend::Auto,
    int num_threads = 0
);

// Query available hardware backends
bool is_gpu_available();

} // namespace sdfgen
