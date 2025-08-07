# OneSDK Windows 开发指南

本文档提供了在Windows平台上搭建OneSDK开发环境、编译源代码和运行示例的详细指南。

## 1. 环境要求

### 1.1. 系统要求
- **操作系统**: Windows 10 或更高版本
- **架构**: x64 (推荐) 或 x86
- **内存**: 至少 4GB RAM
- **磁盘空间**: 至少 2GB 可用空间

### 1.2. 必需工具

#### 1.2.1. Visual Studio (推荐)
- **Visual Studio 2019** 或 **Visual Studio 2022**
- 安装以下工作负载：
  - 使用C++的桌面开发
  - CMake工具
- 或者安装 **Visual Studio Build Tools**

#### 1.2.2. 替代方案：MinGW-w64
如果不想使用Visual Studio，可以使用MinGW-w64：
- 下载并安装 [MSYS2](https://www.msys2.org/)
- 通过MSYS2安装MinGW-w64工具链

#### 1.2.3. CMake
- **版本要求**: CMake 3.10+ 但不高于4.x
- **安装方式**:
  - 通过Visual Studio安装器安装
  - 或从 [CMake官网](https://cmake.org/download/) 下载安装

#### 1.2.4. Git
- 从 [Git官网](https://git-scm.com/download/win) 下载安装

#### 1.2.5. OpenSSL (可选)
如果需要TLS支持，建议安装OpenSSL：
- 从 [OpenSSL官网](https://slproweb.com/products/Win32OpenSSL.html) 下载
- 或使用vcpkg包管理器安装

## 2. 环境搭建

### 2.1. 使用Visual Studio (推荐)

#### 2.1.1. 安装Visual Studio
1. 下载 [Visual Studio Installer](https://visualstudio.microsoft.com/downloads/)
2. 运行安装器，选择"使用C++的桌面开发"工作负载
3. 确保选中以下组件：
   - MSVC v143编译器工具集
   - Windows 10/11 SDK
   - CMake工具
   - Git for Windows

#### 2.1.2. 配置环境变量
设置以下环境变量（可选，用于自定义路径）：
```cmd
set OPENSSL_ROOT_DIR=C:\OpenSSL-Win64
set WINDOWS_SDK_PATH=C:\Program Files (x86)\Windows Kits\10
```

### 2.2. 使用MinGW-w64（备选）

#### 2.2.1. 安装MSYS2
1. 下载并安装 [MSYS2](https://www.msys2.org/)
2. 打开MSYS2终端，更新包数据库：
   ```bash
   pacman -Syu
   ```

#### 2.2.2. 安装开发工具
```bash
pacman -S mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-openssl
pacman -S git
```

#### 2.2.3. 配置PATH
将MinGW-w64的bin目录添加到系统PATH：
```
C:\msys64\mingw64\bin
```

## 3. 获取代码

### 3.1. 克隆仓库
```cmd
git clone --recursive https://github.com/volcengine/onesdk.git
cd onesdk
```

### 3.2. 检查依赖
确保所有子模块都已正确下载：
```cmd
git submodule update --init --recursive
```

## 4. 配置连接凭据

在运行示例之前，需要从火山引擎IoT平台控制台获取以下凭据并配置到示例源代码中：

- **IoT管理API主机**: `https://iot-cn-shanghai.iot.volces.com` (固定值)
- **IoT MQTT主机**: `<instance-id>-cn-shanghai.iot.volces.com`
- **实例ID**: 您的服务实例ID
- **产品密钥**: 所有认证类型都需要
- **产品密钥**: "按类型"认证需要
- **设备名称**: 所有认证类型都需要
- **设备密钥**: "按设备"认证需要

*注意：对于"按类型"动态注册（`ONESDK_AUTH_DYNAMIC_PRE_REGISTERED`或`ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED`），您必须在控制台中为产品启用"启用动态注册"开关。*

## 5. 构建和运行示例

### 5.1. 使用Visual Studio

#### 5.1.1. 配置示例参数
打开示例文件，如 `examples/chat/text_image/main_text_image.c`，填入上一步获取的凭据：

```c
#define SAMPLE_HTTP_HOST "https://iot-cn-shanghai.iot.volces.com"
#define SAMPLE_INSTANCE_ID "<your_instance_id>"
#define SAMPLE_MQTT_HOST "<your_instance_id>.cn-shanghai.iot.volces.com"
#define SAMPLE_PRODUCT_KEY "<your_product_key>"
#define SAMPLE_DEVICE_NAME "<your_device_name>"

// 用于按设备认证
#define SAMPLE_DEVICE_SECRET "<your_device_secret>" 

// 用于按类型认证
#define SAMPLE_PRODUCT_SECRET "<your_product_secret>"
```

#### 5.1.2. 构建项目

在项目根目录运行：

```cmd
build.bat
```
或者

```powershell
build.ps1
```

或者使用CMake GUI：
1. 打开CMake GUI
2. 设置源代码目录为项目根目录
3. 设置构建目录为 `build`
4. 点击"Configure"，选择Visual Studio版本
5. 点击"Generate"
6. 打开生成的Visual Studio解决方案文件进行构建

#### 5.1.3. 运行示例
编译后的可执行文件位于 `build\Release\` 目录：
```cmd
cd build\examples\chat\chatbot\Release
onesdk_chatbot_example.exe
```

### 5.2. 使用MinGW-w64

#### 5.2.1. 配置示例参数
同Visual Studio方式。

#### 5.2.2. 构建项目
```cmd
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

#### 5.2.3. 运行示例
```cmd
cd bin
onesdk_chatbot_example.exe
```

## 6. 构建选项

您可以通过向 `build.bat` 传递CMake选项来自定义构建：
```cmd
build.bat -D<OPTION_NAME>=<VALUE>
```
也可以通过`build.ps1` 传递CMake选项

```powershell
.\build.ps1 -CMakeOptions @("-D<OPTION_NAME1>=<VALUE1>", "-D<OPTION_NAME2>=<VALUE2>")
```

| 选项                      | 默认值 | 描述                                              |
| --------------------------- | ------- | -------------------------------------------------------- |
| `ONESDK_ENABLE_AI`          | `ON`    | 启用AI功能（文本、图像）。                        |
| `ONESDK_ENABLE_AI_REALTIME` | `ON`    | 启用实时AI功能（如WebSocket音频）。    |
| `ONESDK_ENABLE_IOT`         | `ON`    | 启用IoT功能（MQTT、OTA、物模型）。            |
| `ONESDK_WITH_EXAMPLE`       | `ON`    | 构建示例应用程序。                              |
| `ONESDK_WITH_TEST`          | `OFF`   | 构建单元测试。                                        |
| `ONESDK_WITH_SHARED`        | `ON`    | 构建共享库。                                  |
| `ONESDK_WITH_STATIC`        | `ON`    | 构建静态库。                                  |
| `ONESDK_WITH_STRICT_MODE`   | `OFF`   | 启用编译器地址消毒器。                       |

### 6.1. 示例构建命令

#### CMD
```cmd
# 仅构建AI功能
build.bat -DONESDK_ENABLE_IOT=OFF

# 构建发布版本
build.bat -DCMAKE_BUILD_TYPE=Release

# 仅构建静态库
build.bat -DONESDK_WITH_SHARED=OFF
```
#### PowerShell

```powershell
# 发布构建和示例
.\build.ps1 -BuildType Release -CMakeOptions @("-DONESDK_WITH_EXAMPLE=ON", "-DONESDK_WITH_TEST=ON")

# 发布构建
.\build.ps1 -BuildType Release -CMakeOptions @("-DONESDK_WITH_EXAMPLE=ON")

# ESP32源码提取
.\build.ps1 -CMakeOptions @("-DONESDK_EXTRACT_SRC=ON", "-DONESDK_WITH_TEST=OFF", "-DONESDK_WITH_EXAMPLE=OFF")
```
## 7. 故障排除

### 7.1. 常见编译错误

#### 7.1.1. `fatal error: 'openssl/ssl.h' file not found`
**解决方案**:
1. 安装OpenSSL并设置环境变量：
   ```cmd
   set OPENSSL_ROOT_DIR=C:\OpenSSL-Win64
   ```
2. 或者使用vcpkg安装：
   ```cmd
   vcpkg install openssl:x64-windows
   ```

#### 7.1.2. `LNK1104: cannot open file 'ws2_32.lib'`
**解决方案**:
确保安装了Windows SDK，并在Visual Studio安装器中选中了"Windows 10/11 SDK"。

#### 7.1.3. `CMake Error: Could not find a package configuration file`
**解决方案**:
1. 确保CMake版本为3.10或更高
2. 检查是否所有依赖库都已正确安装

### 7.2. 运行时错误

#### 7.2.1. `SSL error: self-signed certificate in certificate chain`
**解决方案**:
1. 验证连接和证书：
   ```cmd
   curl https://iot-cn-shanghai.iot.volces.com
   ```
2. 如果失败，更新系统的CA存储

#### 7.2.2. `The dynamic register switch of product is not turned on`
**解决方案**:
对于"按类型"认证，确保在IoT控制台中为产品启用了"动态注册"。

#### 7.2.3. `No access to model xxxxxxx`
**解决方案**:
在控制台中转到 `IoT Platform -> AI Services -> Model Configuration`，并将所需的AI模型添加到您的产品中。

### 7.3. 性能优化

#### 7.3.1. 并行构建
使用多核构建以加快编译速度：
```cmd
build.bat --parallel 8
```

#### 7.3.2. 增量构建
避免清理构建目录以利用增量编译：
```cmd
build.bat
```

## 8. 开发工具集成

### 8.1. Visual Studio Code
1. 安装C/C++扩展
2. 安装CMake Tools扩展
3. 打开项目文件夹
4. 使用Ctrl+Shift+P打开命令面板，选择"CMake: Configure"

### 8.2. CLion
1. 打开项目文件夹
2. CLion会自动检测CMakeLists.txt
3. 配置工具链为Visual Studio或MinGW

## 9. 调试

### 9.1. Visual Studio调试
1. 在Visual Studio中打开生成的解决方案
2. 设置断点
3. 按F5开始调试

### 9.2. 命令行调试
```cmd
# 使用GDB (MinGW)
gdb onesdk_chatbot_example.exe

# 使用Visual Studio调试器
devenv onesdk_chatbot_example.exe
```

## 10. 相关链接

- [火山引擎IoT平台](https://www.volcengine.com/docs/6893/1455924)
- [AI网关](https://www.volcengine.com/docs/6893/1263412)
- [CMake官方文档](https://cmake.org/documentation/)
- [Visual Studio文档](https://docs.microsoft.com/visualstudio/)
- [MinGW-w64文档](https://www.mingw-w64.org/documentation/) 