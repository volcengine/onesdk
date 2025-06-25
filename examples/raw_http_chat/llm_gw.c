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

#include "libwebsockets.h"
#include "protocols/http.h"
#include <stdio.h>

#include "onesdk_hello_world.h"

char buf[2048] = {0};
size_t buf_len = 0;
void user_show_char(sse_context_t *ctx, bool last_chunk, void *cb_user_data) {

    if (ctx != NULL ) {
        ChatCompletionChunk *chunk = (ChatCompletionChunk *)malloc(sizeof(ChatCompletionChunk));
        char tmp[1024] = {0};
        memcpy(tmp, ctx->data, ctx->data_len);
        printf("data to parse: >>>>>\n%s\n", tmp);

        int r = parse_json(tmp, chunk);
        if (r != 0) {
            printf("Failed to parse json\n");
            return;
        }
        if (chunk != NULL) {
            size_t len = strlen(chunk->delta_content);
            memcpy(buf + buf_len, chunk->delta_content, len);
            buf_len += len;
        }
    }
}
void user_print_chart(void *cb_user_data) {
        printf("FINAL result: %s\n", buf);
}

int llm_gw_text(bool stream) {
    // 创建一个新的 HTTP 请求上下文
    http_request_context_t *http_ctx = new_http_ctx();
    if (http_ctx == NULL) {
        printf("Failed to create HTTP context\n");
        return 1;
    }

    // 设置请求的 URL
    http_ctx_set_url(http_ctx, "http://llm-gateway.vei.gtm.volcdns.com:30506/v1/chat/completions");
    // http_ctx_set_url(http_ctx, "http://127.0.0.1:8080/hello");
    // 设置请求的方法为 POST
    http_ctx_set_method(http_ctx, HTTP_POST);

    // 设置请求头
    http_ctx_add_header(http_ctx, "Content-Type", "application/json");
    // http_ctx_add_header(http_ctx, "Authorization", "Bearer sk-2ed0eeb73dbc489ab189360bc9f49fbfa2yo0yd3m72o390s");
    http_ctx_set_bearer_token(http_ctx, "Bearer sk-2ed0eeb73dbc489ab189360bc9f49fbfa2yo0yd3m72o390s");
    if (stream) {
        http_ctx_add_header(http_ctx, "Accept", "text/event-stream");
        http_ctx_set_on_get_sse_cb(http_ctx, user_show_char, http_ctx);
        http_ctx_set_on_complete_cb(http_ctx, user_print_chart, http_ctx);
    }
    // 设置请求体
    const char *request_body = "{\"model\":\"doubao-1.5-vision-pro-32k\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"What's in this image?\"},{\"type\":\"image_url\",\"image_url\":{\"url\":\"https://vei-demo.tos-cn-beijing.volces.com/car.jpeg\"}}]}],\"max_tokens\":300,\"stream\":true}";

    if (!stream) {
        request_body = "{\"model\":\"doubao-1.5-vision-pro-32k\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"What's in this image?\"},{\"type\":\"image_url\",\"image_url\":{\"url\":\"https://vei-demo.tos-cn-beijing.volces.com/car.jpeg\"}}]}],\"max_tokens\":300}";

    }

    http_ctx_set_json_body(http_ctx, (char *)request_body);
    printf("ca crt: %s\n", http_ctx->ca_crt);
    // 发送请求
    http_response_t *response = http_request(http_ctx);
    if (response != NULL) {
        // 打印响应体
        printf("Response body:\n%s\n", response->response_body);

        // 打印响应码
        printf("Response code: %d\n", http_ctx->client->response_code);

        // 释放响应资源
        // onesdk_http_response_release(response);
    } else {
        printf("Failed to send request\n");
    }

    // 释放 HTTP 上下文资源
    http_ctx_release(http_ctx);

    return 0;
}

