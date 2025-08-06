# OneSDK Windows Development Guide

This document provides a detailed guide for setting up the OneSDK development environment, compiling source code, and running examples on the Windows platform.

## 1. Environment Requirements

### 1.1. System Requirements
- **Operating System**: Windows 10 or higher
- **Architecture**: x64 (recommended) or x86
- **Memory**: At least 4GB RAM
- **Disk Space**: At least 2GB available space

### 1.2. Required Tools

#### 1.2.1. Visual Studio (Recommended)
- **Visual Studio 2019** or **Visual Studio 2022**
- Install the following workloads:
  - Desktop development with C++
  - CMake tools
- Or install **Visual Studio Build Tools**

#### 1.2.2. Alternative: MinGW-w64
If you don't want to use Visual Studio, you can use MinGW-w64:
- Download and install [MSYS2](https://www.msys2.org/)
- Install MinGW-w64 toolchain through MSYS2

#### 1.2.3. CMake
- **Version Requirement**: CMake 3.10+ but not higher than 4.x
- **Installation Methods**:
  - Install through Visual Studio installer
  - Or download from [CMake official website](https://cmake.org/download/)

#### 1.2.4. Git
- Download and install from [Git official website](https://git-scm.com/download/win)

#### 1.2.5. OpenSSL (Optional)
If TLS support is needed, it's recommended to install OpenSSL:
- Download from [OpenSSL official website](https://slproweb.com/products/Win32OpenSSL.html)
- Or install using vcpkg package manager

## 2. Environment Setup

### 2.1. Using Visual Studio (Recommended)

#### 2.1.1. Install Visual Studio
1. Download [Visual Studio Installer](https://visualstudio.microsoft.com/downloads/)
2. Run the installer and select "Desktop development with C++" workload
3. Ensure the following components are selected:
   - MSVC v143 compiler toolset
   - Windows 10/11 SDK
   - CMake tools
   - Git for Windows

#### 2.1.2. Configure Environment Variables
Set the following environment variables (optional, for custom paths):
```cmd
set OPENSSL_ROOT_DIR=C:\OpenSSL-Win64
set WINDOWS_SDK_PATH=C:\Program Files (x86)\Windows Kits\10
```

### 2.2. Using MinGW-w64 (Alternative)

#### 2.2.1. Install MSYS2
1. Download and install [MSYS2](https://www.msys2.org/)
2. Open MSYS2 terminal and update package database:
   ```bash
   pacman -Syu
   ```

#### 2.2.2. Install Development Tools
```bash
pacman -S mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-openssl
pacman -S git
```

#### 2.2.3. Configure PATH
Add MinGW-w64's bin directory to system PATH:
```
C:\msys64\mingw64\bin
```

## 3. Get the Code

### 3.1. Clone Repository
```cmd
git clone --recursive https://github.com/volcengine/onesdk.git
cd onesdk
```

### 3.2. Check Dependencies
Ensure all submodules are properly downloaded:
```cmd
git submodule update --init --recursive
```

## 4. Configure Connection Credentials

Before running examples, you need to obtain the following credentials from the Volcengine IoT Platform console and configure them in the example source code:

- **IoT Management API Host**: `https://iot-cn-shanghai.iot.volces.com` (fixed value)
- **IoT MQTT Host**: `<instance-id>-cn-shanghai.iot.volces.com`
- **Instance ID**: Your service instance ID
- **Product Key**: Required for all authentication types
- **Product Secret**: Required for "by type" authentication
- **Device Name**: Required for all authentication types
- **Device Secret**: Required for "by device" authentication

*Note: For "by type" dynamic registration (`ONESDK_AUTH_DYNAMIC_PRE_REGISTERED` or `ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED`), you must enable the "Enable Dynamic Registration" switch for the product in the console.*

## 5. Build and Run Examples

### 5.1. Using Visual Studio

#### 5.1.1. Configure Example Parameters
Open example files, such as `examples/chat/text_image/main_text_image.c`, and fill in the credentials obtained in the previous step:

```c
#define SAMPLE_HTTP_HOST "https://iot-cn-shanghai.iot.volces.com"
#define SAMPLE_INSTANCE_ID "<your_instance_id>"
#define SAMPLE_MQTT_HOST "<your_instance_id>.cn-shanghai.iot.volces.com"
#define SAMPLE_PRODUCT_KEY "<your_product_key>"
#define SAMPLE_DEVICE_NAME "<your_device_name>"

// For device-based authentication
#define SAMPLE_DEVICE_SECRET "<your_device_secret>" 

// For type-based authentication
#define SAMPLE_PRODUCT_SECRET "<your_product_secret>"
```

#### 5.1.2. Build Project

Run in the project root directory:

```cmd
build.bat
```
Or

```powershell
build.ps1
```

Or use CMake GUI:
1. Open CMake GUI
2. Set source directory to project root directory
3. Set build directory to `build`
4. Click "Configure" and select Visual Studio version
5. Click "Generate"
6. Open the generated Visual Studio solution file for building

#### 5.1.3. Run Examples
Compiled executables are located in the `build\Release\` directory:
```cmd
cd build\examples\chat\chatbot\Release
onesdk_chatbot_example.exe
```

### 5.2. Using MinGW-w64

#### 5.2.1. Configure Example Parameters
Same as Visual Studio method.

#### 5.2.2. Build Project
```cmd
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

#### 5.2.3. Run Examples
```cmd
cd bin
onesdk_chatbot_example.exe
```

## 6. Build Options

You can customize the build by passing CMake options to `build.bat`:
```cmd
build.bat -D<OPTION_NAME>=<VALUE>
```
You can also pass CMake options through `build.ps1`:

```powershell
.\build.ps1 -CMakeOptions @("-D<OPTION_NAME1>=<VALUE1>", "-D<OPTION_NAME2>=<VALUE2>")
```

| Option                      | Default | Description                                              |
| --------------------------- | ------- | -------------------------------------------------------- |
| `ONESDK_ENABLE_AI`          | `ON`    | Enable AI functionality (text, image).                        |
| `ONESDK_ENABLE_AI_REALTIME` | `ON`    | Enable real-time AI functionality (such as WebSocket audio).    |
| `ONESDK_ENABLE_IOT`         | `ON`    | Enable IoT functionality (MQTT, OTA, thing model).            |
| `ONESDK_WITH_EXAMPLE`       | `ON`    | Build example applications.                              |
| `ONESDK_WITH_TEST`          | `OFF`   | Build unit tests.                                        |
| `ONESDK_WITH_SHARED`        | `ON`    | Build shared libraries.                                  |
| `ONESDK_WITH_STATIC`        | `ON`    | Build static libraries.                                  |
| `ONESDK_WITH_STRICT_MODE`   | `OFF`   | Enable compiler address sanitizer.                       |

### 6.1. Example Build Commands

#### CMD
```cmd
# Build AI functionality only
build.bat -DONESDK_ENABLE_IOT=OFF

# Build release version
build.bat -DCMAKE_BUILD_TYPE=Release

# Build static libraries only
build.bat -DONESDK_WITH_SHARED=OFF
```
#### PowerShell

```powershell
# Release build with examples
.\build.ps1 -BuildType Release -CMakeOptions @("-DONESDK_WITH_EXAMPLE=ON", "-DONESDK_WITH_TEST=ON")

# Release build
.\build.ps1 -BuildType Release -CMakeOptions @("-DONESDK_WITH_EXAMPLE=ON")

# ESP32 source code extraction
.\build.ps1 -CMakeOptions @("-DONESDK_EXTRACT_SRC=ON", "-DONESDK_WITH_TEST=OFF", "-DONESDK_WITH_EXAMPLE=OFF")
```
## 7. Troubleshooting

### 7.1. Common Compilation Errors

#### 7.1.1. `fatal error: 'openssl/ssl.h' file not found`
**Solution**:
1. Install OpenSSL and set environment variables:
   ```cmd
   set OPENSSL_ROOT_DIR=C:\OpenSSL-Win64
   ```
2. Or install using vcpkg:
   ```cmd
   vcpkg install openssl:x64-windows
   ```

#### 7.1.2. `LNK1104: cannot open file 'ws2_32.lib'`
**Solution**:
Ensure Windows SDK is installed and "Windows 10/11 SDK" is selected in Visual Studio installer.

#### 7.1.3. `CMake Error: Could not find a package configuration file`
**Solution**:
1. Ensure CMake version is 3.10 or higher
2. Check if all dependency libraries are properly installed

### 7.2. Runtime Errors

#### 7.2.1. `SSL error: self-signed certificate in certificate chain`
**Solution**:
1. Verify connection and certificates:
   ```cmd
   curl https://iot-cn-shanghai.iot.volces.com
   ```
2. If it fails, update the system's CA store

#### 7.2.2. `The dynamic register switch of product is not turned on`
**Solution**:
For "by type" authentication, ensure "Dynamic Registration" is enabled for the product in the IoT console.

#### 7.2.3. `No access to model xxxxxxx`
**Solution**:
In the console, go to `IoT Platform -> AI Services -> Model Configuration` and add the required AI models to your product.

### 7.3. Performance Optimization

#### 7.3.1. Parallel Building
Use multi-core building to speed up compilation:
```cmd
build.bat --parallel 8
```

#### 7.3.2. Incremental Building
Avoid cleaning build directory to leverage incremental compilation:
```cmd
build.bat
```

## 8. Development Tool Integration

### 8.1. Visual Studio Code
1. Install C/C++ extension
2. Install CMake Tools extension
3. Open project folder
4. Use Ctrl+Shift+P to open command palette and select "CMake: Configure"

### 8.2. CLion
1. Open project folder
2. CLion will automatically detect CMakeLists.txt
3. Configure toolchain as Visual Studio or MinGW

## 9. Debugging

### 9.1. Visual Studio Debugging
1. Open the generated solution in Visual Studio
2. Set breakpoints
3. Press F5 to start debugging

### 9.2. Command Line Debugging
```cmd
# Using GDB (MinGW)
gdb onesdk_chatbot_example.exe

# Using Visual Studio debugger
devenv onesdk_chatbot_example.exe
```

## 10. Related Links

- [Volcengine IoT Platform](https://www.volcengine.com/docs/6893/1455924)
- [AI Gateway](https://www.volcengine.com/docs/6893/1263412)
- [CMake Official Documentation](https://cmake.org/documentation/)
- [Visual Studio Documentation](https://docs.microsoft.com/visualstudio/)
- [MinGW-w64 Documentation](https://www.mingw-w64.org/documentation/)