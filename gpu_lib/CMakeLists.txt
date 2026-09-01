# GPU implementation library using CUDA
enable_language(CUDA)

add_library(sdfgen_gpu STATIC
    makelevelset3_gpu.cu
)

target_include_directories(sdfgen_gpu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sdfgen_gpu
    PUBLIC
        sdfgen_headers
        CUDA::cudart
    PRIVATE
        sdfgen_options
)

# Architectures come from the SDFGEN_CUDA_ARCHITECTURES cache variable
# (default all-major) so CI runners and user GPUs are not locked to the
# author's hardware.
set_target_properties(sdfgen_gpu PROPERTIES
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED ON
    CUDA_SEPARABLE_COMPILATION OFF
    CUDA_ARCHITECTURES "${SDFGEN_CUDA_ARCHITECTURES}"
    POSITION_INDEPENDENT_CODE ON
)

# CRITICAL: Disable Fused Multiply-Add (FMA) instructions
# This ensures bit-for-bit numerical parity with the CPU by preventing the GPU
# from optimizing floating-point operations (like y1*x2 - x1*y2) differently
# than the CPU. Without this, the orientation() function's tie-breaking produces
# subtly different signs for near-zero values, causing point_in_triangle_2d to
# classify edge points inconsistently between CPU and GPU, leading to sign errors.
target_compile_options(sdfgen_gpu PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--fmad=false>)
