// SDFGen - Signed Distance Field Generator
// Copyright (c) 2015 Christopher Batty, 2025 Brad Chamberlain
// Licensed under the MIT License - see LICENSE file

#include "makelevelset3_gpu.h"
#include <cuda_runtime.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstddef>

// CUDA error checking macro
#define CUDA_CHECK(err) { \
    if (err != cudaSuccess) { \
        std::cerr << "CUDA Error: " << cudaGetErrorString(err) \
                  << " in " << __FILE__ << " at line " << __LINE__ << std::endl; \
        exit(EXIT_FAILURE); \
    } \
}

namespace sdfgen {
namespace gpu {

// ============================================================================
// Data Structures
// ============================================================================

// Unified struct for atomic distance+triangle updates
struct DistTriPair {
    float dist;      // 32 bits - signed distance
    int   tri_idx;   // 32 bits - closest triangle index
};

// ============================================================================
// Device Utility Functions
// ============================================================================

__device__ __host__ inline int grid_index(int i, int j, int k, int ni, int nj) {
    return k * nj * ni + j * ni + i;
}

__device__ inline unsigned long long pack_dist_tri(float dist, int tri_idx) {
    DistTriPair dt;
    dt.dist = dist;
    dt.tri_idx = tri_idx;
    return *reinterpret_cast<unsigned long long*>(&dt);
}

__device__ inline DistTriPair unpack_dist_tri(unsigned long long packed) {
    return *reinterpret_cast<DistTriPair*>(&packed);
}

__device__ inline float fmin3(float a, float b, float c) {
    return fminf(fminf(a, b), c);
}

__device__ inline float fmax3(float a, float b, float c) {
    return fmaxf(fmaxf(a, b), c);
}

__device__ inline int clamp_int(int val, int min_val, int max_val) {
    return max(min_val, min(max_val, val));
}

// ============================================================================
// Device Geometry Functions
// ============================================================================

__device__ float mag2(const Vec3f& a) {
    return a.v[0]*a.v[0] + a.v[1]*a.v[1] + a.v[2]*a.v[2];
}

__device__ float dot_prod(const Vec3f& a, const Vec3f& b) {
    return a.v[0]*b.v[0] + a.v[1]*b.v[1] + a.v[2]*b.v[2];
}

__device__ float dist(const Vec3f& a, const Vec3f& b) {
    float dx = a.v[0] - b.v[0];
    float dy = a.v[1] - b.v[1];
    float dz = a.v[2] - b.v[2];
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

__device__ float point_segment_distance(const Vec3f& x0, const Vec3f& x1, const Vec3f& x2) {
    // dx = x2 - x1
    float dx0 = x2.v[0] - x1.v[0];
    float dx1 = x2.v[1] - x1.v[1];
    float dx2 = x2.v[2] - x1.v[2];
    double m2 = dx0*dx0 + dx1*dx1 + dx2*dx2;

    if (m2 < 1e-30) return dist(x0, x1);

    // temp = x2 - x0
    float temp0 = x2.v[0] - x0.v[0];
    float temp1 = x2.v[1] - x0.v[1];
    float temp2 = x2.v[2] - x0.v[2];

    float s12 = (float)((temp0*dx0 + temp1*dx1 + temp2*dx2) / m2);
    s12 = fmaxf(0.0f, fminf(1.0f, s12));

    // result = x1 * s12 + x2 * (1 - s12)
    float r0 = s12 * x1.v[0] + (1.0f - s12) * x2.v[0];
    float r1 = s12 * x1.v[1] + (1.0f - s12) * x2.v[1];
    float r2 = s12 * x1.v[2] + (1.0f - s12) * x2.v[2];

    float d0 = x0.v[0] - r0;
    float d1 = x0.v[1] - r1;
    float d2 = x0.v[2] - r2;
    return sqrtf(d0*d0 + d1*d1 + d2*d2);
}

__device__ float point_triangle_distance(const Vec3f& x0, const Vec3f& x1, const Vec3f& x2, const Vec3f& x3) {
    // x13 = x1 - x3
    float x13_0 = x1.v[0] - x3.v[0];
    float x13_1 = x1.v[1] - x3.v[1];
    float x13_2 = x1.v[2] - x3.v[2];

    // x23 = x2 - x3
    float x23_0 = x2.v[0] - x3.v[0];
    float x23_1 = x2.v[1] - x3.v[1];
    float x23_2 = x2.v[2] - x3.v[2];

    // x03 = x0 - x3
    float x03_0 = x0.v[0] - x3.v[0];
    float x03_1 = x0.v[1] - x3.v[1];
    float x03_2 = x0.v[2] - x3.v[2];

    float m13 = x13_0*x13_0 + x13_1*x13_1 + x13_2*x13_2;
    float m23 = x23_0*x23_0 + x23_1*x23_1 + x23_2*x23_2;
    float d = x13_0*x23_0 + x13_1*x23_1 + x13_2*x23_2;
    float invdet = 1.0f / fmaxf(m13 * m23 - d * d, 1e-30f);
    float a = x13_0*x03_0 + x13_1*x03_1 + x13_2*x03_2;
    float b = x23_0*x03_0 + x23_1*x03_1 + x23_2*x03_2;

    float w23 = invdet * (m23 * a - d * b);
    float w31 = invdet * (m13 * b - d * a);
    float w12 = 1.0f - w23 - w31;

    if (w23 >= 0.0f && w31 >= 0.0f && w12 >= 0.0f) {
        // Inside triangle - compute weighted point
        float p0 = w23*x1.v[0] + w31*x2.v[0] + w12*x3.v[0];
        float p1 = w23*x1.v[1] + w31*x2.v[1] + w12*x3.v[1];
        float p2 = w23*x1.v[2] + w31*x2.v[2] + w12*x3.v[2];

        float d0 = x0.v[0] - p0;
        float d1 = x0.v[1] - p1;
        float d2 = x0.v[2] - p2;
        return sqrtf(d0*d0 + d1*d1 + d2*d2);
    } else {
        // Clamp to edges
        if (w23 > 0.0f)
            return fminf(point_segment_distance(x0, x1, x2), point_segment_distance(x0, x1, x3));
        else if (w31 > 0.0f)
            return fminf(point_segment_distance(x0, x1, x2), point_segment_distance(x0, x2, x3));
        else
            return fminf(point_segment_distance(x0, x1, x3), point_segment_distance(x0, x2, x3));
    }
}

__device__ int orientation(double x1, double y1, double x2, double y2, double& twice_signed_area) {
    twice_signed_area = y1*x2 - x1*y2;
    if (twice_signed_area > 0) return 1;
    else if (twice_signed_area < 0) return -1;
    else if (y2 > y1) return 1;
    else if (y2 < y1) return -1;
    else if (x1 > x2) return 1;
    else if (x1 < x2) return -1;
    else return 0;
}

__device__ bool point_in_triangle_2d(double x0, double y0,
                                     double x1, double y1, double x2, double y2, double x3, double y3,
                                     double& a, double& b, double& c) {
    x1 -= x0; x2 -= x0; x3 -= x0;
    y1 -= y0; y2 -= y0; y3 -= y0;

    int signa = orientation(x2, y2, x3, y3, a);
    if (signa == 0) return false;

    int signb = orientation(x3, y3, x1, y1, b);
    if (signb != signa) return false;

    int signc = orientation(x1, y1, x2, y2, c);
    if (signc != signa) return false;

    double sum = a + b + c;
    if (fabs(sum) < 1e-30) return false;

    a /= sum;
    b /= sum;
    c /= sum;
    return true;
}

// ============================================================================
// Kernel 1: Grid Initialization
// ============================================================================

__global__ void initialize_grids_kernel(DistTriPair* dist_tri, int* intersection_count,
                                       int ni, int nj, int nk, float max_dist) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int k = blockIdx.z * blockDim.z + threadIdx.z;

    if (i >= ni || j >= nj || k >= nk) return;

    int idx = grid_index(i, j, k, ni, nj);
    dist_tri[idx].dist = max_dist;
    dist_tri[idx].tri_idx = -1;
    intersection_count[idx] = 0;
}

// ============================================================================
// Kernel 2: Near-Band Distance with 64-bit Atomic Updates
// ============================================================================

__global__ void near_band_distance_kernel(
    const Vec3ui* tri, const Vec3f* x,
    DistTriPair* dist_tri, int* intersection_count,
    int num_triangles, Vec3f origin, float dx, int ni, int nj, int nk, int exact_band)
{
    int t_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (t_idx >= num_triangles) return;

    // Load triangle vertices
    Vec3ui pqr = tri[t_idx];
    Vec3f p = x[pqr.v[0]];
    Vec3f q = x[pqr.v[1]];
    Vec3f r = x[pqr.v[2]];

    // Compute grid coordinates
    double fip = ((double)p.v[0] - origin.v[0]) / dx;
    double fjp = ((double)p.v[1] - origin.v[1]) / dx;
    double fkp = ((double)p.v[2] - origin.v[2]) / dx;
    double fiq = ((double)q.v[0] - origin.v[0]) / dx;
    double fjq = ((double)q.v[1] - origin.v[1]) / dx;
    double fkq = ((double)q.v[2] - origin.v[2]) / dx;
    double fir = ((double)r.v[0] - origin.v[0]) / dx;
    double fjr = ((double)r.v[1] - origin.v[1]) / dx;
    double fkr = ((double)r.v[2] - origin.v[2]) / dx;

    // Distance computation bounding box
    int i0 = clamp_int((int)(fmin3(fip, fiq, fir)) - exact_band, 0, ni - 1);
    int i1 = clamp_int((int)(fmax3(fip, fiq, fir)) + exact_band + 1, 0, ni - 1);
    int j0 = clamp_int((int)(fmin3(fjp, fjq, fjr)) - exact_band, 0, nj - 1);
    int j1 = clamp_int((int)(fmax3(fjp, fjq, fjr)) + exact_band + 1, 0, nj - 1);
    int k0 = clamp_int((int)(fmin3(fkp, fkq, fkr)) - exact_band, 0, nk - 1);
    int k1 = clamp_int((int)(fmax3(fkp, fkq, fkr)) + exact_band + 1, 0, nk - 1);

    // Compute distances
    for (int k = k0; k <= k1; ++k) {
        for (int j = j0; j <= j1; ++j) {
            for (int i = i0; i <= i1; ++i) {
                // Create grid point without calling constructor
                float gx_data[3] = {i * dx + origin.v[0], j * dx + origin.v[1], k * dx + origin.v[2]};
                const Vec3f& gx = *reinterpret_cast<Vec3f*>(gx_data);

                float d = point_triangle_distance(gx, p, q, r);
                int idx = grid_index(i, j, k, ni, nj);

                // 64-bit atomic update
                unsigned long long* addr = (unsigned long long*)&dist_tri[idx];
                unsigned long long old_val = *addr;
                DistTriPair old_dt = unpack_dist_tri(old_val);

                while (d < old_dt.dist) {
                    unsigned long long new_val = pack_dist_tri(d, t_idx);
                    unsigned long long prev = atomicCAS(addr, old_val, new_val);
                    if (prev == old_val) break;
                    old_val = prev;
                    old_dt = unpack_dist_tri(old_val);
                }
            }
        }
    }

    // Intersection counting
    int j0_int = clamp_int((int)ceil(fmin3(fjp, fjq, fjr)), 0, nj - 1);
    int j1_int = clamp_int((int)floor(fmax3(fjp, fjq, fjr)), 0, nj - 1);
    int k0_int = clamp_int((int)ceil(fmin3(fkp, fkq, fkr)), 0, nk - 1);
    int k1_int = clamp_int((int)floor(fmax3(fkp, fkq, fkr)), 0, nk - 1);

    for (int k = k0_int; k <= k1_int; ++k) {
        for (int j = j0_int; j <= j1_int; ++j) {
            double a, b, c;

            // Test at grid cell corner (j,k) to match CPU implementation
            // This ensures consistent results across all mesh types and densities
            if (point_in_triangle_2d(j, k, fjp, fkp, fjq, fkq, fjr, fkr, a, b, c)) {
                double fi = a * fip + b * fiq + c * fir;
                int i_interval = (int)ceil(fi);

                // Replicate the CPU's logic for handling intersections
                // that occur before the grid starts (i < 0).
                if (i_interval < 0) {
                    int idx = grid_index(0, j, k, ni, nj); // Accumulate at the first cell
                    atomicAdd(&intersection_count[idx], 1);
                } else if (i_interval < ni) {
                    int idx = grid_index(i_interval, j, k, ni, nj);
                    atomicAdd(&intersection_count[idx], 1);
                }
            }
        }
    }
}

// ============================================================================
// Kernel 3: Fast Sweep with Parallel Eikonal Solver
// ============================================================================
// This kernel implements a Jacobi-style parallel update for the Eikonal equation.
// It converges robustly and is significantly faster than propagating triangle indices.
// The Eikonal equation |∇φ| = 1 means distance changes by dx per grid cell.

__global__ void fast_sweep_eikonal_kernel(
    const float* phi_read, float* phi_write,
    float dx, int ni, int nj, int nk)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int k = blockIdx.z * blockDim.z + threadIdx.z;

    if (i >= ni || j >= nj || k >= nk) return;

    int idx = grid_index(i, j, k, ni, nj);
    float current_phi = phi_read[idx];
    float new_phi = current_phi;

    // --- Eikonal Update Step ---
    // Find the minimum neighbor distance in each axis.
    // Initialize with a large value to ensure only actual neighbor values are used.
    float min_x = FLT_MAX;
    if (i > 0)      min_x = fminf(min_x, phi_read[idx - 1]);
    if (i < ni - 1) min_x = fminf(min_x, phi_read[idx + 1]);

    float min_y = FLT_MAX;
    if (j > 0)      min_y = fminf(min_y, phi_read[idx - ni]);
    if (j < nj - 1) min_y = fminf(min_y, phi_read[idx + ni]);

    float min_z = FLT_MAX;
    if (k > 0)      min_z = fminf(min_z, phi_read[idx - ni*nj]);
    if (k < nk - 1) min_z = fminf(min_z, phi_read[idx + ni*nj]);

    // Sort the three minimums so that min_x <= min_y <= min_z
    if (min_x > min_y) { float temp = min_x; min_x = min_y; min_y = temp; }
    if (min_y > min_z) { float temp = min_y; min_y = min_z; min_z = temp; }
    if (min_x > min_y) { float temp = min_x; min_x = min_y; min_y = temp; }

    // Solve for updated distance incrementally in 1D, 2D, then 3D
    float updated_phi;

    // 1D update (from closest neighbor)
    updated_phi = min_x + dx;
    if (updated_phi < new_phi) {
        new_phi = updated_phi;
    }

    // 2D update (from two closest neighbors)
    float discr = 2.0f * dx*dx - (min_y - min_x)*(min_y - min_x);
    if (discr >= 0) {
        updated_phi = (min_x + min_y + sqrtf(discr)) * 0.5f;
        if (updated_phi < new_phi) {
            new_phi = updated_phi;
        }
    }

    // 3D update (from all three neighbors)
    float b = -(min_x + min_y + min_z);
    float c = min_x*min_x + min_y*min_y + min_z*min_z - dx*dx;
    discr = b*b - 3.0f*c;
    if (discr >= 0) {
        updated_phi = (-b + sqrtf(discr)) / 3.0f;
        if (updated_phi < new_phi) {
            new_phi = updated_phi;
        }
    }

    phi_write[idx] = new_phi;
}

// ============================================================================
// Kernel 4: Sign Correction
// ============================================================================

__global__ void sign_correction_kernel(float* phi, const int* intersection_count,
                                      int ni, int nj, int nk) {
    int j = blockIdx.x * blockDim.x + threadIdx.x;
    int k = blockIdx.y * blockDim.y + threadIdx.y;

    if (j >= nj || k >= nk) return;

    int total_count = 0;
    for (int i = 0; i < ni; ++i) {
        int idx = grid_index(i, j, k, ni, nj);
        total_count += intersection_count[idx];

        if (total_count % 2 == 1) {  // Inside mesh
            phi[idx] = -phi[idx];
        }
    }
}

// ============================================================================
// Host Orchestrator
// ============================================================================

void make_level_set3(const std::vector<Vec3ui> &tri, const std::vector<Vec3f> &x,
                     const Vec3f &origin, float dx, int ni, int nj, int nk,
                     Array3f &phi, const int exact_band)
{
    // Get device info
    int device;
    CUDA_CHECK(cudaGetDevice(&device));
    cudaDeviceProp props;
    CUDA_CHECK(cudaGetDeviceProperties(&props, device));
    // std::cout << "GPU: " << props.name << " (Compute " << props.major << "." << props.minor << ")" << std::endl;

    const size_t num_grid_cells = (size_t)ni * nj * nk;
    const size_t num_triangles = tri.size();
    const size_t num_vertices = x.size();

    // std::cout << "Grid: " << ni << "x" << nj << "x" << nk << " = " << num_grid_cells << " cells" << std::endl;
    // std::cout << "Mesh: " << num_vertices << " vertices, " << num_triangles << " triangles" << std::endl;

    // Allocate device memory
    Vec3ui* d_tri;
    Vec3f* d_x;
    DistTriPair* d_dist_tri;
    int* d_intersection_count;
    float* d_phi_read;
    float* d_phi_write;
    int* d_closest_tri_read;
    int* d_closest_tri_write;

    CUDA_CHECK(cudaMalloc(&d_tri, num_triangles * sizeof(Vec3ui)));
    CUDA_CHECK(cudaMalloc(&d_x, num_vertices * sizeof(Vec3f)));
    CUDA_CHECK(cudaMalloc(&d_dist_tri, num_grid_cells * sizeof(DistTriPair)));
    CUDA_CHECK(cudaMalloc(&d_intersection_count, num_grid_cells * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_phi_read, num_grid_cells * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_phi_write, num_grid_cells * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_closest_tri_read, num_grid_cells * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_closest_tri_write, num_grid_cells * sizeof(int)));

    // Host to device copy
    CUDA_CHECK(cudaMemcpy(d_tri, tri.data(), num_triangles * sizeof(Vec3ui), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_x, x.data(), num_vertices * sizeof(Vec3f), cudaMemcpyHostToDevice));

    // Kernel 1: Initialize
    dim3 blockInit(8, 8, 8);
    dim3 gridInit((ni + 7) / 8, (nj + 7) / 8, (nk + 7) / 8);
    initialize_grids_kernel<<<gridInit, blockInit>>>(d_dist_tri, d_intersection_count, ni, nj, nk, (ni+nj+nk)*dx);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // Kernel 2: Near-band distances
    int blockNear = 256;
    int gridNear = (num_triangles + 255) / 256;
    near_band_distance_kernel<<<gridNear, blockNear>>>(d_tri, d_x, d_dist_tri, d_intersection_count,
                                                       num_triangles, origin, dx, ni, nj, nk, exact_band);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // Extract phi and triangle indices from DistTriPair
    CUDA_CHECK(cudaMemcpy2D(d_phi_read, sizeof(float), d_dist_tri, sizeof(DistTriPair),
                           sizeof(float), num_grid_cells, cudaMemcpyDeviceToDevice));
    CUDA_CHECK(cudaMemcpy2D(d_closest_tri_read, sizeof(int),
                           (char*)d_dist_tri + offsetof(DistTriPair, tri_idx), sizeof(DistTriPair),
                           sizeof(int), num_grid_cells, cudaMemcpyDeviceToDevice));

    // DEBUG: Check near-band distances
    std::vector<float> debug_phi(num_grid_cells);
    CUDA_CHECK(cudaMemcpy(debug_phi.data(), d_phi_read, num_grid_cells * sizeof(float), cudaMemcpyDeviceToHost));
    float min_near = *std::min_element(debug_phi.begin(), debug_phi.end());
    float max_near = *std::max_element(debug_phi.begin(), debug_phi.end());
    // Debug output commented out for production use
    // float init_max = (ni+nj+nk)*dx;
    // int unchanged = 0, updated = 0;
    // for (float v : debug_phi) {
    //     if (fabsf(v - init_max) < 1e-6f) unchanged++;
    //     else updated++;
    // }
    // std::cout << "After near-band: min=" << min_near << ", max=" << max_near << std::endl;
    // std::cout << "  Cells: " << updated << " updated by near-band, " << unchanged << " still at init value" << std::endl;

    // Check a few specific sample values right after near-band
    // std::cout << "  Sample near-band values: ";
    // int sample_indices[] = {0, 100, 1000, (int)(num_grid_cells/2)};
    // for (int i = 0; i < 4; i++) {
    //     int idx = sample_indices[i];
    //     if (idx < num_grid_cells) std::cout << "[" << idx << "]=" << debug_phi[idx] << " ";
    // }
    // std::cout << std::endl;

    // Kernel 3: Fast sweeping
    dim3 blockSweep(8, 8, 8);
    dim3 gridSweep = gridInit;

    // Jacobi iterations need more passes than Gauss-Seidel sweeps for same convergence
    // CPU uses 2 passes × 8 directional Gauss-Seidel sweeps = 16 effective sweeps
    // GPU Jacobi method: Match CPU's effective sweep count but iterate more
    // to allow information to propagate across the entire gridf
    const int sweep_iterations = std::max(ni, std::max(nj, nk)) * 2;

    // std::cout << "Fast sweeping: " << sweep_iterations << " Jacobi iterations" << std::endl;

    for (int iter = 0; iter < sweep_iterations; ++iter) {
        fast_sweep_eikonal_kernel<<<gridSweep, blockSweep>>>(
            d_phi_read, d_phi_write, dx, ni, nj, nk);
        CUDA_CHECK(cudaGetLastError());
        std::swap(d_phi_read, d_phi_write);

        // DEBUG: Check convergence periodically (commented out for production)
        // if (iter % (sweep_iterations / 4) == (sweep_iterations / 4 - 1) || iter == sweep_iterations - 1) {
        //     CUDA_CHECK(cudaMemcpy(debug_phi.data(), d_phi_read, num_grid_cells * sizeof(float), cudaMemcpyDeviceToHost));
        //     float min_val = *std::min_element(debug_phi.begin(), debug_phi.end());
        //     float max_val = *std::max_element(debug_phi.begin(), debug_phi.end());
        //     float avg_val = 0.0f;
        //     for (float v : debug_phi) avg_val += v;
        //     avg_val /= num_grid_cells;
        //     std::cout << "  Iteration " << (iter+1) << ": min=" << min_val << ", max=" << max_val << ", avg=" << avg_val << std::endl;
        // }
    }
    CUDA_CHECK(cudaDeviceSynchronize());

    // DEBUG: Sample values at specific points for comparison (commented out for production)
    // std::cout << "Sample distances before sign correction:" << std::endl;
    // int final_samples[] = {0, 100, 1000, 5000, 10000};
    // for (int s = 0; s < 5; s++) {
    //     int sample_idx = final_samples[s];
    //     if (sample_idx < num_grid_cells) {
    //         int k = sample_idx / (ni * nj);
    //         int j = (sample_idx - k * ni * nj) / ni;
    //         int i = sample_idx - k * ni * nj - j * ni;
    //         std::cout << "  [" << i << "," << j << "," << k << "]: " << debug_phi[sample_idx] << std::endl;
    //     }
    // }

    // DEBUG: Check intersection counts before sign correction (commented out for production)
    // std::vector<int> debug_intersections(num_grid_cells);
    // CUDA_CHECK(cudaMemcpy(debug_intersections.data(), d_intersection_count, num_grid_cells * sizeof(int), cudaMemcpyDeviceToHost));
    // int total_intersections = 0;
    // for (int v : debug_intersections) total_intersections += v;
    // std::cout << "Total intersections: " << total_intersections << std::endl;
    // std::cout << "Sample intersection counts at y=3,z=3: ";
    // for (int i = 0; i < 15 && i < ni; i++) {
    //     int idx = grid_index(i, 3, 3, ni, nj);
    //     std::cout << "[" << i << "]=" << debug_intersections[idx] << " ";
    // }
    // std::cout << std::endl;

    // Kernel 4: Sign correction
    dim3 blockSign(16, 16);
    dim3 gridSign((nj + 15) / 16, (nk + 15) / 16);
    sign_correction_kernel<<<gridSign, blockSign>>>(d_phi_read, d_intersection_count, ni, nj, nk);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // Device to host copy
    phi.resize(ni, nj, nk);
    float* phi_data = &phi.a[0];
    CUDA_CHECK(cudaMemcpy(phi_data, d_phi_read, num_grid_cells * sizeof(float), cudaMemcpyDeviceToHost));

    // DEBUG: Verify data was copied correctly (commented out for production)
    // std::cout << "GPU->CPU copy verification:" << std::endl;
    // std::cout << "  Sample values after copy: ";
    // for (int s = 0; s < 5 && s < num_grid_cells; s++) {
    //     std::cout << "[" << s << "]=" << phi_data[s] << " ";
    // }
    // std::cout << std::endl;

    // Verify using Array3f accessor
    // std::cout << "  Via Array3f accessor: ";
    // for (int i = 0; i < 5 && i < ni; i++) {
    //     std::cout << "[" << i << ",0,0]=" << phi(i,0,0) << " ";
    // }
    // std::cout << std::endl;

    // Cleanup
    CUDA_CHECK(cudaFree(d_tri));
    CUDA_CHECK(cudaFree(d_x));
    CUDA_CHECK(cudaFree(d_dist_tri));
    CUDA_CHECK(cudaFree(d_intersection_count));
    CUDA_CHECK(cudaFree(d_phi_read));
    CUDA_CHECK(cudaFree(d_phi_write));
    CUDA_CHECK(cudaFree(d_closest_tri_read));
    CUDA_CHECK(cudaFree(d_closest_tri_write));

    // std::cout << "GPU SDF computation complete." << std::endl;
}

} // namespace gpu
} // namespace sdfgen
