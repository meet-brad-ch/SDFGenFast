@echo off
setlocal

REM ============================================================================
REM Generic CMake Build Script for Windows
REM ============================================================================
REM Builds specified CMake target using Visual Studio toolchain
REM Works with any CMake-based project - just place in a 'tools' subdirectory
REM
REM Usage:
REM   build_with_vs.bat <target> [build_type] [jobs]
REM
REM Arguments:
REM   target     - CMake target name (required, or "all" for everything)
REM   build_type - Debug or Release (default: Release)
REM   jobs       - Number of parallel jobs (default: auto-detected)
REM
REM Examples:
REM   build_with_vs.bat all              # Build all targets (Release)
REM   build_with_vs.bat gflip3d          # Build specific target
REM   build_with_vs.bat clean            # Clean build artifacts
REM   build_with_vs.bat gflip3d Debug    # Build in Debug mode
REM   build_with_vs.bat all Release 8    # Build with 8 parallel jobs
REM ============================================================================

REM Check for target argument
if "%1"=="" (
    echo.
    echo [build_with_vs] ERROR: No target specified
    echo.
    echo Usage: build_with_vs.bat ^<target^> [build_type] [jobs]
    echo.
    echo Examples:
    echo   build_with_vs.bat all           # Build everything
    echo   build_with_vs.bat gflip3d       # Build specific target
    echo   build_with_vs.bat clean         # Clean artifacts
    echo.
    exit /b 1
)

set TARGET=%1
set BUILD_TYPE=%2
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

set JOBS=%3
if "%JOBS%"=="" set JOBS=%NUMBER_OF_PROCESSORS%

REM Setup VS environment
call "%~dp0setup_vs_env.bat"
if errorlevel 1 (
    echo.
    echo [build_with_vs] ERROR: Cannot continue without Visual Studio
    echo [build_with_vs] Build aborted.
    echo.
    exit /b 1
)

REM Define paths - assumes this script is in 'tools' subdirectory
set PROJECT_ROOT=%~dp0..
set BUILD_DIR=%PROJECT_ROOT%\build-%BUILD_TYPE%

REM Check if build directory exists
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo.
    echo [build_with_vs] ============================================
    echo [build_with_vs] ERROR: Build directory not configured
    echo [build_with_vs] ============================================
    echo [build_with_vs] Build directory: %BUILD_DIR%
    echo.
    echo [build_with_vs] Please run configure_cmake.bat first:
    echo [build_with_vs]   cd %~dp0
    echo [build_with_vs]   configure_cmake.bat %BUILD_TYPE%
    echo.
    exit /b 1
)

echo.
echo [build_with_vs] ============================================
echo [build_with_vs] Build Configuration
echo [build_with_vs] ============================================
echo [build_with_vs] Target:     %TARGET%
echo [build_with_vs] Build Type: %BUILD_TYPE%
echo [build_with_vs] Build Dir:  %BUILD_DIR%
echo [build_with_vs] Jobs:       %JOBS%
echo [build_with_vs] ============================================
echo.

REM Navigate to build directory
cd /d "%BUILD_DIR%"

REM Build the target
echo [build_with_vs] Building...
echo.

cmake --build . --target %TARGET% -j %JOBS%

if errorlevel 1 (
    echo.
    echo [build_with_vs] ============================================
    echo [build_with_vs] ERROR: Build failed for target '%TARGET%'
    echo [build_with_vs] ============================================
    exit /b 1
)

echo.
echo [build_with_vs] ============================================
echo [build_with_vs] SUCCESS: Build completed for '%TARGET%'
echo [build_with_vs] ============================================

exit /b 0
