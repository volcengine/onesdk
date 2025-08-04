@echo off
setlocal enabledelayedexpansion

REM OneSDK Windows Build Script
REM This script builds the OneSDK project on Windows

echo ========================================
echo OneSDK Windows Build Script
echo ========================================

REM Check CMake version
echo Checking CMake version...
cmake --version >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: CMake is not installed or not in PATH!
    exit /b 1
)

REM Get CMake version - extract from the first line
for /f "tokens=3 delims= " %%a in ('cmake --version 2^>nul') do (
    set CMAKE_VERSION=%%a
    goto :version_extracted
)
:version_extracted

echo Detected CMake version: %CMAKE_VERSION%

REM Extract major and minor version numbers
for /f "tokens=1,2 delims=." %%a in ("%CMAKE_VERSION%") do (
    set CMAKE_MAJOR=%%a
    set CMAKE_MINOR=%%b
)





REM Check version compatibility
if %CMAKE_MAJOR%==3 (
    if %CMAKE_MINOR% geq 10 (
        echo CMake version is compatible (3.x)
    ) else (
        echo Warning: CMake version %CMAKE_VERSION% is below 3.10
        echo Minimum recommended: 3.10
        echo Continuing with build...
    )
) else if %CMAKE_MAJOR% geq 4 (
    echo Warning: CMake version %CMAKE_VERSION% is 4.0 or above
    echo This may cause compatibility issues with cJSON
    echo Recommended: Use CMake 3.x for best compatibility
    echo Continuing with build...
) else (
    echo Warning: CMake version %CMAKE_VERSION% is below 3.10
    echo Minimum recommended: 3.10
    echo Continuing with build...
)

REM Check if clean-build parameter is provided
set CLEAN_BUILD=0
if "%1"=="clean-build" (
    set CLEAN_BUILD=1
    shift
)

REM Clean build directory if requested
if %CLEAN_BUILD%==1 (
    echo Cleaning build directory...
    if exist build rmdir /s /q build
)

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake %* ..

REM Check if CMake configuration was successful
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed!
    cd ..
    exit /b 1
)

REM Build the project
echo Building the project...
cmake --build . --config Release

REM Check if build was successful
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed!
    cd ..
    exit /b 1
)

echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Build artifacts are located in:
echo   - Libraries: build\lib\
echo   - Executables: build\bin\
echo   - Headers: build\include\
echo.

cd .. 