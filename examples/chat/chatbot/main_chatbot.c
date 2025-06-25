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

#define ENABLE_AI

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "onesdk.h"
#include "locale.h"
#include <time.h>  // 在文件头部添加
static char *global_error_msg = NULL;

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

// 流式回调函数
#define MAX_HISTORY 5  // 保留最近5轮对话
#define CHAT_BUFFER_SIZE 40960

int global_err = 0;
// 新增对话历史结构体
typedef struct {
    chat_message_t messages[MAX_HISTORY*2]; // 存储用户和AI的交替对话
    int count;  // 当前对话总数
} ChatHistory;

// 修改后的流式回调
// 修改缓冲区处理逻辑
void stream_callback(const char *chat_data, size_t chat_data_len, void *user_data) {
    // 直接输出流式数据
    fwrite(chat_data, 1, chat_data_len, stdout);
    fflush(stdout);  // 确保立即输出
}

void completed_callback(void *user_data) {
    // printf("Chat completed.\n");
    // printf("completed_callback user_data = %p\n", user_data);
}

// 新增历史记录管理函数
void add_to_history(ChatHistory *history, const char *role, const char *content) {
    if (history->count >= MAX_HISTORY*2) {
        // 释放即将被覆盖的最旧两条消息
        for (int i = 0; i < 2; i++) {
            free((void*)history->messages[i].role);
            free((void*)history->messages[i].content);
        }
        // 移除最旧的一对问答
        memmove(&history->messages[0], &history->messages[2], 
               (history->count - 2) * sizeof(chat_message_t));
        history->count -= 2;
    }
    
    // 使用strdup复制字符串
    history->messages[history->count] = (chat_message_t){
        .role = strdup(role),
        .content = strdup(content)
    };
    history->count++;
}

void error_callback(int code, const char *msg, void *user_data) {
    printf("Error occurred: code = %d, msg = %s\n", code, msg);
    // 将msg内容拷贝到global_error_msg
    if (global_error_msg) {
        free(global_error_msg);
    }
    global_error_msg = strdup(msg);
    global_err = code;
    // printf("error_callback user_data = %p\n", user_data);
}

int main() {
    setlocale(LC_CTYPE, "en_US.UTF-8"); // 明确指定UTF-8编码
    ChatHistory history = {0};
    char input[CHAT_BUFFER_SIZE]; // 使用新宏名
    // 初始化请求数据
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
    memset(onesdk_ctx, 0, sizeof(onesdk_ctx_t));
    int ret = onesdk_init(onesdk_ctx, config);
    if (ret != 0) {
        fprintf(stderr, "onesdk init failed with code %d\n", ret);
        return ret;
    }
    // 必须在onesdk_init后再初始化回调
    onesdk_chat_set_callbacks(onesdk_ctx, &callbacks, NULL/*your app context*/);
    // 给出用户提示：
    printf("\n欢迎使用火山引擎-端智能OneSDK！\n");
    printf("输入 'exit' 以退出。\n");
    // 改为交互式循环
    while (1) {
        printf("\n[用户]: ");
        if (!fgets(input, CHAT_BUFFER_SIZE, stdin)) break; // 使用新宏名
        input[strcspn(input, "\n")] = 0; // 去除换行
        
        if (strcmp(input, "exit") == 0) break;

        // 添加用户消息到历史
        add_to_history(&history, "user", input);
        
        // 构建包含历史的请求
        onesdk_chat_request_t request = {
            .model = "Doubao-1.5-pro-256k",
            .messages = history.messages,
            .messages_count = history.count,
            .stream = true,
            .temperature = 0.7,
        };
        
        char output[CHAT_BUFFER_SIZE] = {0};
        size_t output_len = sizeof(output);
        
        printf("请求中...\n");
        int result = onesdk_chat(onesdk_ctx, &request, output, &output_len);
        if (result) {
            fprintf(stderr, "请求失败: %d\n", result);
            continue;
        }
        printf("[OneSDK-Chat]: ");
        // 等待并获取完整响应
        result = onesdk_chat_wait(onesdk_ctx);
        if (!result && output[0]) {
            add_to_history(&history, "assistant", output); // 添加AI回复到历史
        }
        if (result) {
            fprintf(stderr, "请求失败: %d, err_msg: %s\n", result, global_error_msg);
            continue;
        }
        printf("\n");
    }

    // 释放历史数据
    for (int i = 0; i < history.count; i++) {
        free((void*)history.messages[i].role);
        free((void*)history.messages[i].content);
    }
    // 释放onesdk 上下文
    onesdk_deinit(onesdk_ctx);
    // user side free
    if (global_error_msg) {
        free(global_error_msg);
    }
    free(config);
    return global_err;
}
