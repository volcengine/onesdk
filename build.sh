# Add clean-build parameter handling
if [ "$1" = "clean-build" ]; then
    rm -rf build
    shift  # Remove clean-build parameter
fi

mkdir -p build
cd build
cmake $@ ..

# Check if nproc exists
if command -v nproc >/dev/null 2>&1; then
    jobs=$(nproc)
else
    jobs=10
fi
make -j $jobs

cd -