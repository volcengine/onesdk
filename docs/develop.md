
English | [中文README](develop.zh_CN.md)
---

## Development Guide

This document provides a comprehensive guide for setting up the development environment, compiling the source code, and running the OneSDK examples.

### 1. Prerequisites

#### 1.1. Toolchain

Ensure the following tools are installed on your system:

- **CMake**: CMake 3.10 or above but not higher than 4.x
- **GCC/Clang**: A C/C++ compiler toolchain.
- **Git**: For cloning the repository.
- **OpenSSL or MbedTLS**: For TLS communication.
  - **OpenSSL (Recommended for Linux/macOS):**
    - macOS: `brew install openssl`
    - Ubuntu/Debian: `sudo apt-get install libssl-dev`
    - CentOS/RHEL: `sudo yum install openssl openssl-devel`
  - **MbedTLS (Recommended for Embedded Platforms):**
    - It is recommended to use MbedTLS v3.4.0 for better compatibility. Refer to the [official MbedTLS repository](https://github.com/Mbed-TLS/mbedtls) for installation instructions.

#### 1.2. Activate Volcengine Services

Before using the SDK, you need to activate the required products on the Volcengine console:

1.  **Activate "Edge Intelligence"**: Follow the [official guide](https://www.volcengine.com/docs/6893/1134368) to activate the service.
2.  **Activate "IoT Platform"**: Open the IoT Platform service and grant necessary permissions.
3.  **Obtain an Instance**: You can get an instance by contacting the product manager or initiating an on-call request through the Volcengine console.

### 2. Getting the Code

Clone the repository from GitHub:

```bash
git clone --recursive https://github.com/volcengine/onesdk.git
cd onesdk
```

### 3. Configure Connection Credentials

To run the examples, you need to obtain the following credentials from your Volcengine IoT Platform console and configure them in the example source code.

-   **IoT Management API Host**: `https://iot-cn-shanghai.iot.volces.com` (Fixed value)
-   **IoT MQTT Host**: `<instance-id>-cn-shanghai.iot.volces.com`
-   **Instance ID**: Your service instance ID.
-   **Product Key**: Required for all authentication types.
-   **Product Secret**: Required for "per-type" authentication.
-   **Device Name**: Required for all authentication types.
-   **Device Secret**: Required for "per-device" authentication.

*Note: For "per-type" dynamic registration (`ONESDK_AUTH_DYNAMIC_PRE_REGISTERED` or `ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED`), you must enable the "Enable Dynamic Registration" switch for your product in the console.*

### 4. Build and Run Examples

#### 4.1. Linux/macOS

1.  **Edit Example Parameters:**
    Open an example file, such as `examples/chat/text_image/main_text_image.c`, and fill in the credentials obtained in the previous step.

    ```c
    #define SAMPLE_HTTP_HOST "https://iot-cn-shanghai.iot.volces.com"
    #define SAMPLE_INSTANCE_ID "<your_instance_id>"
    #define SAMPLE_MQTT_HOST "<your_instance_id>.cn-shanghai.iot.volces.com"
    #define SAMPLE_PRODUCT_KEY "<your_product_key>"
    #define SAMPLE_DEVICE_NAME "<your_device_name>"
    
    // Use for per-device authentication
    #define SAMPLE_DEVICE_SECRET "<your_device_secret>" 
    
    // Use for per-type authentication
    #define SAMPLE_PRODUCT_SECRET "<your_product_secret>"
    ```

2.  **Build the Project:**
    Run the `build.sh` script from the root directory. This will compile the SDK and all examples.

    ```bash
    # Build with default options
    ./build.sh
    
    # Or, build without tests
    ./build.sh -DONESDK_WITH_TEST=OFF
    ```

3.  **Run the Example:**
    The compiled binaries are located in the `build/bin` directory.

    ```bash
    ./build/examples/chat/text_image/onesdk_chat_example
    ```

#### 4.2. Espressif ESP32

For ESP32 development, it is recommended to use the VSCode ESP-IDF extension or install ESP-IDF manually.

-   **VSCode Extension (Auto-install)**: [ESP-IDF VSCode Extension](https://github.com/espressif/vscode-esp-idf-extension)
-   **Manual Install**: [ESP-IDF Get Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html)
-   For audio examples, the **ESP-ADF** framework is also required: [ESP-ADF Get Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/index.html)

**To run an example:**
1.  Navigate to an ESP32 example directory, e.g., `examples/onesdk_esp32/`.
2.  Modify the connection parameters in the relevant source file (e.g., `main/chat.c`).
3.  Follow the instructions in the `README.md` file within that directory to configure, build, flash, and monitor the application.

### 5. Build Options

You can customize the build by passing CMake options to `build.sh`:
`./build.sh -D<OPTION_NAME>=<VALUE>`

| Option                      | Default | Description                                              |
| --------------------------- | ------- | -------------------------------------------------------- |
| `ONESDK_ENABLE_AI`          | `ON`    | Enable AI features (text, image).                        |
| `ONESDK_ENABLE_AI_REALTIME` | `ON`    | Enable real-time AI features (e.g., WebSocket audio).    |
| `ONESDK_ENABLE_IOT`         | `ON`    | Enable IoT features (MQTT, OTA, Thing Model).            |
| `ONESDK_WITH_EXAMPLE`       | `ON`    | Build example applications.                              |
| `ONESDK_WITH_TEST`          | `OFF`   | Build unit tests.                                        |
| `ONESDK_WITH_SHARED`        | `ON`    | Build shared libraries.                                  |
| `ONESDK_WITH_STATIC`        | `ON`    | Build static libraries.                                  |
| `ONESDK_WITH_STRICT_MODE`   | `OFF`   | Enable compiler address sanitizer.                       |

### 6. Troubleshooting

1.  **`Fatal error: 'openssl/ssl.h' file not found`**
    If CMake cannot find the TLS headers, you may need to specify the include path in the root `CMakeLists.txt`. For example, on Apple Silicon with Homebrew:
    `include_directories("/opt/homebrew/include")`

2.  **`SSL error: self-signed certificate in certificate chain`**
    This usually means the system's root CA certificates are not found or are incorrect.
    -   Verify connectivity and certificates with `curl https://iot-cn-shanghai.iot.volces.com`.
    -   If it fails, update your system's CA store:
        -   CentOS/RHEL: `sudo update-ca-trust`
        -   Ubuntu/Debian: `sudo update-ca-certificates`

3.  **`The dynamic register switch of product is not turned on`**
    For `per-type` authentication, ensure you have enabled "Dynamic Registration" for the product in the IoT console.

4.  **`No access to model xxxxxxx`**
    Go to `IoT Platform -> AI Services -> Model Configuration` in the console and add the required AI model to your product.

5.  **`ERROR A stack overflow in task main has been detected` (ESP32)**
    Run `idf.py menuconfig`, go to `Component config -> ESP System Settings`, and increase the `Main task stack size` to `5000` or more.


### Volocano Engine Product Links

- [IOT Platform](https://www.volcengine.com/docs/6893/1455924)
- [AI Gateway](https://www.volcengine.com/docs/6893/1263412)