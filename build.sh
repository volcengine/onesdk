# 添加clean-build参数处理
if [ "$1" = "clean-build" ]; then
    rm -rf build
    shift  # 移除clean-build参数
fi

mkdir -p build
cd build
cmake $@ ..

# 检查nproc是否存在
if command -v nproc >/dev/null 2>&1; then
    jobs=$(nproc)
else
    jobs=10
fi
make -j $jobs

cd -