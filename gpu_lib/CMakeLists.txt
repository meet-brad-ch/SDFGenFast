cmake_minimum_required(VERSION 3.20)

# Enable CUDA language
enable_language(CUDA)

# GPU implementation library using CUDA
add_library(sdfgen_gpu STATIC
    makelevelset3_gpu.cu
)

# CUDA-specific settings
# Target compute capabilities: 80 (Ampere), 89 (Ada/RTX 4090)
set_target_properties(sdfgen_gpu PROPERTIES
    CUDA_STANDARD 11
    CUDA_STANDARD_REQUIRED ON
    CUDA_SEPARABLE_COMPILATION OFF
    CUDA_ARCHITECTURES "80;89"
    POSITION_INDEPENDENT_CODE ON
)

# CRITICAL: Disable Fused Multiply-Add (FMA) instructions
# This ensures bit-for-bit numerical parity with the CPU by preventing the GPU
# from optimizing floating-point operations (like y1*x2 - x1*y2) differently
# than the CPU. Without this, the orientation() function's tie-breaking produces
# subtly different signs for near-zero values, causing point_in_triangle_2d to
# classify edge points inconsistently between CPU and GPU, leading to sign errors.
target_compile_options(sdfgen_gpu PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--fmad=false>)

# Make headers available
target_include_directories(sdfgen_gpu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common  # For vec.h, array3.h
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # For accessing cpu_lib headers
)

# Link CUDA runtime
target_link_libraries(sdfgen_gpu PUBLIC CUDA::cudart)
