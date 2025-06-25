// Copyright (2025) Beijing Volcano Engine Technology Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "onesdk.h"
#include "aigw/auth.h"
#include "aigw/llm.h"

static char global_sign_root_ca[] = "-----BEGIN CERTIFICATE-----\n"
"MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G\n"
"A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp\n"
"Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4\n"
"MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG\n"
"A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n"
"hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8\n"
"RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT\n"
"gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm\n"
"KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd\n"
"QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ\n"
"XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw\n"
"DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o\n"
"LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU\n"
"RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp\n"
"jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK\n"
"6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX\n"
"mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs\n"
"Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH\n"
"WD9f\n"
"-----END CERTIFICATE-----\n";

#define SAMPLE_HTTP_HOST "https://iot-cn-shanghai.iot.volces.com"
#define SAMPLE_INSTANCE_ID "***"
// #define SAMPLE_MQTT_HOST "***"
#define SAMPLE_DEVICE_NAME "***"
#define SAMPLE_DEVICE_SECRET "***"
#define SAMPLE_PRODUCT_KEY "***"
#define SAMPLE_PRODUCT_SECRET "***"

int global_err = 0;
// 流式回调函数
void stream_callback(const char *chat_data, size_t chat_data_len, void *user_data) {
    // printf("Stream data received: len = %d, data = \n%s\n", (int)chat_data_len, chat_data);
    // printf("stream_callback user_data = %p", user_data);
}

// 完成回调函数
void completed_callback(void *user_data) {
    printf("Chat completed.\n");
    // printf("completed_callback user_data = %p\n", user_data);
}

void error_callback(int code, const char *msg, void *user_data) {
    printf("Error occurred: code = %x, msg = %s\n", code, msg);
    printf("error_callback user_data = %p\n", user_data);
    global_err = code;
}

int main() {
    // 初始化请求数据
    // 定义请求消息
    chat_message_t messages[] = {
        {
            .role = "user",
            .content = "who are you?"
        }
    };
    // 构建请求结构体
    onesdk_chat_request_t request = {
        .model = "Doubao-1.5-pro-256k",
        .messages = messages,
        .messages_count = sizeof(messages) / sizeof(chat_message_t),
        .stream = true, // 可以切换为 false 测试非流式请求
        .temperature = .9,
    };
    iot_basic_config_t iot_cfg = {
        .http_host = SAMPLE_HTTP_HOST,
        .instance_id = SAMPLE_INSTANCE_ID,
        .product_key = SAMPLE_PRODUCT_KEY,
        .product_secret = SAMPLE_PRODUCT_SECRET,
        .device_name = SAMPLE_DEVICE_NAME,
        .auth_type = ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED,
        .verify_ssl = true,
        .ssl_ca_cert = digicert_global_root_g2,
    };
    iot_basic_ctx_t iot_ctx = {
        .config = &iot_cfg,
    };
    onesdk_chat_callbacks_t callbacks = {
        .onesdk_stream_cb = stream_callback,
        .onesdk_chat_completed_cb = completed_callback,
        .onesdk_error_cb = error_callback,
    };

    // // 非流式请求
    char output[2048] = {0};
    size_t output_len = sizeof(output);
    onesdk_config_t *config = malloc(sizeof(onesdk_config_t));
    if (NULL == config) {
        return VOLC_ERR_INIT;
    }
    memset(config, 0, sizeof(onesdk_config_t));
    config->device_config = &iot_cfg;
    // 初始化onesdk
    onesdk_ctx_t *onesdk_ctx = malloc(sizeof(onesdk_ctx_t));
    if (NULL == onesdk_ctx) {
        return VOLC_ERR_INIT;
    }
    int ret = onesdk_init(onesdk_ctx, config);
    if (ret != 0) {
        fprintf(stderr, "onesdk init failed with code %d\n", ret);
        goto failed;
        return ret;
    }
    // 必须在onesdk_init后再初始化回调
    onesdk_chat_set_callbacks(onesdk_ctx, &callbacks, NULL/*your app context*/);
    global_err = onesdk_chat(onesdk_ctx, &request, output, &output_len);

    if (global_err) {
        fprintf(stderr, "chat request failed with code1 %X\n", global_err);
        goto failed;
        return global_err;
    }

    // 等待异步请求完成（流式）
    global_err = onesdk_chat_wait(onesdk_ctx);
    if (global_err != 0) {
        fprintf(stderr, "Streaming request failed with code:%d. see previous output for more.\n", global_err);
        goto failed;
        return global_err;
    }

    printf("(chat_user)Full text: %s\n", output);

    printf("chat with image begin\n");
    // vision  chat
    chat_multi_content_t multi_contents[] = {
        {
            .type = "text",
            .text = "请帮我识别一下这张图片",
        },
        {
            .type = "image_url",
            .image_url = {
                .url = "https://vei-demo.tos-cn-beijing.volces.com/car.jpeg", // 也可以是图片的base64编码
                // .url = "data:image/jpeg;base64,/9j/4AAQSkZJRgABAQEAAAAAAAD/4QAuRXhpZgAATU0AKgAAAAgAAkAAA
                .detail = "auto"
            },
        }
    };
    chat_message_t messages_vision[] = {
        {
            .role = "user",
            .multi_contents = multi_contents,
            .multi_contents_count = sizeof(multi_contents) / sizeof(chat_multi_content_t),
        }
    };
    // 构建请求结构体
    onesdk_chat_request_t request_vision = {
        .model = "doubao-1.5-vision-pro-32k", //必须是支持多模态的模型
        .messages = messages_vision,
        .messages_count = sizeof(messages_vision) / sizeof(chat_message_t),
        .stream = true, // 多模态大模型支持true, 当前 ocr图像识别暂时不支持流式，必须为false
        .temperature = .7,
    };
    global_err = onesdk_chat(onesdk_ctx, &request_vision, output, &output_len);

    if (global_err) {
        fprintf(stderr, "chat request failed with code2 %X\n", global_err);
        goto failed;
        return global_err;
    }
    global_err = onesdk_chat_wait(onesdk_ctx);
    printf("(chat_image)Full text: %s\n", output);
    // 释放onesdk 上下文
    failed:
    onesdk_deinit(onesdk_ctx);
    free(onesdk_ctx);
    free(config);
    return global_err;
}
