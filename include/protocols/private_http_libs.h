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

#ifndef  PRIVATE_HTTP_LIBS_H
#define  PRIVATE_HTTP_LIBS_H
// 每个 SSE 块的用户数据结构
struct sse_data {
    char id[128];        // 事件 ID
    char event[128];     // 事件类型
    char data[1024];     // 数据部分
};

// 全局缓冲区保存部分消息流，解析时用
#define SSE_BUFFER_SIZE 15360
typedef struct sse_context {
    char buffer[SSE_BUFFER_SIZE]; // 临时缓冲区
    size_t used_len;          // 当前缓冲区使用长度
    char *data; // point to the buffer
    size_t data_len;
    char *event; // not yet supported
    size_t event_len;
    char *id; // not yet supported
    size_t id_len;
} sse_context_t;

void parse_sse_message(sse_context_t *ctx, const char *message, size_t len);
void generate_user_agent(char *user_agent, size_t size);
void *onesdk_memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);

#define HEADER_AIGW_HARDWARE_ID "X-Hardware-Id"

#endif // PRIVATE_HTTP_LIBS_H
