#!/bin/bash
# 拷贝onesdk源码到examples/onesdk_esp32目录下
# 注意：需要先执行build.sh脚本，生成build/output目录


# 检查是否是onesdk根目录执行，否则报错
ONESDK_ROOT_DIR=$(dirname $(dirname $(realpath $0)))
echo "onesdk root directory: $ONESDK_ROOT_DIR"
if [ ! -f "$ONESDK_ROOT_DIR/CMakeLists.txt" ]; then
    echo "please run this script within onesdk root directory"
    exit 1
fi
# 检查build/output目录是否存在
if [ ! -d "$ONESDK_ROOT_DIR/build/output" ]; then
    echo "build/output directory not found, execute 'build.sh -DONESDK_EXTRACT_SRC=ON' first"
    exit 1
fi


# 使用绝对路径进行拷贝操作
for example_dir in "onesdk_esp32" "onesdk_esp32_audio"; do
    cp -rv "$ONESDK_ROOT_DIR/build/output/"* "$ONESDK_ROOT_DIR/examples/${example_dir}/"
    cp -v "$ONESDK_ROOT_DIR/components_onesdk.txt" "$ONESDK_ROOT_DIR/examples/${example_dir}/components/onesdk/CMakeLists.txt"
    # cp -v "$ONESDK_ROOT_DIR/libwebsockets_cmakelists.txt" "$ONESDK_ROOT_DIR/examples/${example_dir}/libwebsockets/CMakeLists.txt"
done