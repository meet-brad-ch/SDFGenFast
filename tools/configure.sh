#!/bin/bash
# SDFGen - CMake Configuration Script with Auto CUDA Detection
# Cross-platform configuration for Ubuntu LTS and other Linux distributions
#
# Usage:
#   ./configure.sh [Release|Debug]
#
# This script automatically:
#   - Detects CUDA Toolkit availability
#   - Configures build type (Release or Debug)
#   - Sets up Ninja build system if available (falls back to Make)
#   - Creates build directory with proper naming

set -e  # Exit on error

# ============================================================================
# Configuration
# ============================================================================

# Get script directory (works even when sourced or executed via symlink)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Default build type
BUILD_TYPE="${1:-Release}"

# Validate build type
if [[ "$BUILD_TYPE" != "Release" && "$BUILD_TYPE" != "Debug" ]]; then
    echo "Error: Build type must be 'Release' or 'Debug'"
    echo "Usage: $0 [Release|Debug]"
    exit 1
fi

# Build directory naming
BUILD_DIR="$PROJECT_ROOT/build-$BUILD_TYPE"

# ============================================================================
# System Detection
# ============================================================================

echo "========================================"
echo "SDFGen - Configuration Script"
echo "========================================"
echo ""
echo "System Information:"
echo "  OS: $(uname -s)"
echo "  Architecture: $(uname -m)"
echo "  Build Type: $BUILD_TYPE"
echo ""

# ============================================================================
# CMake Detection
# ============================================================================

if ! command -v cmake &> /dev/null; then
    echo "Error: CMake not found"
    echo ""
    echo "On Ubuntu/Debian:"
    echo "  sudo apt-get install cmake"
    echo ""
    echo "Required: CMake 3.20 or higher"
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
echo "CMake: $CMAKE_VERSION"

# Check minimum CMake version (3.20)
CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d'.' -f1)
CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d'.' -f2)

if [ "$CMAKE_MAJOR" -lt 3 ] || { [ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -lt 20 ]; }; then
    echo "Error: CMake 3.20 or higher required (found $CMAKE_VERSION)"
    exit 1
fi

# ============================================================================
# Build System Detection (Ninja vs Make)
# ============================================================================

if command -v ninja &> /dev/null; then
    GENERATOR="Ninja"
    echo "Build System: Ninja (fast parallel builds)"
else
    GENERATOR="Unix Makefiles"
    echo "Build System: Make (consider installing ninja for faster builds)"
    echo "  sudo apt-get install ninja-build"
fi

echo ""

# ============================================================================
# CUDA Detection (Automatic)
# ============================================================================

echo "CUDA Detection:"

CUDA_AVAILABLE=0

# Method 1: Check for nvcc compiler
if command -v nvcc &> /dev/null; then
    CUDA_VERSION=$(nvcc --version | grep "release" | sed -n 's/.*release \([0-9.]*\).*/\1/p')
    echo "  ✓ CUDA Toolkit found: $CUDA_VERSION"
    echo "  ✓ nvcc: $(which nvcc)"
    CUDA_AVAILABLE=1
else
    echo "  ✗ nvcc not found in PATH"
fi

# Method 2: Check common CUDA installation paths
if [ $CUDA_AVAILABLE -eq 0 ]; then
    CUDA_PATHS=(
        "/usr/local/cuda"
        "/usr/local/cuda-12"
        "/usr/local/cuda-11"
        "/opt/cuda"
    )

    for cuda_path in "${CUDA_PATHS[@]}"; do
        if [ -d "$cuda_path" ] && [ -f "$cuda_path/bin/nvcc" ]; then
            echo "  ✓ CUDA found: $cuda_path"
            CUDA_AVAILABLE=1
            break
        fi
    done

    if [ $CUDA_AVAILABLE -eq 0 ]; then
        echo "  ✗ CUDA not found in common paths"
    fi
fi

# Method 3: Check for CUDA runtime library
if [ $CUDA_AVAILABLE -eq 0 ]; then
    if ldconfig -p | grep -q libcudart.so; then
        echo "  ✓ libcudart.so found in library path"
        CUDA_AVAILABLE=1
    else
        echo "  ✗ libcudart.so not found"
    fi
fi

echo ""

if [ $CUDA_AVAILABLE -eq 1 ]; then
    echo "Build Configuration: CPU + GPU (CUDA acceleration enabled)"
    echo "  GPU support will be compiled and used automatically at runtime"
else
    echo "Build Configuration: CPU-only"
    echo "  Install CUDA Toolkit to enable GPU acceleration:"
    echo "    https://developer.nvidia.com/cuda-downloads"
fi

echo ""

# ============================================================================
# VTK Detection (Optional)
# ============================================================================

echo "Optional Dependencies:"

if command -v vtk &> /dev/null || pkg-config --exists vtk 2>/dev/null; then
    echo "  ✓ VTK found (enables .vti output format)"
else
    echo "  ✗ VTK not found (only binary .sdf output will be available)"
    echo "    sudo apt-get install libvtk9-dev"
fi

echo ""

# ============================================================================
# Create Build Directory
# ============================================================================

echo "========================================"
echo "Creating Build Directory"
echo "========================================"
echo ""

if [ -d "$BUILD_DIR" ]; then
    echo "Build directory exists: $BUILD_DIR"
    read -p "Remove and reconfigure? [y/N] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Removing existing build directory..."
        rm -rf "$BUILD_DIR"
    else
        echo "Keeping existing configuration"
        exit 0
    fi
fi

mkdir -p "$BUILD_DIR"
echo "Created: $BUILD_DIR"
echo ""

# ============================================================================
# Run CMake Configuration
# ============================================================================

echo "========================================"
echo "Running CMake Configuration"
echo "========================================"
echo ""

cd "$BUILD_DIR"

cmake \
    -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    "$PROJECT_ROOT"

# ============================================================================
# Configuration Summary
# ============================================================================

echo ""
echo "========================================"
echo "Configuration Complete"
echo "========================================"
echo ""
echo "Build directory: $BUILD_DIR"
echo "Build type: $BUILD_TYPE"
echo "Generator: $GENERATOR"
echo ""
