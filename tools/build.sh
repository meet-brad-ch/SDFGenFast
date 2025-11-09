#!/bin/bash
# SDFGen - Build Script for Linux
# Wrapper around CMake build system
#
# Usage:
#   ./build.sh [target] [Release|Debug]
#
# Examples:
#   ./build.sh                    # Build all targets (Release)
#   ./build.sh SDFGen             # Build main application only
#   ./build.sh test_cli_backend   # Build specific test
#   ./build.sh all Debug          # Build everything in Debug mode

set -e  # Exit on error

# ============================================================================
# Configuration
# ============================================================================

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Parse arguments
TARGET="${1:-all}"
BUILD_TYPE="${2:-Release}"

# Validate build type
if [[ "$BUILD_TYPE" != "Release" && "$BUILD_TYPE" != "Debug" ]]; then
    echo "Error: Build type must be 'Release' or 'Debug'"
    echo "Usage: $0 [target] [Release|Debug]"
    exit 1
fi

BUILD_DIR="$PROJECT_ROOT/build-$BUILD_TYPE"

# ============================================================================
# Pre-flight Checks
# ============================================================================

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory not found: $BUILD_DIR"
    echo ""
    echo "Please run configuration first:"
    echo "  cd tools && ./configure.sh $BUILD_TYPE"
    exit 1
fi

# Check if CMakeCache.txt exists
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "Error: CMake configuration not found in $BUILD_DIR"
    echo ""
    echo "Please run configuration first:"
    echo "  cd tools && ./configure.sh $BUILD_TYPE"
    exit 1
fi

# ============================================================================
# Detect Build System and CPU Count
# ============================================================================

# Detect generator from CMakeCache.txt
GENERATOR=$(grep "CMAKE_GENERATOR:" "$BUILD_DIR/CMakeCache.txt" | cut -d'=' -f2)

# Detect CPU count for parallel builds
if command -v nproc &> /dev/null; then
    NPROC=$(nproc)
elif [ -f /proc/cpuinfo ]; then
    NPROC=$(grep -c ^processor /proc/cpuinfo)
else
    NPROC=4  # Default fallback
fi

# ============================================================================
# Build Information
# ============================================================================

echo ""
echo "============================================"
echo "Build Configuration"
echo "============================================"
echo "Target:     $TARGET"
echo "Build Type: $BUILD_TYPE"
echo "Build Dir:  $BUILD_DIR"
echo "Generator:  $GENERATOR"
echo "Jobs:       $NPROC"
echo "============================================"
echo ""

# ============================================================================
# Build
# ============================================================================

echo "Building..."
echo ""

cd "$BUILD_DIR"

# Use cmake --build for cross-platform compatibility
cmake --build . --target "$TARGET" --config "$BUILD_TYPE" -j "$NPROC"

BUILD_EXIT_CODE=$?

echo ""

# ============================================================================
# Build Result
# ============================================================================

if [ $BUILD_EXIT_CODE -eq 0 ]; then
    echo "============================================"
    echo "SUCCESS: Build completed for '$TARGET'"
    echo "============================================"
    echo ""
    exit 0
else
    echo "============================================"
    echo "FAILED: Build failed for '$TARGET'"
    echo "============================================"
    echo ""
    echo "Build directory: $BUILD_DIR"
    echo "Exit code: $BUILD_EXIT_CODE"
    echo ""
    exit $BUILD_EXIT_CODE
fi
