#!/bin/bash
set -e  # Exit on any error

# Function to check CMake version
check_cmake_version() {
    if ! command -v cmake &> /dev/null; then
        echo "Error: CMake is not installed!"
        exit 1
    fi
    
    cmake_version=$(cmake --version | head -n1 | cut -d' ' -f3)
    cmake_major=$(echo $cmake_version | cut -d'.' -f1)
    cmake_minor=$(echo $cmake_version | cut -d'.' -f2)
    
    echo "Detected CMake version: $cmake_version"
    
    # Check if version is 3.x (major version 3, minor version >= 10)
    if [ "$cmake_major" -eq 3 ] && [ "$cmake_minor" -ge 10 ]; then
        echo "CMake version is compatible (3.x)"
    elif [ "$cmake_major" -gt 3 ]; then
        echo "Warning: CMake version $cmake_version is higher than 3.x"
        echo "This may cause compatibility issues with cJSON"
        echo "Consider using CMake 3.26.x for best compatibility"
        read -p "Continue anyway? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    else
        echo "Error: CMake version $cmake_version is too old. Minimum required: 3.10"
        exit 1
    fi
}

# Check CMake version before proceeding
check_cmake_version

# Add clean-build parameter handling
if [ "$1" = "clean-build" ]; then
    rm -rf build
    shift  # Remove clean-build parameter
fi

mkdir -p build
cd build

echo "Running cmake with arguments: $@"
cmake $@ ..
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Check if nproc exists
if command -v nproc >/dev/null 2>&1; then
    jobs=$(nproc)
else
    jobs=10
fi

echo "Building with $jobs jobs..."
make -j $jobs
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build completed successfully!"
cd -