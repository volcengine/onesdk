# OneSDK Windows 构建问题总结

## 已解决的问题

### 1. CMake版本警告 ✅
**问题**: 示例CMakeLists.txt文件中的cmake_minimum_required版本太低
**解决方案**: 已将所有示例文件的CMake版本要求从3.5更新到3.10

### 2. target_link_libraries语法错误 ✅
**问题**: 多个示例文件中的target_link_libraries语法错误
**解决方案**: 已修复所有示例文件中的语法错误，将${PLATFORM_LIBS}正确放置在target_link_libraries参数中

### 3. 平台检测和配置 ✅
**问题**: 需要添加Windows平台支持
**解决方案**: 已在主CMakeLists.txt中添加了完整的Windows平台检测和配置

## 当前遇到的问题

### 1. libwebsockets编码问题 ❌
**问题**: libwebsockets源文件包含非ASCII字符，导致Visual Studio编译错误
**错误信息**:
```
warning C4819: 该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失。
error C2220: 以下警告被视为错误
```

**解决方案**:
1. **临时解决方案**: 在CMakeLists.txt中添加编码设置
   ```cmake
   if(MSVC)
       add_compile_options(/utf-8)
   endif()
   ```

2. **推荐解决方案**: 使用预编译的libwebsockets库
   - 下载预编译的Windows版本
   - 或使用vcpkg安装: `vcpkg install libwebsockets:x64-windows`

### 2. 外部库依赖问题 ❌
**问题**: external_libs目录中的库文件可能不完整或版本不兼容
**解决方案**:
1. 确保所有子模块都已正确下载
2. 考虑使用系统安装的库而不是项目内的库

## 构建状态

### ✅ 成功的部分
- CMake配置成功
- 平台检测正常工作
- 示例CMakeLists.txt语法正确
- cJSON库编译成功

### ❌ 失败的部分
- libwebsockets库编译失败（编码问题）
- OneSDK主库编译失败（依赖libwebsockets）

## 推荐的解决方案

### 方案1: 修复编码问题（推荐）
在项目根目录的CMakeLists.txt中添加以下代码：

```cmake
# Add UTF-8 support for MSVC
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_options(/wd4819)  # Suppress encoding warnings
endif()
```

### 方案2: 使用预编译库
1. 安装vcpkg包管理器
2. 安装预编译的libwebsockets:
   ```cmd
   vcpkg install libwebsockets:x64-windows
   ```
3. 修改CMakeLists.txt使用系统库而不是项目内库

### 方案3: 使用MinGW-w64
MinGW-w64通常对编码问题更宽容：
```cmd
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

## 测试建议

### 1. 测试CMake配置
```cmd
python scripts/test_cmake_config.py
```

### 2. 测试环境检查
```cmd
python scripts/test_windows_build.py
```

### 3. 最小化构建测试
```cmd
.\build.ps1 -CMakeOptions "-DONESDK_WITH_EXAMPLE=OFF"
```

## 下一步行动

1. **立即**: 添加UTF-8编码支持到CMakeLists.txt
2. **短期**: 测试修复后的构建
3. **中期**: 考虑使用预编译库或更新外部库版本
4. **长期**: 完善Windows构建文档和自动化测试

## 相关文件

- `CMakeLists.txt` - 主构建配置
- `build.ps1` - PowerShell构建脚本
- `docs/develop_windows.md` - Windows开发指南
- `scripts/test_cmake_config.py` - CMake配置测试
- `scripts/test_windows_build.py` - 环境检查脚本 