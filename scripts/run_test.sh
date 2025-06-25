#!/bin/bash
set -e  # 遇到错误立即退出
set -x  # 打印执行命令

# 清理并重建构建目录
rm -rf build
mkdir -p build
cd build

# 配置CMake（覆盖率模式）
cmake .. -DCMAKE_BUILD_TYPE=Coverage

# 并行编译覆盖率目标
make coverage -j $(nproc)

cd ..
