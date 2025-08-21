## 概述
onesdk 支持使用预编译的外部库（如 OpenSSL 和 libwebsockets）进行构建。**当前仅支持静态库（.a 文件）**，不支持动态库（.so/.dylib 文件）。通过设置 `ONESDK_USE_PREBUILT_LIBS` 开关，可以启用此功能并指定预编译库的路径。

## 4. 配置构建选项
### 4.1 启用预构建库支持
通过CMake选项启用预构建库功能：
```bash
./build.sh -DONESDK_USE_PREBUILT_LIBS=ON
```

### 4.2 可选配置项
| 配置选项 | 说明 | 默认值 |
|---------|------|--------|
| ONESDK_WITH_SHARED | 构建动态库 | ON |
| ONESDK_WITH_STATIC | 构建静态库 | ON |
| ONESDK_WITH_EXAMPLE | 构建示例程序 | ON |

## 5. 完整构建步骤

1. 确保预构建库已正确放置在 `onesdk/libs` 目录下

```bash
onesdk/
└── libs/
    ├── include/
    │   ├── lws_config_private.h  # libwebsockets私有头文件（必选，建议根据libwebsockets版本和开关替换）
    │   ├── lws_config.h          # libwebsockets公开文件（必选，建议根据libwebsockets版本和开关替换）
    │   └── openssl/              # OpenSSL头文件目录(可选，一般在系统目录下)
    │       ├── ssl.h
    │       └── crypto.h
    ├── libwebsockets.a       # libwebsockets静态库（必选）
    ├── libssl.a              # OpenSSL ssl库（可选，根据是否需要TLS功能选择）
    └── libcrypto.a           # OpenSSL crypto库（可选，根据是否需要TLS功能选择）
```

2. 执行构建脚本

```bash
./build.sh -DONESDK_USE_PREBUILT_LIBS=ON
```

## 6. 验证构建结果
构建成功后，可在以下路径找到输出库文件：
- 静态库: `build/libonesdk.a`

## 7. 故障排除
### 7.1 头文件找不到
**错误示例**：`fatal error: 'libwebsockets.h' file not found`
**解决方法**：
1. 确认预构建库的头文件已放置在 `libs/include` 目录
2. 检查CMake配置是否正确启用 `ONESDK_USE_PREBUILT_LIBS`

### 7.2 库文件找不到
**错误示例**：`ld: library not found for -lwebsockets`
**解决方法**：
1. 确认预构建库文件已放置在 `libs/lib` 目录
2. 检查库文件命名是否符合系统标准（如Linux下为`libwebsockets.a`)

## 8. 注意事项
1. **库版本兼容性**：预构建库需与onesdk兼容，建议使用与源码编译版本相同的库
2. **平台一致性**：确保预构建库与目标平台匹配（如x86_64 vs arm64）
3. **静态库链接顺序**：链接时需注意库的依赖顺序，通常OpenSSL应在libwebsockets之后

## 目录结构要求
预编译库需按照以下结构放置在 `onesdk/libs` 目录中：