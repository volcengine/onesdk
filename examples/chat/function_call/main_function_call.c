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

int global_err = 0;
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
// #define SAMPLE_AIGW_ENDPOINT "https://ai-gateway.vei.volces.com"

/**
 * @brief 当前支持单次请求多个function call的模型如下：其他模型建议用单个function
 * Doubao-1.5-pro-32k
 * Deepseek-chat
 */
static char *model = "Deepseek-chat";
// 流式回调函数
void stream_callback(const char *chat_data, size_t chat_data_len, void *user_data) {
    //printf("Stream data received: len = %d, data = \n%s\n", (int)chat_data_len, chat_data);
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

// 新增本地函数1: 获取天气
char* get_weather(const char *location, const char *unit, char *out, int out_size) {
    snprintf(out, out_size, 
        "{\"temperature\": 22, \"unit\": \"%s\", \"description\": \"晴朗\", \"location\": \"%s\"}",
        unit ? unit : "celsius", location);
    return out;
}

// 新增本地函数2: 获取最高山峰
char* get_highest_mountain(const char *country, char *out, int out_size) {
    const char* mountain = "未知";
    int height = 0;
    
    if (strcmp(country, "中国") == 0) {
        mountain = "珠穆朗玛峰";
        height = 8848;
    } else if (strcmp(country, "美国") == 0) {
        mountain = "德纳利山"; 
        height = 6190;
    } else if (strcmp(country, "日本") == 0) {
        mountain = "富士山";
        height = 3776;
    }
    
    snprintf(out, out_size,
        "{\"name\": \"%s\", \"height\": %d, \"unit\": \"米\", \"country\": \"%s\"}",
        mountain, height, country);
    return out;
}

// 修改handle_tool_calls函数
static void handle_tool_calls(onesdk_ctx_t *onesdk_ctx, chat_message_t *messages, int message_count, onesdk_chat_response_t *response) {
    if (response == NULL || response->choices_count == 0) return;



    chat_tool_call_t *tool_calls = response->choices[0].message.tool_calls;
    int tool_calls_count = response->choices[0].message.tool_calls_count;
    // 构造新的message长度并初始化
    int new_message_count = message_count + tool_calls_count + 1;
    printf("handle_tool_calls: tool_calls_count = %d, new_message_count= %d\n", tool_calls_count, new_message_count);
    chat_message_t *new_messages = (chat_message_t *)malloc(sizeof(chat_message_t) * new_message_count);
    memset(new_messages, 0, sizeof(chat_message_t) * new_message_count);
    // 复制原始消息到新数组
    memcpy(new_messages, messages, sizeof(chat_message_t) * message_count);
    // 将大模型输出添加到消息数组中
    chat_message_t *assistant_message = new_messages + message_count;
    memcpy(assistant_message, &(response->choices[0].message), sizeof(chat_message_t));

    // 初始化tool_messages数组
    chat_message_t *tool_messages = assistant_message + 1;
    for (int i = 0; i < tool_calls_count; i++) {
        chat_tool_call_t *call = &tool_calls[i];
        cJSON *args = cJSON_Parse(call->function.arguments);
        if (!args) continue;
        chat_message_t *tool_message = &tool_messages[i];
        tool_message->role = "tool";
        char *result = (char *)malloc(2048);
        memset(result, 0, sizeof(2048));
        if (strcmp(call->function.name, "get_weather") == 0) {
            const char *location = cJSON_GetStringValue(cJSON_GetObjectItem(args, "location"));
            const char *unit = cJSON_GetStringValue(cJSON_GetObjectItem(args, "unit"));
            // 申请buffer
            tool_message->content = get_weather(location, unit, result, 2048);
            tool_message->tool_call_id = call->id;
        } 
        else if (strcmp(call->function.name, "get_highest_mountain") == 0) {
            const char *country = cJSON_GetStringValue(cJSON_GetObjectItem(args, "country"));
            tool_message->content = get_highest_mountain(country, result, 2048);
            tool_message->tool_call_id = call->id;
        }
        cJSON_Delete(args);
    }

    // 构造第二次请求
    onesdk_chat_request_t second_request = {
        .model = model,
        .messages = new_messages,
        .messages_count = new_message_count,
        .stream = true,
        .temperature = .9,
    };

    char second_output[2048] = {0};
    size_t second_output_len = sizeof(second_output);
    
    global_err = onesdk_chat(onesdk_ctx, &second_request, second_output, &second_output_len);
    if (global_err == 0) {
        onesdk_chat_wait(onesdk_ctx);
        printf("Final response: |%s|\n", second_output);
    }
    // 释放内存
    // Free allocated memory
    for (int i = message_count; i < new_message_count; i++) {
        if (new_messages[i].content) {
            free((void *)(new_messages[i].content));
        }
    }
    free(new_messages);
}

int main() {
    // 初始化请求数据
    // 定义请求消息

    
    // 修改消息数组
    chat_message_t messages[] = {
        {
            .role = "user",
            .content = "告诉我北京的天气和中国的最高山峰"
        }
    };
    
// 修改工具定义数组
// 注意：不通模型对functioncall的支持程度不同，目前仅有deepseek-chat支持单次调用多个functioncall的能力
// 对于其他模型，请将下面的模型改数组删掉一个
static chat_tool_t tools[] = {
    {
        .type = "function",
        .function = {
            .name = "get_weather",
            .description = "获取指定地区的当前天气",
            .parameters = "{\"type\": \"object\", \"properties\": {\"location\": {\"type\": \"string\", \"description\": \"城市或地区\"},\"unit\": {\"type\": \"string\", \"enum\": [\"celsius\", \"fahrenheit\"]}}}",
            .required = "[\"location\"]"
        }
    },
    {
        .type = "function",
        .function = {
            .name = "get_highest_mountain",
            .description = "获取指定国家的最高山峰",
            .parameters = "{\"type\": \"object\", \"properties\": {\"country\": {\"type\": \"string\", \"description\": \"国家名称\"}}}",
            .required = "[\"country\"]"
        }
    }
};
    // 修改请求结构体初始化（在main函数中）
    onesdk_chat_request_t request = {
        .model = model,
        .messages = messages,
        .messages_count = sizeof(messages) / sizeof(chat_message_t),
        .tools = tools,                     // 新增工具数组
        .tools_count = sizeof(tools) / sizeof(chat_tool_t),
        .tool_choice = "auto",              // 新增工具选择策略
        .stream = false,                    // 当前，function call本次调用，仅支持非流式
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
    memset(onesdk_ctx, 0, sizeof(onesdk_ctx_t));
    int ret = onesdk_init(onesdk_ctx, config);
    if (ret != 0) {
        fprintf(stderr, "onesdk init failed with code %d\n", ret);
        goto failed;
    }
    // 必须在onesdk_init后再初始化回调
    // if (request.stream) {
        onesdk_chat_set_callbacks(onesdk_ctx, &callbacks, NULL/*your app context*/);
    // }
    onesdk_chat_response_t *response = NULL;
    if (request.tools) {
        // 对于支持function call的请求，需要设置返回参数
        // printf("response_out : %p|\n", &response);
        onesdk_chat_set_functioncall_data(onesdk_ctx, &response);
    }
    int result = onesdk_chat(onesdk_ctx, &request, output, &output_len);
    // TODO 获取函数调用参数，并进行本地函数调用
    if (result) {
        fprintf(stderr, "chat request failed with code5 %d\n", result);
        goto failed;
        return result;
    }
    printf("(chat_user)function call text: %s\n", output);

    // 等待异步请求完成（流式）
    result = onesdk_chat_wait(onesdk_ctx);
    if (result != 0) {
        fprintf(stderr, "function call request failed with code:%d. see previous output for more.\n", result);
        goto failed;
    }
    // 在第一次请求完成后添加处理逻辑
    if (result == 0 && response != NULL) {
        printf("Received function call response count\n");
        handle_tool_calls(onesdk_ctx, request.messages, request.messages_count, response);
    }
    // 释放onesdk 上下文
    failed:
    onesdk_deinit(onesdk_ctx);
    free(onesdk_ctx);
    free(config);
    return result;
}