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

#ifndef ONESDK_CHAT_H
#define ONESDK_CHAT_H

#ifdef ONESDK_ENABLE_AI
#include <stdint.h>

#include "infer_inner_chat.h"
#include "iot_basic.h"

typedef void (*onesdk_chat_stream_callback)(const char *chat_data, size_t chat_data_len, void *user_data);
typedef void (*onesdk_chat_completed_callback)(void *user_data);
typedef void (*onesdk_chat_error_callback)(int error_code, const char *error_msg, void *user_data);

typedef struct onesdk_chat_callbacks {
    onesdk_chat_stream_callback onesdk_stream_cb;
    onesdk_chat_completed_callback onesdk_chat_completed_cb;
    onesdk_chat_error_callback onesdk_error_cb;
}  onesdk_chat_callbacks_t;

typedef struct onesdk_chat_context{
    chat_request_context_t *request_context;
    // chat_request_t *request;
    char *endpoint;
    char *api_key;
    char *completion_id; // returned completion id

    // onesdk_chat_stream_callback onesdk_stream_cb;
    // onesdk_chat_completed_callback onesdk_chat_completed_cb;
    // onesdk_chat_error_callback onesdk_error_cb;
    onesdk_chat_callbacks_t *callbacks;
    void *user_data;
    // internals
    chat_stream_callback _chat_stream_cb;
    chat_completed_callback _chat_completed_cb;
    chat_error_callback _chat_error_cb;
    void *_chat_user_data;

    bool is_streaming;
    bool is_completed;
    char *chat_data;
    size_t chat_data_current_len;
    size_t chat_data_len;
    chat_response_t **_response_out;
    iot_basic_ctx_t *iot_basic_ctx;
} onesdk_chat_context_t ;
typedef  chat_request_t onesdk_chat_request_t;
typedef  chat_response_t onesdk_chat_response_t;

void _chat_internal_stream_callback(const chat_stream_response_t *partial_response, void *user_data);
void _chat_internal_completed_callback(void *user_data);
onesdk_chat_context_t *onesdk_chat_context_init(const char *endpoint, const char *api_key, void *user_data);
int onesdk_chat_send_inner(onesdk_chat_context_t *ctx, onesdk_chat_request_t *request,char *output, size_t *output_len);

int onesdk_chat_wait_inner(onesdk_chat_context_t *ctx) ;

int onesdk_chat_release_context_inner(onesdk_chat_context_t *ctx);

#endif // ONESDK_ENABLE_AI
#endif  // ONESDK_CHAT_H