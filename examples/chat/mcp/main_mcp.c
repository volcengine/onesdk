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
#include "locale.h"
#include "cJSON.h"
#include "onesdk_chat.h"
#include "mcp_client.h"

#include "libwebsockets.h"
#include "protocols/http.h"
#include "protocols/private_lws_http_client.h"

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
#define SAMPLE_MCP_SERVER_IP "10.37.51.21"
// #define SAMPLE_MCP_SERVER_IP "127.0.0.1"
#define SAMPLE_MCP_SERVER_PORT 8000

#define MAX_HISTORY 5  // 保留最近5轮对话
#define CHAT_BUFFER_SIZE 1024 

// 新增对话历史结构体
typedef struct {
    chat_message_t messages[MAX_HISTORY*2]; // 存储用户和AI的交替对话
    int count;  // 当前对话总数
} ChatHistory;


/**
 * @brief 当前支持单次请求多个function call的模型如下：其他模型建议用单个function
 * Doubao-1.5-pro-32k
 * Deepseek-chat
 */
static char *model = "Deepseek-chat";


void clear_history(ChatHistory *history){
        if (history->count >= MAX_HISTORY*2) {
        // 释放即将被覆盖的最旧两条消息
        for (int i = 0; i < 2; i++) {
            free((void*)history->messages[i].role);
            if(history->messages[i].content){
                free((void*)history->messages[i].content);
            }
            if(history->messages[i].tool_calls_count > 0){
                for(int j = 0; j < history->messages[i].tool_calls_count; j++){
                    free((void*)history->messages[i].tool_calls[j].id);
                    free((void*)history->messages[i].tool_calls[j].type);
                    free((void*)history->messages[i].tool_calls[j].function.name);
                    free((void*)history->messages[i].tool_calls[j].function.arguments);
                }
                free((void*)history->messages[i].tool_calls);
            }
        }
        // 移除最旧的一对问答
        memmove(&history->messages[0], &history->messages[2], 
               (history->count - 2) * sizeof(chat_message_t));
        history->count -= 2;
    }
}


// 新增历史记录管理函数
void add_user_content_to_history(ChatHistory *history, const char *content) {
    clear_history(history);
    
    // 使用strdup复制字符串
    history->messages[history->count] = (chat_message_t){
        .role = strdup("user"),
        .content = strdup(content)
    };
    history->count++;
}


void add_llm_response_to_history(ChatHistory *history, onesdk_chat_response_t *response) {
    clear_history(history);

    // 使用strdup复制字符串
    history->messages[history->count] = (chat_message_t){
        .role = strdup("assistant"),
    };
    if(response->choices[0].message.tool_calls_count == 0){
        history->messages[history->count].content = strdup(response->choices[0].message.content);
    }else{
        history->messages[history->count].tool_calls = (chat_tool_call_t *)malloc(response->choices[0].message.tool_calls_count * sizeof(chat_tool_call_t));
        for(int i = 0; i < response->choices[0].message.tool_calls_count; i++){
            history->messages[history->count].tool_calls[i].id = strdup(response->choices[0].message.tool_calls[i].id);
            history->messages[history->count].tool_calls[i].type = strdup("function");
            history->messages[history->count].tool_calls[i].function.name = strdup(response->choices[0].message.tool_calls[i].function.name);
            history->messages[history->count].tool_calls[i].function.arguments = strdup(response->choices[0].message.tool_calls[i].function.arguments);
        }
        history->messages[history->count].tool_calls_count = response->choices[0].message.tool_calls_count;
    }
    history->count++;
}


void add_tool_result_to_history(ChatHistory *history, char *tool_id, tool_result_t *result) {
    clear_history(history); 

    //获取content
    char *content = mcp_get_tool_result_text(result);
    
    // 使用strdup复制字符串
    history->messages[history->count] = (chat_message_t){
        .role = strdup("tool"),
        .content = strdup(content),
        .tool_call_id = strdup(tool_id)
    };
    history->count++;
    free(content);
}


void* mcp_receive_thread(void *arg){
    if(arg == NULL){
        return NULL;
    }
    mcp_context_t *ctx = (mcp_context_t *)arg;
    block_receive_from_sse_server(ctx);
}

pthread_t mcp_receiver_run(mcp_context_t *ctx){
    pthread_t tid;
    pthread_create(&tid, NULL, mcp_receive_thread, (void*)ctx);
    return tid;
}




int main(){
    setlocale(LC_CTYPE, "en_US.UTF-8"); // 明确指定UTF-8编码
    onesdk_config_t *config = NULL;
    onesdk_ctx_t *onesdk_ctx = NULL;
    ChatHistory history = {0};
    char input[CHAT_BUFFER_SIZE] = {0}; 

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
    // 非流式请求
    char output[2048] = {0};
    size_t output_len = sizeof(output);
    config = malloc(sizeof(onesdk_config_t));
    if (NULL == config) {
        goto failed;
    }
    memset(config, 0, sizeof(onesdk_config_t));
    config->device_config = &iot_cfg;
    // 初始化onesdk
    onesdk_ctx = malloc(sizeof(onesdk_ctx_t));
    if (NULL == onesdk_ctx) {
        goto failed;
    }
    memset(onesdk_ctx, 0, sizeof(onesdk_ctx_t));
    int ret = onesdk_init(onesdk_ctx, config);
    if (ret != 0) {
        fprintf(stderr, "onesdk init failed with code %d\n", ret);
        goto failed;
    }

    //启动sse接收线程
    mcp_context_t *ctx = mcp_ctx_new();
    mcp_receiver_init(ctx,SAMPLE_MCP_SERVER_IP, SAMPLE_MCP_SERVER_PORT);//mcp server 地址
    pthread_t tid = mcp_receiver_run(ctx);

    tools_list_t *tools_list = mcp_tools_list(ctx);
    if(!tools_list){
        fprintf(stderr, "mcp_tools_list failed");
        goto failed;
    }
    chat_tool_t *tools = (chat_tool_t *)malloc(sizeof(chat_tool_t) * tools_list->count);
    for (int i = 0; i < tools_list->count; i++) {
        tools[i].type = strdup("function");
        tools[i].function.name = strdup(tools_list->tools[i].name);
        tools[i].function.description = strdup(tools_list->tools[i].description);
        tools[i].function.parameters = strdup(tools_list->tools[i].input_schema_raw);
    }

    sprintf(input, "你好,麻烦帮我查下我的账号下有几个物联网实例");
    add_user_content_to_history(&history, input);

    onesdk_chat_response_t *response = NULL;
    onesdk_chat_set_functioncall_data(onesdk_ctx, &response);
    // 修改请求结构体初始化（在main函数中）
    onesdk_chat_request_t request = {
        .model = model,
        .tools = tools,                     // 新增工具数组
        .tools_count = tools_list->count,
        .tool_choice = "auto",              // 新增工具选择策略
        .stream = false,                    // 当前，function call本次调用，仅支持非流式
        .temperature = .9,
    };
    while (true)
    {
        request.messages = history.messages;
        request.messages_count = history.count;
        if(response){
            chat_free_response(response);
            response = NULL;
        }    
        output_len = sizeof(output);
        int result = onesdk_chat(onesdk_ctx, &request, output, &output_len);
        // TODO 获取函数调用参数，并进行本地函数调用
        if (result) {
            fprintf(stderr, "chat request failed with code5 %d\n", result);
            goto failed;
            return result;
        }

        // 等待异步请求完成（流式）
        result = onesdk_chat_wait(onesdk_ctx);
        if (result != 0) {
            fprintf(stderr, "function call request failed with code:%d. see previous output for more.\n", result);
            goto failed;
        }
        // 在第一次请求完成后添加处理逻辑
        if (result != 0 || response == NULL) {
            printf("response is null\n");
            goto failed;
        }

        if(response->choices[0].message.tool_calls_count == 0){
            printf("no tools call\n");
            break;
        }
        add_llm_response_to_history(&history, response);
        // 处理工具调用结果
        for (int i = 0; i < response->choices[0].message.tool_calls_count; i++) {
            chat_tool_call_t *tool_call = &response->choices[0].message.tool_calls[i];
            printf("Tool ID: %s\n", tool_call->id);
            printf("Tool Type: %s\n", tool_call->type);
            printf("Tool Function Name: %s\n", tool_call->function.name);
            printf("Tool Function Arguments: %s\n", tool_call->function.arguments);
            tool_result_t * tool_result = mcp_tool_call(ctx, tool_call->function.name, tool_call->function.arguments);;
            add_tool_result_to_history(&history, tool_call->id, tool_result);
        }
    }
    printf("assistant content %s\n", output);
    
    // 释放onesdk 上下文
failed:
    // 释放历史数据
    for (int i = 0; i < history.count; i++) {
        free((void*)history.messages[i].role);
        if(history.messages[i].content){
            free((void*)history.messages[i].content);
        }
        if(history.messages[i].tool_calls_count > 0){
            for(int j = 0; j < history.messages[i].tool_calls_count; j++){
                free((void*)history.messages[i].tool_calls[j].id);
                free((void*)history.messages[i].tool_calls[j].type);
                free((void*)history.messages[i].tool_calls[j].function.name);
                free((void*)history.messages[i].tool_calls[j].function.arguments);
            }
            free((void*)history.messages[i].tool_calls);
        }
    }
    if(response){
        chat_free_response(response);
    }
    if(onesdk_ctx){
        onesdk_deinit(onesdk_ctx);
    }
    if(config){
        free(config);
    }
    http_close(ctx->http_ctx);;
    pthread_join(tid, NULL);
    http_ctx_release(ctx->http_ctx);
    mcp_ctx_free(ctx);
    return 0;
}