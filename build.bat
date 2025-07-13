@echo off
setlocal enabledelayedexpansion

REM OneSDK Windows Build Script
REM This script builds the OneSDK project on Windows

echo ========================================
echo OneSDK Windows Build Script
echo ========================================

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