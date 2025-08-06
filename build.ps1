# OneSDK Windows PowerShell Build Script
# This script builds the OneSDK project on Windows using PowerShell

param(
    [switch]$CleanBuild,
    [string]$BuildType = "Release",
    [string]$Generator = "",
    [int]$Parallel = 0,
    [string[]]$CMakeOptions = @()
)

Write-Host "========================================" -ForegroundColor Green
Write-Host "OneSDK Windows PowerShell Build Script" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

# 确保输出不被缓冲
$OutputEncoding = [System.Text.Encoding]::UTF8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# Check if running as administrator (optional)
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
if ($isAdmin) {
    Write-Host "Running as Administrator" -ForegroundColor Yellow
}

# Check prerequisites
Write-Host "Checking prerequisites..." -ForegroundColor Cyan

# Check CMake
try {
    $cmakeVersion = cmake --version 2>$null | Select-String "cmake version" | ForEach-Object { $_.ToString().Split()[2] }
    if ($cmakeVersion) {
        Write-Host "CMake version: $cmakeVersion" -ForegroundColor Green
        
        # Check CMake version compatibility
        $versionParts = $cmakeVersion.Split('.')
        $majorVersion = [int]$versionParts[0]
        $minorVersion = [int]$versionParts[1]
        
        if ($majorVersion -eq 3 -and $minorVersion -ge 10) {
            Write-Host "CMake version is compatible (3.x)" -ForegroundColor Green
        } elseif ($majorVersion -eq 4) {
            Write-Host "Warning: CMake version $cmakeVersion is 4.x" -ForegroundColor Yellow
            Write-Host "This may cause compatibility issues with cJSON" -ForegroundColor Yellow
            Write-Host "Consider using CMake 3.26.x for best compatibility" -ForegroundColor Yellow
            
            $response = Read-Host "Continue anyway? (y/N)"
            if ($response -notmatch "^[Yy]$") {
                exit 1
            }
        } elseif ($majorVersion -gt 4) {
            Write-Host "Error: CMake version $cmakeVersion is too new. Maximum supported: 4.x" -ForegroundColor Red
            Write-Host "Please use CMake 3.10-4.x for compatibility" -ForegroundColor Red
            exit 1
        } else {
            Write-Host "Error: CMake version $cmakeVersion is too old. Minimum required: 3.10" -ForegroundColor Red
            exit 1
        }
    } else {
        Write-Host "CMake not found or version could not be determined" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "CMake not found. Please install CMake and add it to PATH." -ForegroundColor Red
    exit 1
}

# Check Git
try {
    $gitVersion = git --version 2>$null
    if ($gitVersion) {
        Write-Host "Git found: $gitVersion" -ForegroundColor Green
    } else {
        Write-Host "Git not found" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "Git not found. Please install Git and add it to PATH." -ForegroundColor Red
    exit 1
}

# Check Visual Studio or MinGW
$vsFound = $false
$mingwFound = $false

# Check for Visual Studio
try {
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsInstall = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 2>$null
        if ($vsInstall) {
            Write-Host "Visual Studio found" -ForegroundColor Green
            $vsFound = $true
            if (-not $Generator) {
                $Generator = "Visual Studio 17 2022"
            }
        }
    }
} catch {
    Write-Host "Visual Studio not found" -ForegroundColor Yellow
}

# Check for MinGW
try {
    $gccVersion = gcc --version 2>$null | Select-String "gcc" | Select-Object -First 1
    if ($gccVersion) {
        Write-Host "MinGW/GCC found: $gccVersion" -ForegroundColor Green
        $mingwFound = $true
        if (-not $Generator) {
            $Generator = "MinGW Makefiles"
        }
    }
} catch {
    Write-Host "MinGW/GCC not found" -ForegroundColor Yellow
}

if (-not $vsFound -and -not $mingwFound) {
    Write-Host "No C++ compiler found. Please install Visual Studio or MinGW-w64." -ForegroundColor Red
    exit 1
}

# Clean build directory if requested
if ($CleanBuild) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path "build") {
        Remove-Item -Recurse -Force "build"
    }
}

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Change to build directory
Push-Location "build"

try {
    # Configure with CMake
    Write-Host "Configuring with CMake..." -ForegroundColor Cyan
    
    $cmakeArgs = @(
        "-G", $Generator,
        "-DCMAKE_BUILD_TYPE=$BuildType"
    )
    
    # Add custom CMake options
    foreach ($option in $CMakeOptions) {
        $cmakeArgs += $option
    }
    
    $cmakeArgs += ".."
    
    Write-Host "CMake command: cmake $($cmakeArgs -join ' ')" -ForegroundColor Gray
    
    # 使用实时输出而不是重定向
    Write-Host "Executing CMake configuration..." -ForegroundColor Yellow
    Write-Host "Note: Configuration output will be displayed in real-time." -ForegroundColor Cyan
    
    try {
        & cmake @cmakeArgs
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Error: CMake configuration failed with exit code $LASTEXITCODE!" -ForegroundColor Red
            Write-Host "Please check the configuration output above for specific error messages." -ForegroundColor Yellow
            exit 1
        }
    } catch {
        Write-Host "Error: CMake configuration process was interrupted or failed!" -ForegroundColor Red
        Write-Host "Exception: $($_.Exception.Message)" -ForegroundColor Red
        exit 1
    }
    
    # Build the project
    Write-Host "Building the project..." -ForegroundColor Cyan
    
    $buildArgs = @(
        "--build", ".",
        "--config", $BuildType,
        "--verbose"  # 添加详细输出
    )
    
    if ($Parallel -gt 0) {
        $buildArgs += "--parallel", $Parallel
        Write-Host "Using $Parallel parallel jobs for building" -ForegroundColor Yellow
    } else {
        Write-Host "Using default parallel jobs for building" -ForegroundColor Yellow
    }
    
    Write-Host "Build command: cmake $($buildArgs -join ' ')" -ForegroundColor Gray
    Write-Host "Starting build process..." -ForegroundColor Green
    
    # 使用实时输出而不是重定向
    Write-Host "Executing CMake build..." -ForegroundColor Yellow
    Write-Host "Note: Build output will be displayed in real-time. Press Ctrl+C to stop if needed." -ForegroundColor Cyan
    
    # 确保输出不被缓冲
    $env:PYTHONUNBUFFERED = "1"
    
    try {
        & cmake @buildArgs
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Error: Build failed with exit code $LASTEXITCODE!" -ForegroundColor Red
            Write-Host "Please check the build output above for specific error messages." -ForegroundColor Yellow
            exit 1
        }
    } catch {
        Write-Host "Error: Build process was interrupted or failed!" -ForegroundColor Red
        Write-Host "Exception: $($_.Exception.Message)" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "Build completed successfully!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Build artifacts are located in:" -ForegroundColor Cyan
    Write-Host "  - Libraries: build\$BuildType\" -ForegroundColor White
    Write-Host "  - Executables: build\$BuildType\" -ForegroundColor White
    Write-Host "  - Headers: build\include\" -ForegroundColor White
    Write-Host ""
    
    # List generated executables
    $exePath = "bin\$BuildType"
    if (Test-Path $exePath) {
        Write-Host "Generated executables:" -ForegroundColor Cyan
        Get-ChildItem -Path $exePath -Filter "*.exe" | ForEach-Object {
            Write-Host "  - $($_.Name)" -ForegroundColor White
        }
    }
    
} finally {
    # Return to original directory
    Pop-Location
}

Write-Host ""
Write-Host "To run an example:" -ForegroundColor Cyan
Write-Host "  cd build\examples\chat\chatbot\$BuildType" -ForegroundColor White
Write-Host "  .\onesdk_chatbot_example.exe" -ForegroundColor White 