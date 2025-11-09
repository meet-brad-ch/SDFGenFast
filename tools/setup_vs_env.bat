@echo off
REM ============================================================================
REM Generic Visual Studio Environment Setup for Windows
REM ============================================================================
REM Auto-detects and initializes the MSVC compiler environment
REM Supports Visual Studio 2022 (Community, Professional, Enterprise)
REM Works with any CMake-based project - just place in a 'tools' subdirectory
REM
REM Usage:
REM   Call this from another batch script:
REM     call setup_vs_env.bat
REM
REM   Or run it to get a shell with VS environment:
REM     setup_vs_env.bat
REM     cmd /k
REM ============================================================================

echo [setup_vs_env] Detecting Visual Studio installation...

REM Try Visual Studio 2022 (Community, Professional, Enterprise)
set VS_PATH=
for %%e in (Community Professional Enterprise) do (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\%%e\VC\Auxiliary\Build\vcvarsall.bat" (
        set VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\%%e\VC\Auxiliary\Build\vcvarsall.bat
        set VS_VERSION=2022
        set VS_EDITION=%%e
        set VS_GENERATOR=Visual Studio 17 2022
        goto :found
    )
)

REM Not found
echo.
echo ============================================
echo [setup_vs_env] ERROR: Visual Studio Not Found
echo ============================================
echo.
echo Visual Studio 2022 is required to build this project.
echo.
echo Installation Instructions:
echo   1. Download Visual Studio 2022 Community (FREE):
echo      https://visualstudio.microsoft.com/downloads/
echo.
echo   2. During installation, select:
echo      [X] Desktop development with C++
echo.
echo   3. After installation, run this script again
echo.
echo Supported editions: Community, Professional, Enterprise
echo Required version: 2022
echo.
echo Searched location:
echo   - C:\Program Files\Microsoft Visual Studio\2022\
echo.
echo ============================================
echo.
exit /b 1

:found
echo [setup_vs_env] Found: Visual Studio %VS_VERSION% %VS_EDITION%
echo [setup_vs_env] Initializing x64 environment...

call "%VS_PATH%" x64

if errorlevel 1 (
    echo [setup_vs_env] ERROR: Failed to initialize Visual Studio environment
    exit /b 1
)

echo [setup_vs_env] Environment ready!
echo [setup_vs_env] Version:   Visual Studio %VS_VERSION% %VS_EDITION%
echo [setup_vs_env] Compiler:  %VSCMD_ARG_TGT_ARCH%
echo [setup_vs_env] Toolset:   %VCToolsVersion%
echo [setup_vs_env] Generator: %VS_GENERATOR%
echo.

exit /b 0
