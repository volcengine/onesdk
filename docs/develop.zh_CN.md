[English](develop.md) | 中文README
---

## 开发指南

本文档为安装、编译和运行 OneSDK 提供了全面的指引。

### 1. 前置条件

#### 1.1. 开发工具链

请确保您的系统已安装以下工具：

- **CMake**: 3.10 或更高版本。
- **GCC/Clang**: C/C++ 编译器。
- **Git**: 用于克隆代码仓库。
- **OpenSSL 或 MbedTLS**: 用于 TLS 安全通信。
  - **OpenSSL (推荐用于 Linux/macOS):**
    - macOS: `brew install openssl`
    - Ubuntu/Debian: `sudo apt-get install libssl-dev`
    - CentOS/RHEL: `sudo yum install openssl openssl-devel`
  - **MbedTLS (推荐用于嵌入式平台):**
    - 建议采用 mbedtls v3.4.0 版本以获得最佳兼容性。请参考 [MbedTLS 官方仓库](https://github.com/Mbed-TLS/mbedtls)进行安装。

#### 1.2. 开通火山引擎服务

在使用 SDK 前，您需要在火山引擎控制台开通相关产品：

1.  **开通【边缘智能】**: 参考[官方指引](https://www.volcengine.com/docs/6893/1134368)开通服务。
2.  **开通【物联网平台】**: 开通物联网平台服务并完成授权。
3.  **获取实例**: 您可以通过联系产品经理或在火山引擎控制台发起 oncall 来获取服务实例。

### 2. 获取代码

从 GitHub 克隆代码仓库：

```bash
git clone --recursive https://github.com/volcengine/onesdk.git
cd onesdk
```

### 3. 配置连接凭证

运行示例程序前，您需要从火山引擎物联网平台控制台获取以下凭证，并填入示例代码中。

-   **物联网管理API地址**: `https://iot-cn-shanghai.iot.volces.com` (固定值)
-   **物联网MQTT地址**: `<instance-id>-cn-shanghai.iot.volces.com`
-   **实例ID (Instance ID)**: 您的服务实例ID。
-   **产品Key (Product Key)**: 所有认证方式都需要。
-   **产品密钥 (Product Secret)**: "一型一密"认证方式需要。
-   **设备名称 (Device Name)**: 所有认证方式都需要。
-   **设备密钥 (Device Secret)**: "一机一密"认证方式需要。

*请注意：对于"一型一密"动态注册（`ONESDK_AUTH_DYNAMIC_PRE_REGISTERED` 或 `ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED`），您必须在控制台的产品设置中开启【是否开启动态注册】开关。*

### 4. 编译和运行示例

#### 4.1. Linux/macOS 平台

1.  **编辑示例参数:**
    打开一个示例文件，例如 `examples/chat/text_image/main_text_image.c`，并将上一步获取的凭证填入其中。

    ```c
    #define SAMPLE_HTTP_HOST "https://iot-cn-shanghai.iot.volces.com"
    #define SAMPLE_INSTANCE_ID "<你的_instance_id>"
    #define SAMPLE_MQTT_HOST "<你的_instance_id>.cn-shanghai.iot.volces.com"
    #define SAMPLE_PRODUCT_KEY "<你的_product_key>"
    #define SAMPLE_DEVICE_NAME "<你的_device_name>"
    
    // 用于"一机一密"认证
    #define SAMPLE_DEVICE_SECRET "<你的_device_secret>" 
    
    // 用于"一型一密"认证
    #define SAMPLE_PRODUCT_SECRET "<你的_product_secret>"
    ```

2.  **构建项目:**
    在项目根目录运行 `build.sh` 脚本，它将编译 SDK 和所有示例。

    ```bash
    # 使用默认选项构建
    ./build.sh
    
    # 构建且不包含单元测试
    ./build.sh -DONESDK_WITH_TEST=OFF
    ```

3.  **运行示例:**
    编译生成的可执行文件位于 `build/bin` 目录下。

    ```bash
    ./build/examples/chat/text_image/onesdk_chat_example
    ```

#### 4.2. 乐鑫 ESP32 平台

对于 ESP32 开发，建议使用 VSCode ESP-IDF 插件或手动安装 ESP-IDF。

-   **VSCode 插件 (自动安装)**: [ESP-IDF VSCode Extension](https://github.com/espressif/vscode-esp-idf-extension)
-   **手动安装**: [ESP-IDF 入门指南](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html)
-   对于音频示例，还需要 **ESP-ADF** 开发框架: [ESP-ADF 入门指南](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/index.html)

**运行示例流程:**
1.  进入一个 ESP32 示例目录，如 `examples/onesdk_esp32/`。
2.  在对应的源文件（如 `main/chat.c`）中修改设备连接参数。
3.  遵循该目录下的 `README.md` 文件中的说明，进行配置、构建、烧录和运行。

### 5. 构建选项

您可以通过向 `build.sh` 脚本传递 CMake 选项来自定义构建：
`./build.sh -D<选项名>=<值>`

| 选项                        | 默认值  | 描述                                           |
| --------------------------- | ------- | ---------------------------------------------- |
| `ONESDK_ENABLE_AI`          | `ON`    | 启用 AI 功能（文本、图像）。                   |
| `ONESDK_ENABLE_AI_REALTIME` | `ON`    | 启用实时 AI 功能（如 WebSocket 音频流）。      |
| `ONESDK_ENABLE_IOT`         | `ON`    | 启用 IoT 功能（MQTT、OTA、物模型）。         |
| `ONESDK_WITH_EXAMPLE`       | `ON`    | 构建示例程序。                                 |
| `ONESDK_WITH_TEST`          | `ON`    | 构建单元测试。                                 |
| `ONESDK_WITH_SHARED`        | `ON`    | 构建动态链接库。                               |
| `ONESDK_WITH_STATIC`        | `ON`    | 构建静态链接库。                               |
| `ONESDK_WITH_STRICT_MODE`   | `OFF`   | 启用编译器的地址消毒器（Address Sanitizer）。    |

### 6. 常见问题处理

1.  **`Fatal error: 'openssl/ssl.h' file not found`**
    如果 CMake 找不到 TLS 头文件，您可能需要在根目录的 `CMakeLists.txt` 中手动指定路径。例如，在 Apple Silicon 平台的 Homebrew 环境下：
    `include_directories("/opt/homebrew/include")`

2.  **`SSL error: self-signed certificate in certificate chain`**
    这通常是由于系统根证书缺失或不正确导致的。
    -   首先通过 `curl https://iot-cn-shanghai.iot.volces.com` 确认连接和证书。
    -   如果命令报错，请尝试更新系统的 CA 证书库：
        -   CentOS/RHEL: `sudo update-ca-trust`
        -   Ubuntu/Debian: `sudo update-ca-certificates`

3.  **`The dynamic register switch of product is not turned on`**
    对于"一型一密"认证方式，请确保您已在物联网控制台的产品详情页开启了【动态注册】开关。

4.  **`No access to model xxxxxxx`**
    请在控制台前往【物联网平台】-》【AI服务】-》【模型配置】页面，为您的产品添加所需的 AI 模型。

5.  **`ERROR A stack overflow in task main has been detected` (ESP32)**
    运行 `idf.py menuconfig`，进入 `Component config -> ESP System Settings`，将 `Main task stack size`（主任务堆栈大小）增加到 `5000` 或更大值后重新编译。

### 火山引擎产品链接

- [IOT Platform](https://www.volcengine.com/docs/6893/1455924)
- [AI Gateway](https://www.volcengine.com/docs/6893/1263412)