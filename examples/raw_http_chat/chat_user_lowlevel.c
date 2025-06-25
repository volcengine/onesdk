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
#include "infer_inner_chat.h"

char full_text[2048] = {0};
int full_text_len = 0;
char response_id[200] = {0};

void stream_callback(const chat_stream_response_t *partial_response, void *user_data);
void complete_callback(void *user_data);
void error_callback(int error_code, char *msg, void *user_data);
void get_chat_completion(const char *api_key, const char *completion_id);


int main() {
    const char *api_key = "sk-ca07d1aa8dff4713b393746e0d6c18dehktu2mjmf2aqvv5a";
    // 定义请求消息
    chat_message_t messages[] = {
        {
            .role = "user",
            .content = "Write a quick sort in Golang!"
        }
    };
    // 构建请求结构体
    chat_request_t request = {
        .model = "Doubao-1.5-pro-32k",
        .messages = messages,
        .messages_count = sizeof(messages) / sizeof(chat_message_t),
        .stream = true, // 可以切换为 false 测试非流式请求
        .store = false,
    };
    // 创建请求上下文
    char *endpoint = "http://llm-gateway.vei.gtm.volcdns.com:30506";

    http_request_context_t *ctx = chat_request_context_init(endpoint, api_key, &request, NULL);
    if (!ctx) {
        fprintf(stderr, "Failed to create request context.\n");
        return 1;
    }
    if (request.stream) {// 发送流式请求
        onesdk_http_ctx_set_timeout_mil(ctx, 60 * 1000/* 60 seconds */);
        int result = chat_send_stream_request(ctx, stream_callback, complete_callback, error_callback);
        if (result != 0) {
            fprintf(stderr, "Failed to send stream request.\n");
        }
        if (wait_chat_complete(ctx) != 0) {
            // complete with error
        } else {
            // complete with success
            printf("complete chat with success\n");
        }
    } else { // 发送非流式请求
        // 设置自定义响应时间，防止出现读取超时
        onesdk_http_ctx_set_timeout_mil(ctx, 60 * 1000/* 60 seconds */);
        chat_response_t *response = chat_send_non_stream_request(ctx);
        if (response) {
            if (response->choices_count > 0) {
                printf("Response text: %s\n", response->choices[0].message.content);
                memcpy(response_id, response->id, strlen(response->id));
            }
            // 释放响应内存
            chat_free_response(response);
        } else {
            fprintf(stderr, "Failed to get response.\n");
        }
    }
    // 释放请求上下文
    chat_release_request_context(ctx);
    // get chat completion
    if (strlen(response_id) > 0) {
        printf("Response with id: %s, len = %zu\n", response_id, strlen(response_id));
        get_chat_completion(api_key, response_id);
    }

    return 0;
}


// 流式响应回调函数示例
void stream_callback(const chat_stream_response_t *partial_response, void *user_data) {
    if (partial_response->choices_count > 0) {
        // 处理流式响应的部分文本，可以将content渐进式输出到屏幕上
        // printf("Received partial text: %s\n", partial_response->choices[0].delta.content);

        if (partial_response->id != NULL) {
            memcpy(response_id, partial_response->id, strlen(partial_response->id));
        }
        // 将输出进行聚合
        if (partial_response->choices[0].delta.content == NULL) {
            return;
        }
        size_t len = strlen(partial_response->choices[0].delta.content);
        // printf("current len = %d, in len =%zu \n", full_text_len, len);
        if (full_text_len + len > sizeof(full_text)) {
            printf("full text is too long, consider increase buffer size of full_text\n");
            return;
        }
        memcpy(full_text + strlen(full_text), partial_response->choices[0].delta.content, len);
        full_text_len += len;
        // printf("full text: %s\n", full_text);
    }
}

void complete_callback(void *user_data) {
    // 异步请求完成后 处理完整的响应
    printf("Received complete response:\n");
    printf("full text: \n%s\n", full_text);
}

void error_callback(int error_code, char *msg, void *user_data) {
    // 处理错误
    printf("Received error: %d, %s\n", error_code, msg);
}

void get_chat_completion(const char *api_key, const char *completion_id) {
    chat_response_t *response = chat_get_chat_completion(api_key, completion_id);
    if (response) {
        if (response->choices_count > 0) {
            printf("get_chat_completion: %s\n", response->choices[0].message.content);
        }
    }
}
