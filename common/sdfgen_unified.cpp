// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#include "sdfgen_unified.h"
#include "config.h"
#include "makelevelset3.h"

#ifdef HAVE_CUDA
#include "makelevelset3_gpu.h"
#include <cuda_runtime.h>
#endif

#include <iostream>
#include <stdexcept>

namespace sdfgen {

bool is_gpu_available() {
#ifdef HAVE_CUDA
    // Check at runtime if a CUDA-capable GPU is actually present
    int device_count = 0;
    cudaError_t error = cudaGetDeviceCount(&device_count);
    return (error == cudaSuccess && device_count > 0);
#else
    return false;
#endif
}

void make_level_set3(
    const std::vector<Vec3ui>& tri,
    const std::vector<Vec3f>& x,
    const Vec3f& origin,
    float dx,
    int nx, int ny, int nz,
    Array3f& phi,
    int exact_band,
    HardwareBackend backend,
    int num_threads)
{
    // Handle Auto mode: try GPU first (if available at runtime), fall back to CPU
    bool auto_selected = (backend == HardwareBackend::Auto);
    if (auto_selected) {
        if (is_gpu_available()) {
            backend = HardwareBackend::GPU;
        } else {
            backend = HardwareBackend::CPU;
        }
    }

    // Dispatch to appropriate implementation
    switch (backend) {
        case HardwareBackend::CPU:
            cpu::make_level_set3(tri, x, origin, dx, nx, ny, nz, phi, exact_band, num_threads);
            break;

        case HardwareBackend::GPU:
#ifdef HAVE_CUDA
            try {
                gpu::make_level_set3(tri, x, origin, dx, nx, ny, nz, phi, exact_band);
            } catch (const std::runtime_error& e) {
                // A CUDA failure on an auto-selected GPU falls back to the
                // CPU implementation. An explicitly requested GPU backend
                // propagates the error to the caller.
                if (!auto_selected) {
                    throw;
                }
                std::cerr << "Warning: GPU backend failed (" << e.what()
                          << "); falling back to CPU." << std::endl;
                cpu::make_level_set3(tri, x, origin, dx, nx, ny, nz, phi,
                                     exact_band, num_threads);
            }
#else
            throw std::runtime_error(
                "GPU backend requested but CUDA support is not available. "
                "Rebuild with CUDA enabled or use HardwareBackend::CPU."
            );
#endif
            break;

        case HardwareBackend::Auto:
            // Should never reach here due to Auto resolution above
            throw std::logic_error("Auto backend should have been resolved");
    }
}

} // namespace sdfgen
