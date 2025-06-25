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

#include "onesdk_config.h"
#ifdef ONESDK_ENABLE_AI_REALTIME

#include "infer_realtime_ws.h"

#include "cJSON.h"
#include "onesdk.h"
#include "error_code.h"

void rt_recv_cb(const char* message, size_t len, void* userdata) {
    onesdk_ctx_t *ctx = userdata;

    // 添加缓冲区初始化和安全检查
    if (!ctx->rt_json_buf) { // 首次使用初始化缓冲区
        ctx->rt_json_buf_capacity = len * 2;
        ctx->rt_json_buf = malloc(ctx->rt_json_buf_capacity);
        ctx->rt_json_buf_size = 0;
    }
    else if (ctx->rt_json_buf_capacity < ctx->rt_json_buf_size + len) {
        // 添加溢出检查
        if (len > SIZE_MAX / 2 || ctx->rt_json_buf_size > SIZE_MAX - len) {
            // 处理内存溢出错误
            fprintf(stderr, "Memory allocation size overflow");
            return;
        }
        ctx->rt_json_buf_capacity = (ctx->rt_json_buf_size + len) * 2;
        char* new_buf = realloc(ctx->rt_json_buf, ctx->rt_json_buf_capacity);
        if (!new_buf) {
            fprintf(stderr, "Memory reallocation failed");
            return;
        }
        ctx->rt_json_buf = new_buf;
    }
    memcpy(ctx->rt_json_buf + ctx->rt_json_buf_size, message, len);
    ctx->rt_json_buf_size += len;

    // 尝试解析完整JSON
    // printf("尝试解析的JSON数据: %.*s\n", (int)ctx->rt_json_buf_size, ctx->rt_json_buf);
    cJSON *root = cJSON_ParseWithLengthOpts(ctx->rt_json_buf, ctx->rt_json_buf_size, (const char **)&(ctx->rt_json_pos), 0);

    while (root) {
        cJSON *type_item = cJSON_GetObjectItem(root, "type");
        cJSON *event_id = cJSON_GetObjectItem(root, "event_id");
        cJSON *delta = cJSON_GetObjectItem(root, "delta");
        cJSON *transcript = cJSON_GetObjectItem(root, "transcript");

        if (type_item && cJSON_IsString(type_item) && event_id) {
            const char *type = type_item->valuestring;

            if (ctx->rt_event_cb) {
                if (strcmp(type, "response.audio.delta") == 0 && delta) {
                    // 音频数据回调
                    if (ctx->rt_event_cb->on_audio) {
                        ctx->rt_event_cb->on_audio(
                            delta->valuestring,
                            cJSON_IsString(delta) ? strlen(delta->valuestring) : 0,
                            ctx->user_data
                        );
                    }
                } else if (strcmp(type, "response.audio_transcript.done") == 0 && transcript) {
                    // 文本转录完成回调
                    if (ctx->rt_event_cb->on_text) {
                        ctx->rt_event_cb->on_text(
                            transcript->valuestring,
                            cJSON_IsString(transcript)? strlen(transcript->valuestring) : 0,
                            ctx->user_data
                        );
                    }
                } else if (strcmp(type, "response.audio_translation.delta") == 0 && delta) {
                    if (ctx->rt_event_cb->on_translation_text) {
                        ctx->rt_event_cb->on_translation_text(
                            delta->valuestring,
                            cJSON_IsString(delta)? strlen(delta->valuestring) : 0,
                            ctx->user_data
                        );
                    }
                } else if (strcmp(type, "response.audio_transcript.delta") == 0 && delta) {
                    if (ctx->rt_event_cb->on_transcript_text) {
                        ctx->rt_event_cb->on_transcript_text(
                            delta->valuestring,
                            cJSON_IsString(delta)? strlen(delta->valuestring) : 0,
                            ctx->user_data
                        );
                    }
                } else if (strcmp(type, "response.done") == 0) {
                    if (ctx->rt_event_cb->on_response_done) {
                        ctx->rt_event_cb->on_response_done(ctx->user_data);
                    }
                } else if (strcmp(type, "error") == 0) {
                    // 错误回调
                    cJSON *error = cJSON_GetObjectItem(root, "error");
                    cJSON *error_code = cJSON_GetObjectItem(error, "code");
                    cJSON *error_message = cJSON_GetObjectItem(error, "message");
                    if (error_code && cJSON_IsString(error_code) && error_message && cJSON_IsString(error_message)) {
                        if (ctx->rt_event_cb->on_error) {
                            ctx->rt_event_cb->on_error(
                                error_code->valuestring,
                                error_message->valuestring,
                                ctx->user_data
                            );
                        }
                    }
                    // TODO：重新建立session
                }
            }
        }

        // 处理剩余数据
        size_t processed = ctx->rt_json_pos - ctx->rt_json_buf;
        ctx->rt_json_buf_size -= processed;
        memmove(ctx->rt_json_buf, ctx->rt_json_buf + processed, ctx->rt_json_buf_size);

        cJSON_Delete(root);
        root = cJSON_ParseWithLengthOpts(ctx->rt_json_buf, ctx->rt_json_buf_size, (const char **)&(ctx->rt_json_pos), 0);
    }
}


int onesdk_rt_set_event_cb(onesdk_ctx_t *ctx, onesdk_rt_event_cb_t *cb) {
    int ret = VOLC_OK;
    if (NULL == ctx || NULL == ctx->aigw_ws_ctx) {
        return VOLC_ERR_INIT;
    }
    ctx->aigw_ws_ctx->callback = rt_recv_cb;
    ctx->aigw_ws_ctx->userdata = ctx;
    ctx->rt_event_cb = cb;
    return ret;
}

int onesdk_rt_session_update(onesdk_ctx_t *ctx, aigw_ws_session_t *session) {
    int ret = VOLC_OK;
    if (NULL == ctx || NULL == ctx->aigw_ws_ctx) {
        return VOLC_ERR_INIT;
    }
    return aigw_ws_session_update(ctx->aigw_ws_ctx, session);
}

int onesdk_rt_session_keepalive(onesdk_ctx_t *ctx) {
    int ret = VOLC_OK;
    if (NULL == ctx || NULL == ctx->aigw_ws_ctx) {
        return VOLC_ERR_INIT;
    }
    int n = 0;
    while (n >= 0) {
        n = aigw_ws_run_event_loop(ctx->aigw_ws_ctx, 0);
    }

    return ret;
}

int onesdk_rt_audio_send(onesdk_ctx_t *ctx, const char *audio_data, size_t len, bool commit) {
    int ret = VOLC_OK;
    if (NULL == ctx || NULL == ctx->aigw_ws_ctx) {
        return VOLC_ERR_INIT;
    }
    ret = aigw_ws_input_audio_buffer_append(ctx->aigw_ws_ctx, audio_data, len);
    if (ret!= VOLC_OK) {
        return ret;
    }
    if (commit) {
        ret = aigw_ws_input_audio_buffer_commit(ctx->aigw_ws_ctx);
        if (ret!= VOLC_OK) {
            return ret;
        }
        ret = aigw_ws_response_create(ctx->aigw_ws_ctx);
    }
    return ret;
}

int onesdk_rt_audio_response_cancel(onesdk_ctx_t *ctx) {
    int ret = VOLC_OK;
    if (NULL == ctx || NULL == ctx->aigw_ws_ctx) {
        return VOLC_ERR_INIT;
    }
    return aigw_ws_response_cancel(ctx->aigw_ws_ctx);
}

int onesdk_rt_translation_session_update(onesdk_ctx_t *ctx, aigw_ws_translation_session_t *session) {
    int ret = VOLC_OK;
    if (NULL == ctx || NULL == ctx->aigw_ws_ctx) {
        return VOLC_ERR_INIT;
    }
    return aigw_ws_translation_session_update(ctx->aigw_ws_ctx, session);
}

int onesdk_rt_translation_audio_send(onesdk_ctx_t *ctx, const char *audio_data, size_t len, bool commit) {
    int ret = VOLC_OK;
    if (NULL == ctx || NULL == ctx->aigw_ws_ctx) {
        return VOLC_ERR_INIT;
    }
    ret = aigw_ws_input_audio_buffer_append(ctx->aigw_ws_ctx, audio_data, len);
    if (ret!= VOLC_OK) {
        return ret;
    }
    if (commit) {
        ret = aigw_ws_input_audio_buffer_commit(ctx->aigw_ws_ctx);
        if (ret!= VOLC_OK) {
            return ret;
        }
        ret = aigw_ws_input_audio_done(ctx->aigw_ws_ctx);
    }
    return ret;
}
#endif // ONESDK_ENABLE_AI_REALTIME