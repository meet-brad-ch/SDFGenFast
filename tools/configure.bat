@echo off
setlocal

REM ============================================================================
REM Generic CMake Configuration Script for Windows
REM ============================================================================
REM Configures a CMake project using Visual Studio toolchain and Ninja generator
REM Works with any CMake-based project - just place in a 'tools' subdirectory
REM
REM Usage:
REM   configure.bat [build_type] [generator]
REM
REM Arguments:
REM   build_type - Debug or Release (default: Release)
REM   generator  - "Ninja" or "Visual Studio" (default: Ninja)
REM
REM Example:
REM   configure.bat Release
REM   configure.bat Debug Ninja
REM   configure.bat Release "Visual Studio"
REM ============================================================================

REM Setup VS environment first
call "%~dp0setup_vs_env.bat"
if errorlevel 1 (
    echo.
    echo [configure] ERROR: Cannot continue without Visual Studio
    echo [configure] Configuration aborted.
    echo.
    exit /b 1
)

REM Parse arguments
set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

set GENERATOR=%~2
if "%GENERATOR%"=="" set GENERATOR=Ninja

REM Define paths - assumes this script is in 'tools' subdirectory
set PROJECT_ROOT=%~dp0..
set BUILD_DIR=%PROJECT_ROOT%\build-%BUILD_TYPE%

echo.
echo [configure] ============================================
echo [configure] CMake Configuration
echo [configure] ============================================
echo [configure] Build Type:  %BUILD_TYPE%
echo [configure] Generator:   %GENERATOR%
echo [configure] Project:     %PROJECT_ROOT%
echo [configure] Build Dir:   %BUILD_DIR%
echo [configure] ============================================
echo.

REM Verify CMakeLists.txt exists
if not exist "%PROJECT_ROOT%\CMakeLists.txt" (
    echo [configure] ERROR: CMakeLists.txt not found in %PROJECT_ROOT%
    echo [configure] Make sure this script is in a 'tools' subdirectory of your CMake project
    exit /b 1
)

REM Create and enter build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM Run CMake configuration
echo [configure] Running CMake...
echo.

if /i "%GENERATOR%"=="Ninja" (
    cmake -G Ninja -DCMAKE_BUILD_TYPE=%BUILD_TYPE% "%PROJECT_ROOT%"
) else if /i "%GENERATOR%"=="Visual Studio" (
    cmake -G "%VS_GENERATOR%" -A x64 "%PROJECT_ROOT%"
) else (
    echo [configure] ERROR: Unknown generator "%GENERATOR%"
    echo [configure] Valid options: Ninja, "Visual Studio"
    exit /b 1
)

if errorlevel 1 (
    echo.
    echo [configure] ERROR: CMake configuration failed
    exit /b 1
)

echo.
echo [configure] ============================================
echo [configure] Configuration complete!
echo [configure] Build directory: %BUILD_DIR%
echo [configure] ============================================
echo.

exit /b 0
