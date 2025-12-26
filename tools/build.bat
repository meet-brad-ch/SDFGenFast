@echo off
setlocal

REM ============================================================================
REM Generic CMake Build Script for Windows
REM ============================================================================
REM Builds specified CMake target using Visual Studio toolchain
REM Works with any CMake-based project - just place in a 'tools' subdirectory
REM
REM Usage:
REM   build.bat <target> [build_type] [jobs]
REM
REM Arguments:
REM   target     - CMake target name (required, or "all" for everything)
REM   build_type - Debug or Release (default: Release)
REM   jobs       - Number of parallel jobs (default: auto-detected)
REM
REM Examples:
REM   build.bat all              # Build all targets (Release)
REM   build.bat gflip3d          # Build specific target
REM   build.bat clean            # Clean build artifacts
REM   build.bat gflip3d Debug    # Build in Debug mode
REM   build.bat all Release 8    # Build with 8 parallel jobs
REM ============================================================================

REM Check for target argument
if "%1"=="" (
    echo.
    echo [build] ERROR: No target specified
    echo.
    echo Usage: build.bat ^<target^> [build_type] [jobs]
    echo.
    echo Examples:
    echo   build.bat all           # Build everything
    echo   build.bat gflip3d       # Build specific target
    echo   build.bat clean         # Clean artifacts
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
    echo [build] ERROR: Cannot continue without Visual Studio
    echo [build] Build aborted.
    echo.
    exit /b 1
)

REM Define paths - assumes this script is in 'tools' subdirectory
set PROJECT_ROOT=%~dp0..
set BUILD_DIR=%PROJECT_ROOT%\build-%BUILD_TYPE%

REM Check if build directory exists
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo.
    echo [build] ============================================
    echo [build] ERROR: Build directory not configured
    echo [build] ============================================
    echo [build] Build directory: %BUILD_DIR%
    echo.
    echo [build] Please run configure.bat first:
    echo [build]   cd %~dp0
    echo [build]   configure.bat %BUILD_TYPE%
    echo.
    exit /b 1
)

echo.
echo [build] ============================================
echo [build] Build Configuration
echo [build] ============================================
echo [build] Target:     %TARGET%
echo [build] Build Type: %BUILD_TYPE%
echo [build] Build Dir:  %BUILD_DIR%
echo [build] Jobs:       %JOBS%
echo [build] ============================================
echo.

REM Navigate to build directory
cd /d "%BUILD_DIR%"

REM Build the target
echo [build] Building...
echo.

cmake --build . --target %TARGET% -j %JOBS%

if errorlevel 1 (
    echo.
    echo [build] ============================================
    echo [build] ERROR: Build failed for target '%TARGET%'
    echo [build] ============================================
    exit /b 1
)

echo.
echo [build] ============================================
echo [build] SUCCESS: Build completed for '%TARGET%'
echo [build] ============================================

exit /b 0
