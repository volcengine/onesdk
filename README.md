English | [中文README](README.zh_CN.md)

<h1 align="center">OneSDK: A Unified AI Access SDK for the Client-side</h1>

## Table of Contents

- [Table of Contents](#table-of-contents)
- [Introduction](#introduction)
- [Features](#features)
  - [Overall Architecture](#overall-architecture)
  - [Feature List](#feature-list)
- [Quick Start](#quick-start)
  - [Prerequisites](#prerequisites)
  - [Installation, Compilation, and Execution](#installation-compilation-and-execution)
- [Contribution](#contribution)
- [Code of Conduct](#code-of-conduct)
- [Security](#security)
- [License](#license)

## Introduction

OneSDK is an integrated development kit for AI applications on the client-side. It provides large language model (LLM) capabilities including voice conversations, text chats, and agents. It also offers device management, supporting interaction with IoT platforms via the MQTT protocol. The SDK is based on libwebsockets to implement protocols like HTTP, WebSocket, and MQTT. It runs on embedded platforms such as Espressif ESP32, FreeRTOS, and uc-OS2, as well as general-purpose platforms like Linux, macOS, and Windows.

## Features

### Overall Architecture
![alt text](images/functions.png)

### Feature List
- **Device Intelligence**
  - **Text Chat**: Supports large model text chat (both streaming and non-streaming).
  - **Image Generation**: Supports large model image recognition and text-to-image generation.
  - **Voice Chat Agent**: Supports various voice chat agents, integrated with Realtime (WebSocket) API.
- **Device Operations**
  - **OTA Upgrade Support**
    - Full package upgrade
    - Differential package upgrade
  - **Device Management**
    - **Token Quota**: Check current quota usage from the cloud console.
  - **SSH**: Supports SSH connections from the console.
  - **Logging**: Supports uploading device operation logs.
- **Device Security**
  - Supports device-side identity authentication with "one key per device" and "one key per product type".
  - Supports device certificates.

## Quick Start

### Prerequisites

Supported Platforms:
- Espressif ESP32
- RTOS (FreeRTOS/uc-OS2)
- Linux (amd64/arm64)
- macOS (Apple Silicon/Intel)
- Windows

### Installation, Compilation, and Execution

Refer to the [Development Guide](docs/develop.md) for instructions on building, compiling, and running.

## Contribution

Please check [Contributing](CONTRIBUTING.md) for more details.

## Code of Conduct

Please check [Code of Conduct](CODE_OF_CONDUCT.md) for more details.

## Security

If you discover a potential security issue in this project, or think you may
have discovered a security issue, we ask that you notify Bytedance Security via our [security center](https://security.bytedance.com/src) or [vulnerability reporting email](sec@bytedance.com).

Please do **not** create a public GitHub issue.

## License

This project is licensed under the [Apache-2.0 License](LICENSE.txt).