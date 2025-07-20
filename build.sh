#!/bin/bash
set -e  # Exit on any error

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