# OneSDK Windows 构建支持

本文档总结了为OneSDK项目添加的Windows平台构建支持。

## 概述

OneSDK现在完全支持在Windows平台上进行构建和开发，包括：

- ✅ Visual Studio 2019/2022 支持
- ✅ MinGW-w64 支持
- ✅ CMake 跨平台构建系统
- ✅ 所有示例程序的Windows构建
- ✅ 详细的Windows开发文档

## 新增文件

### 构建脚本
- `build.bat` - Windows批处理构建脚本
- `build.ps1` - PowerShell构建脚本（功能更强大）

### 文档
- `docs/develop_windows.md` - 详细的Windows开发指南
- `WINDOWS_BUILD_SUPPORT.md` - 本文档

### 工具脚本
- `scripts/update_windows_support.py` - 批量更新示例CMakeLists.txt
- `scripts/test_windows_build.py` - Windows构建环境检查
- `scripts/test_cmake_config.py` - CMake配置测试

## 修改的文件

### 核心构建系统
- `CMakeLists.txt` - 添加Windows平台检测和配置
- `platform/plat/platform.c` - 添加Windows平台特定代码

### 示例构建文件
所有 `examples/*/CMakeLists.txt` 文件都已更新以支持Windows：
- 添加平台检测
- 条件编译支持
- Windows特定库链接
- 平台特定包含路径

## 主要特性

### 1. 平台自动检测
CMake现在会自动检测Windows平台并应用相应的配置：
```cmake
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(BUILD_PLATFORM "windows")
    # Windows specific settings
endif()
```

### 2. 编译器支持
- **Visual Studio MSVC**: 完整的Visual Studio 2019/2022支持
- **MinGW-w64**: 开源GCC工具链支持
- 自动检测和配置

### 3. 库依赖处理
- Windows特定库自动链接（ws2_32, crypt32, iphlpapi）
- OpenSSL路径配置
- Windows SDK路径支持

### 4. 构建选项
支持所有现有的CMake选项：
```cmd
# 仅构建AI功能
build.bat -DONESDK_ENABLE_IOT=OFF

# 构建调试版本
build.bat -DCMAKE_BUILD_TYPE=Debug

# 并行构建
build.bat --parallel 8
```

## 快速开始

### 1. 环境要求
- Windows 10 或更高版本
- Visual Studio 2019/2022 或 MinGW-w64
- CMake 3.10+
- Git

### 2. 构建步骤
```cmd
# 克隆项目
git clone --recursive https://github.com/volcengine/onesdk.git
cd onesdk

# 使用Visual Studio构建（推荐）
build.bat

# 或使用PowerShell脚本
.\build.ps1

# 或使用MinGW-w64
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

### 3. 运行示例
```cmd
cd build\bin\Release
.\onesdk_chatbot_example.exe
```

## 环境检查

运行环境检查脚本：
```cmd
python scripts/test_windows_build.py
```

运行CMake配置测试：
```cmd
python scripts/test_cmake_config.py
```

## 故障排除

### 常见问题

1. **CMake找不到编译器**
   - 确保安装了Visual Studio或MinGW-w64
   - 检查PATH环境变量

2. **OpenSSL头文件找不到**
   - 设置 `OPENSSL_ROOT_DIR` 环境变量
   - 或使用vcpkg安装OpenSSL

3. **Windows SDK找不到**
   - 确保在Visual Studio安装器中选中了Windows SDK
   - 或设置 `WINDOWS_SDK_PATH` 环境变量

### 详细故障排除
请参考 `docs/develop_windows.md` 中的详细故障排除指南。

## 开发工具集成

### Visual Studio Code
1. 安装C/C++和CMake Tools扩展
2. 打开项目文件夹
3. 使用Ctrl+Shift+P选择"CMake: Configure"

### CLion
1. 打开项目文件夹
2. CLion会自动检测CMakeLists.txt
3. 配置工具链

## 测试状态

- ✅ CMake配置测试通过
- ✅ 平台检测正常工作
- ✅ 示例构建文件更新完成
- ✅ 文档完整

## 贡献

如果您在使用Windows构建时遇到问题，请：

1. 查看 `docs/develop_windows.md` 中的故障排除指南
2. 运行环境检查脚本
3. 提交Issue并包含详细的错误信息

## 相关链接

- [Windows开发指南](docs/develop_windows.md)
- [主开发指南](docs/develop.md)
- [项目README](README.md) 