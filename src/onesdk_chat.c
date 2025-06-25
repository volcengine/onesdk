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
#ifdef ONESDK_ENABLE_AI

#include "onesdk_chat.h"
#include "onesdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// user interfaces
int onesdk_chat(onesdk_ctx_t *ctx, onesdk_chat_request_t *request, char *output, size_t *output_len) {
    if (ctx->chat_ctx == NULL && ctx->chat_ctx->request_context == NULL) {
        fprintf(stderr, "[onesdk_chat]onesdk_chat_context_t is null, create chat context first\n");
        aigw_llm_config_t *llm_config = ctx->config->aigw_llm_config;
        char *endpoint = llm_config->url;
        char *api_key = llm_config->api_key;
        onesdk_chat_context_t *chat_ctx = onesdk_chat_context_init(
            endpoint, api_key/*,NULL, NULL, NULL*/,ctx);
        chat_ctx->iot_basic_ctx = ctx->iot_basic_ctx;
        ctx->chat_ctx = chat_ctx;
    }
    return onesdk_chat_send_inner(ctx->chat_ctx, request, output, output_len);
}
int onesdk_chat_wait(onesdk_ctx_t *ctx) {
    if (ctx->chat_ctx == NULL) {
        fprintf(stderr, "[onesdk_chat_wait]onesdk_chat_context_t is null, create chat context first\n");
        return -1;
    }
    return onesdk_chat_wait_inner(ctx->chat_ctx);

}
void onesdk_chat_release_context(onesdk_ctx_t *ctx) {
    if (ctx) {
        if (ctx->chat_ctx) {
            onesdk_chat_release_context_inner(ctx->chat_ctx);
            ctx->chat_ctx = NULL;
        }
        // free(ctx);
    }
}

void onesdk_chat_set_callbacks(onesdk_ctx_t *ctx, onesdk_chat_callbacks_t *cbs, void *user_data) {
    if (cbs != NULL && ctx->chat_ctx != NULL) {
        ctx->chat_ctx->callbacks = cbs;
        ctx->chat_ctx->user_data = user_data;
    } else {
        fprintf(stderr, "onesdk_chat_set_callbacks is not set\n");
    }

}

void onesdk_chat_set_functioncall_data(onesdk_ctx_t *ctx, onesdk_chat_response_t **response_out) {
    if (ctx->chat_ctx != NULL) {
        ctx->chat_ctx->_response_out = response_out;
    } else {
        fprintf(stderr, "onesdk_chat_set_function_call is not set\n");
    }
}

/* internal interfaces */

void _chat_internal_stream_callback(const chat_stream_response_t *partial_response, void *user_data) {
    onesdk_ctx_t *o_ctx = (onesdk_ctx_t *)user_data;
    if (!o_ctx) {
        fprintf(stderr, "onesdk_ctx_t is null");
        return;
    }
    onesdk_chat_context_t *ctx = (onesdk_chat_context_t *)o_ctx->chat_ctx;
    if (!ctx) {
        fprintf(stderr, "[_chat_internal_stream_callback]onesdk_chat_context_t is null");
        return;
    }
    if (!partial_response ) {
        fprintf(stderr, "partial_response is null");
        return;
    }

    if (partial_response->choices_count > 0) {
        // 处理流式响应的部分文本，可以将content渐进式输出到屏幕上
        if (partial_response->choices[0].delta.content != NULL) {
            // printf("Received partial text(len=%d): %s\n", strlen(partial_response->choices[0].delta.content), partial_response->choices[0].delta.content);
            // stream = true , content内容
            size_t len = strlen(partial_response->choices[0].delta.content);
            size_t max_copy_len = len;
            // printf("current len = %d, in-len =%zu \n", ctx->chat_data_current_len, len);
            if (ctx->chat_data_current_len + max_copy_len >= ctx->chat_data_len -1) {
                fprintf(stderr, "full text is too long, consider increase buffer size of full_text\n");
                max_copy_len = ctx->chat_data_len - ctx->chat_data_current_len;
            }
            memcpy(ctx->chat_data + ctx->chat_data_current_len, partial_response->choices[0].delta.content, max_copy_len);\
            ctx->chat_data_current_len += max_copy_len;
            ctx->chat_data[ctx->chat_data_current_len] = '\0'; // 添加字符串结束符，确保输出是一个完整的字符串，而不是一个字符数组[]
            onesdk_chat_callbacks_t *cbs = ctx->callbacks;
            
            if (cbs != NULL && cbs->onesdk_stream_cb != NULL) {
                // printf("call user stream callback:%s|\n", ctx->chat_data + ctx->chat_data_current_len - max_copy_len);
                cbs->onesdk_stream_cb(ctx->chat_data + ctx->chat_data_current_len - max_copy_len, max_copy_len, ctx->user_data);
            }
        } // 标准流式响应输出

        // 处理function call相关响应
        if (partial_response->choices[0].delta.tool_calls != NULL && partial_response->choices[0].delta.tool_calls_count > 0) {
            // printf("Received function call name: %s\n", partial_response->choices[0].delta.function_call->name);
            // 处理函数调用的名称
            for (size_t i = 0; i < partial_response->choices[0].delta.tool_calls_count; i++)
            {
                // TODO 对于不同模型，function call的结构可能不同，需要根据具体模型的要求进行解析
                //fprintf(stderr, "Received function call name: %s\n", partial_response->choices[0].delta.tool_calls[i].function.name);
                //fprintf(stderr, "Received function call arguments: %s\n", partial_response->choices[0].delta.tool_calls[i].function.arguments);
            }
        }
        if (partial_response->id != NULL && ctx->completion_id != NULL) {
            // printf("completion_id: len=(%d), id: %s\n", strlen(partial_response->id), partial_response->id);
            // memcpy(ctx->completion_id, partial_response->id, strlen(partial_response->id));
        }

    }
}

void _chat_internal_completed_callback(void *user_data) {
    if (!user_data) {
        fprintf(stderr, "[_chat_internal_completed_callback]user_data is null");
        return;
    }
    onesdk_ctx_t *o_ctx = (onesdk_ctx_t *)user_data;
    if (!o_ctx || !o_ctx->chat_ctx) {
        fprintf(stderr, "Invalid context in completion callback\n");
        return;
    }
    onesdk_chat_context_t *ctx = o_ctx->chat_ctx;
    onesdk_chat_callbacks_t *cbs = ctx ? ctx->callbacks : NULL;
    if (!ctx) {
        fprintf(stderr, "[_chat_internal_completed_callback]onesdk_chat_context_t is null");
        return;
    }
    if (cbs != NULL && cbs->onesdk_chat_completed_cb) {
        // printf("call onesdk_chat_completed_cb: data:|\n%s\n", ctx->chat_data);
        cbs->onesdk_chat_completed_cb(ctx->user_data);
    }
    ctx->is_completed = true;
}

void _chat_internal_error_callback(int error_code,const char *error_msg, void *user_data) {
    onesdk_ctx_t *o_ctx = (onesdk_ctx_t *)user_data;
    onesdk_chat_context_t *ctx = (onesdk_chat_context_t *)o_ctx->chat_ctx;
    if (!ctx) {
        fprintf(stderr, "[_chat_internal_error_callback]onesdk_chat_context_t is null, error %d, error_msg: %s\n", error_code, error_msg);
        return;
    }
    onesdk_chat_callbacks_t *cbs = ctx->callbacks;
    if (cbs!= NULL && cbs->onesdk_error_cb) {
        fprintf(stderr, "call onesdk_error_cb: err:|%s\n", error_msg);
        cbs->onesdk_error_cb(error_code, error_msg, ctx->user_data);
    }
}

/** 
 * @brief 创建上下文
 * 
 * @param request
 * @param stream_callback
 * @param completed_callback
 * @param user_data
 * @return onesdk_chat_context*
 */
onesdk_chat_context_t *onesdk_chat_context_init(const char *endpoint, const char *api_key, void *user_data) {
    onesdk_chat_context_t *ctx = (onesdk_chat_context_t *)malloc(sizeof(onesdk_chat_context_t));
    if (!ctx || !endpoint || !api_key) {
        fprintf(stderr, "invalid ctx or endpoint or api_key");
        return NULL;
    }
    memset(ctx, 0, sizeof(onesdk_chat_context_t));
    ctx->callbacks = NULL;
    ctx->user_data = NULL; // user handler
    ctx->_chat_stream_cb = _chat_internal_stream_callback;
    ctx->_chat_completed_cb = _chat_internal_completed_callback;
    ctx->_chat_error_cb = _chat_internal_error_callback;
    ctx->_chat_user_data = user_data;
    ctx->endpoint = (char *)endpoint;
    ctx->api_key = (char *)api_key;
    ctx->completion_id = NULL;
    ctx->is_streaming = false;
    ctx->is_completed = false;
    ctx->chat_data = NULL;
    ctx->chat_data_len = 0;
    ctx->chat_data_current_len = 0;
    ctx->_response_out = NULL;
    return ctx;
}

/**
 * @brief 发送请求
 *
 * @param ctx
 * 
 * @return int 0 成功，在同步请求时, 请配置output和output_len, 用于接收response
 */
int onesdk_chat_send_inner(onesdk_chat_context_t *ctx, onesdk_chat_request_t *request,char *output, size_t *output_len) {
    if (!ctx) {
        return -1;
    }
    // start chat 
    chat_request_t *chat_request = (chat_request_t *)request;
    chat_request_context_t *chat_request_ctx = chat_request_context_init(ctx->endpoint, ctx->api_key, chat_request, ctx->iot_basic_ctx); // 请替换为实际的 API 密钥
    if (!chat_request_ctx) {
        printf("create chat request context failed");
        return VOLC_ERR_HTTP_SEND_FAILED;
    }
    // 配置回调函数
    chat_request_ctx->on_chat_stream_cb = ctx->_chat_stream_cb;
    chat_request_ctx->on_chat_completed_cb = ctx->_chat_completed_cb;
    chat_request_ctx->on_chat_error_cb = ctx->_chat_error_cb;
    chat_request_ctx->user_data = ctx->_chat_user_data;
    // 
    ctx->request_context = chat_request_ctx;

    ctx->chat_data = output;
    ctx->chat_data_current_len = 0;
    ctx->chat_data_len = *output_len;
    int ret_code = VOLC_OK;
    ctx->is_streaming = request->stream;
    if (request->stream) {
        int ret = chat_send_stream_request(chat_request_ctx, ctx->_chat_stream_cb, ctx->_chat_completed_cb, ctx->_chat_error_cb, ctx->_chat_user_data);
        ret_code =  ret;
    } else {
        chat_response_t *response = chat_send_non_stream_request(chat_request_ctx);
        if (response) {
            // 处理非流式响应
            if (output && output_len) {
                if (response->choices_count > 0 && response->choices[0].message.content != NULL) {
                    strncpy(output, response->choices[0].message.content, *output_len -1);
                    int in_len = strlen(response->choices[0].message.content);
                    if (in_len >= *output_len) {
                        fprintf(stderr, "[warn]response is too long, trancated %d from %d", in_len - *output_len, in_len);
                        // output_len equal strlen(output)
                    }
                     else {
                        *output_len = in_len;
                    }
                    // output[*output_len-1] = '\0';
                }
                if (ctx->_response_out == NULL ) { // release response if not request out
                    printf("release response, due to not needed\n");
                    chat_free_response(response);
                } else {
                    // printf("_response_out, response : %p, %p, tool_calls = %p|", ctx->_response_out, &response);
                    *(ctx->_response_out) = response;
                }
            }
            ret_code = VOLC_OK;
        } else {
            fprintf(stderr, "chat_send_stream_request failed, ret = %d", -1);
            ret_code = -1;

        }
    }
    if (chat_request_ctx != NULL && !ctx->is_streaming) {
        // 同步请求，完成后立即释放上下文
        chat_release_request_context(chat_request_ctx);
        ctx->request_context = NULL;
        chat_request_ctx = NULL;
    }
    return ret_code;
}

/**
 * @brief 等待异步请求完成
 * 
 * @param ctx
 * @return int 0 成功，其他失败
 */
int onesdk_chat_wait_inner(onesdk_chat_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    int ret = VOLC_OK;
    if (ctx->is_streaming) {
        ret = chat_wait_complete(ctx->request_context);
        if (ctx->request_context) {
            chat_release_request_context(ctx->request_context);
            ctx->request_context = NULL;
        }
    }
    return ret;
}

int onesdk_chat_release_context_inner(onesdk_chat_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    if (ctx->request_context) {
        chat_release_request_context(ctx->request_context);
        ctx->request_context = NULL;
    }
    if (ctx->_response_out != NULL && *(ctx->_response_out) != NULL) {
        chat_free_response(*(ctx->_response_out));
        *(ctx->_response_out) = NULL;
    }
    if (ctx->chat_data) {
        
    }
    // if (ctx->callbacks) {
    //     free(ctx->callbacks);
    //     ctx->callbacks = NULL;
    // }
    if (ctx->completion_id) {
        free(ctx->completion_id);
        ctx->completion_id = NULL;
    }
    free(ctx);
    return VOLC_OK;
}

#endif // ONESDK_ENABLE_AI