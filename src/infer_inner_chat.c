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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "plat/platform.h"
#include "protocols/http.h"
#include "infer_inner_chat.h"
#include "aigw/auth.h"

static char *chat_completions_path = "/v1/chat/completions";

// 创建请求上下文
chat_request_context_t *chat_request_context_init(const char* endpoint, const char *api_key, const chat_request_t *request, iot_basic_ctx_t *iot_basic_ctx) {
    http_request_context_t *http_ctx = new_http_ctx();
    if (!http_ctx) {
        printf("new http ctx failed\n");
        return NULL;
    }
    char full_path[256];

    if (endpoint == NULL || strlen(endpoint) <= 0) {
        printf("endpoint is null\n");
        return NULL;
    }
    snprintf(full_path, sizeof(full_path), "%s%s", endpoint, chat_completions_path);
    http_ctx_set_url(http_ctx, full_path);
    http_ctx_set_timeout_mil(http_ctx, ONESDK_INFER_DEFAULT_CHAT_TIMEOUT); // 60 seconds
    http_ctx_set_method(http_ctx, HTTP_POST);

    // 构建请求体
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", request->model);
    cJSON *messages_array = cJSON_CreateArray();
    for (int i = 0; i < request->messages_count; i++) {
        cJSON *message_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(message_obj, "role", request->messages[i].role);
        // 处理多模态内容
        if (request->messages[i].multi_contents) {
            // 遍历多模态内容数组
            cJSON *content_array = cJSON_CreateArray();
            // 处理多模态内容数组
            for (int j = 0; j < request->messages[i].multi_contents_count; j++) {
                chat_multi_content_t *content = &request->messages[i].multi_contents[j];
                cJSON *content_item = cJSON_CreateObject();
                cJSON_AddStringToObject(content_item, "type", content->type);
                if (strcmp(content->type, "image_url") == 0) {
                    cJSON *image_url_obj = cJSON_CreateObject();
                    cJSON_AddStringToObject(image_url_obj, "url", content->image_url.url);
                    if (content->image_url.detail) {
                        cJSON_AddStringToObject(image_url_obj, "detail", content->image_url.detail);
                    }
                    cJSON_AddItemToObject(content_item, "image_url", image_url_obj);
                } else if (strcmp(content->type, "text") == 0) {
                    cJSON_AddStringToObject(content_item, "text", content->text);
                } 
                cJSON_AddItemToArray(content_array, content_item);
            }
            // char *content_array_str = cJSON_Print(content_array);
            cJSON_AddItemToObject(message_obj, "content", content_array);
        } else {
            cJSON_AddStringToObject(message_obj, "content", request->messages[i].content);
        }
        // 添加tool_calls(大模型返回的部分再次发送给大模型)
        if (request->messages[i].tool_calls) {
            cJSON *tool_calls_array = cJSON_CreateArray();
            for (int j = 0; j < request->messages[i].tool_calls_count; j++) {
                cJSON *tool_call_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(tool_call_obj, "id", request->messages[i].tool_calls[j].id);
                cJSON_AddStringToObject(tool_call_obj, "type", request->messages[i].tool_calls[j].type);
                cJSON *functionObj = cJSON_CreateObject();
                cJSON_AddStringToObject(functionObj, "name", request->messages[i].tool_calls[j].function.name);
                cJSON_AddStringToObject(functionObj, "arguments", request->messages[i].tool_calls[j].function.arguments);
                // 添加可选的description字段
                cJSON_AddItemToObject(tool_call_obj, "function", functionObj);
                cJSON_AddItemToArray(tool_calls_array, tool_call_obj);
            }
            cJSON_AddItemToObject(message_obj, "tool_calls", tool_calls_array);
        }
        // 添加tool_call_id
        if (request->messages[i].tool_call_id) {
            cJSON_AddStringToObject(message_obj, "tool_call_id", request->messages[i].tool_call_id);
        }
        cJSON_AddItemToArray(messages_array, message_obj);
    }
    cJSON_AddItemToObject(root, "messages", messages_array);
    cJSON_AddBoolToObject(root, "store", request->store);
    if (request->tools_count > 0 && request->stream) {
        // function call当前有行为差异，强制stream=false
        fprintf(stderr, "function call not support stream, must be stream=false\n");
        return NULL;
    }
    cJSON_AddBoolToObject(root, "stream", request->stream);

    // 新增max_completion_tokens字段
    if (request->max_completion_tokens > 0) {
        cJSON_AddNumberToObject(root, "max_tokens", request->max_completion_tokens);
    }
    // 处理字符串类型的temperature参数
    if (request->temperature > 0) {
        cJSON_AddNumberToObject(root, "temperature", request->temperature);
    }
    // 处理字符串类型的top_p参数
    if (request->top_p > 0 ) {
        cJSON_AddNumberToObject(root, "top_p", request->top_p);
    }
    // 添加tools字段
    if (request->tools_count > 0) {
        cJSON *tools_array = cJSON_CreateArray();
        for (int i = 0; i < request->tools_count; i++) {
            cJSON *tool_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(tool_obj, "type", request->tools[i].type);
            
            cJSON *function_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(function_obj, "name", request->tools[i].function.name);
            
            // 添加可选的description字段
            if (request->tools[i].function.description) {
                cJSON_AddStringToObject(function_obj, "description", request->tools[i].function.description);
            }
            
            // 解析参数JSON字符串为对象
            cJSON *params = cJSON_Parse(request->tools[i].function.parameters);
            if (params) {
                cJSON_AddItemToObject(function_obj, "parameters", params);
            }
            if (request->tools[i].function.required) {
                cJSON_AddStringToObject(function_obj, "required", request->tools[i].function.required);
            }
            
            cJSON_AddItemToObject(tool_obj, "function", function_obj);
            cJSON_AddItemToArray(tools_array, tool_obj);
        }
        cJSON_AddItemToObject(root, "tools", tools_array);
    }

    // 处理tool_choice字段
    if (request->tool_choice) {
        // 支持字符串或对象格式
        cJSON *tool_choice = cJSON_Parse(request->tool_choice);
        if (tool_choice) {
            cJSON_AddItemToObject(root, "tool_choice", tool_choice);
        } else {
            cJSON_AddStringToObject(root, "tool_choice", request->tool_choice);
        }
    }
    char *request_body = cJSON_Print(root);
    cJSON_Delete(root);
    http_ctx_set_json_body(http_ctx, request_body);
    // 设置请求头
    http_ctx_add_header(http_ctx, "Content-Type", "application/json");
    char auth_header[256];
    memset(auth_header, 0, sizeof(auth_header));
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", api_key);
    http_ctx_set_bearer_token(http_ctx, auth_header);
    // add HEADER_AIGW_HARDWARE_ID
    if (iot_basic_ctx) {
        // printf("inject iot ralated header\n");
        http_ctx = device_auth_client(http_ctx, iot_basic_ctx);
    }
    if (iot_basic_ctx->config != NULL && iot_basic_ctx->config->ssl_ca_cert != NULL) {
        http_ctx_set_ca_crt(http_ctx, iot_basic_ctx->config->ssl_ca_cert);
    }
    if (iot_basic_ctx->config != NULL) {
        http_ctx_set_verify_ssl(http_ctx, iot_basic_ctx->config->verify_ssl);
    }
    free(request_body);
    chat_request_context_t *chat_ret_ctx = malloc(sizeof(chat_request_context_t));
    chat_ret_ctx->http_ctx = http_ctx;
    chat_ret_ctx->endpoint = strdup(endpoint);
    chat_ret_ctx->api_key = strdup(api_key);
    chat_ret_ctx->completion_id = NULL;
    chat_ret_ctx->on_chat_stream_cb = NULL;
    chat_ret_ctx->on_chat_completed_cb = NULL;
    chat_ret_ctx->on_chat_error_cb = NULL;
    return chat_ret_ctx;
}


// 解析单个选择项
chat_choice_t parse_choice(cJSON *choice_json) {
    chat_choice_t choice = {0};
    cJSON *index = cJSON_GetObjectItem(choice_json, "index");
    if (cJSON_IsNumber(index)) {
        choice.index = index->valueint;
    }
    cJSON *message_obj = cJSON_GetObjectItem(choice_json, "message");
    if (cJSON_IsObject(message_obj)) {
        cJSON *role = cJSON_GetObjectItem(message_obj, "role");
        if (cJSON_IsString(role)) {
            choice.message.role = strdup(role->valuestring);
        }
        cJSON *content = cJSON_GetObjectItem(message_obj, "content");
        if (cJSON_IsString(content)) {
            choice.message.content = strdup(content->valuestring);
        }
    }
    cJSON *finish_reason = cJSON_GetObjectItem(choice_json, "finish_reason");
    if (cJSON_IsString(finish_reason)) {
        choice.finish_reason = strdup(finish_reason->valuestring);
    }

    // 增强tool_calls解析
    cJSON *tool_calls = cJSON_GetObjectItem(message_obj, "tool_calls");
    if (tool_calls && cJSON_IsArray(tool_calls)) {
        int tool_calls_count = cJSON_GetArraySize(tool_calls);
        choice.message.tool_calls = malloc(tool_calls_count * sizeof(chat_tool_call_t));
        choice.message.tool_calls_count = tool_calls_count;

        for (int i = 0; i < tool_calls_count; i++) {
            cJSON *call = cJSON_GetArrayItem(tool_calls, i);
            chat_tool_call_t *tc = &choice.message.tool_calls[i];
            memset(tc, 0, sizeof(chat_tool_call_t));
            // 解析id和type
            tc->id = strdup(cJSON_GetObjectItem(call, "id")->valuestring);
            tc->type = strdup(cJSON_GetObjectItem(call, "type")->valuestring);
            
            // 解析function对象
            cJSON *func = cJSON_GetObjectItem(call, "function");
            if (cJSON_IsObject(func)) {
                tc->function.name = strdup(cJSON_GetObjectItem(func, "name")->valuestring);
                tc->function.arguments = strdup(cJSON_GetObjectItem(func, "arguments")->valuestring); 
            }
            // printf("DELETE choice: id: %s, type: %s, function.name: %s, function.arguments: %s\n", tc->id, tc->type, tc->function.name, tc->function.arguments);
        }
    }

    // 保持function_call解析
    cJSON *function_call = cJSON_GetObjectItem(message_obj, "function_call");
    if (function_call && cJSON_IsObject(function_call)) {
        choice.message.function_call = cJSON_PrintUnformatted(function_call);
    }
    return choice;
}

// 解析用量结构体
chat_usage parse_usage(cJSON *usage_json) {
    chat_usage usage = {0};
    cJSON *prompt_tokens = cJSON_GetObjectItem(usage_json, "prompt_tokens");
    if (cJSON_IsNumber(prompt_tokens)) {
        usage.prompt_tokens = prompt_tokens->valueint;
    }
    cJSON *completion_tokens = cJSON_GetObjectItem(usage_json, "completion_tokens");
    if (cJSON_IsNumber(completion_tokens)) {
        usage.completion_tokens = completion_tokens->valueint;
    }
    cJSON *total_tokens = cJSON_GetObjectItem(usage_json, "total_tokens");
    if (cJSON_IsNumber(total_tokens)) {
        usage.total_tokens = total_tokens->valueint;
    }
    cJSON *completion_details_obj = cJSON_GetObjectItem(usage_json, "completion_tokens_details");
    if (cJSON_IsObject(completion_details_obj)) {
        cJSON *reasoning_tokens = cJSON_GetObjectItem(completion_details_obj, "reasoning_tokens");
        if (cJSON_IsNumber(reasoning_tokens)) {
            usage.completion_tokens_details.reasoning_tokens = reasoning_tokens->valueint;
        }
        cJSON *accepted_prediction_tokens = cJSON_GetObjectItem(completion_details_obj, "accepted_prediction_tokens");
        if (cJSON_IsNumber(accepted_prediction_tokens)) {
            usage.completion_tokens_details.accepted_prediction_tokens = accepted_prediction_tokens->valueint;
        }
        cJSON *rejected_prediction_tokens = cJSON_GetObjectItem(completion_details_obj, "rejected_prediction_tokens");
        if (cJSON_IsNumber(rejected_prediction_tokens)) {
            usage.completion_tokens_details.rejected_prediction_tokens = rejected_prediction_tokens->valueint;
        }
    }
    return usage;
}

// 解析响应 JSON（非流式chat）
chat_response_t *parse_response(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        return NULL;
    }

    chat_response_t *response = (chat_response_t *)malloc(sizeof(chat_response_t));
    if (!response) {
        cJSON_Delete(root);
        return NULL;
    }
    memset(response, 0, sizeof(chat_response_t));

    cJSON *id = cJSON_GetObjectItem(root, "id");
    if (cJSON_IsString(id)) {
        response->id = strdup(id->valuestring);
    }
    cJSON *object = cJSON_GetObjectItem(root, "object");
    if (cJSON_IsString(object)) {
        response->object = strdup(object->valuestring);
    }
    cJSON *created = cJSON_GetObjectItem(root, "created");
    if (cJSON_IsNumber(created)) {
        response->created = created->valuedouble;
    }
    cJSON *choices = cJSON_GetObjectItem(root, "choices");
    if (cJSON_IsArray(choices)) {
        int choices_count = cJSON_GetArraySize(choices);
        response->choices_count = choices_count;
        response->choices = (chat_choice_t *)malloc(choices_count * sizeof(chat_choice_t));
        for (int i = 0; i < choices_count; i++) {
            cJSON *choice_json = cJSON_GetArrayItem(choices, i);
            response->choices[i] = parse_choice(choice_json);
        }
    }
    cJSON *model = cJSON_GetObjectItem(root, "model");
    if (cJSON_IsString(model)) {
        response->model = strdup(model->valuestring);
    }
    cJSON *system_fingerprint = cJSON_GetObjectItem(root, "system_fingerprint");
    if (cJSON_IsString(system_fingerprint)) {
        response->system_fingerprint = strdup(system_fingerprint->valuestring);
    }

    cJSON *usage = cJSON_GetObjectItem(root, "usage");
    if (cJSON_IsObject(usage)) {
        response->usage = parse_usage(usage);
    }

    // 新增字段解析
    cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
    if (cJSON_IsString(request_id)) {
        response->request_id = strdup(request_id->valuestring);
    }
    cJSON *tool_choice = cJSON_GetObjectItem(root, "tool_choice");
    if (cJSON_IsNull(tool_choice)) {
        response->tool_choice = NULL;
    }
    cJSON *seed = cJSON_GetObjectItem(root, "seed");
    if (cJSON_IsNumber(seed)) {
        response->seed = seed->valuedouble;
    }
    cJSON *top_p = cJSON_GetObjectItem(root, "top_p");
    if (cJSON_IsNumber(top_p)) {
        response->top_p = top_p->valuedouble;
    }
    cJSON *temperature = cJSON_GetObjectItem(root, "temperature");
    if (cJSON_IsNumber(temperature)) {
        response->temperature = temperature->valuedouble;
    }
    cJSON *presence_penalty = cJSON_GetObjectItem(root, "presence_penalty");
    if (cJSON_IsNumber(presence_penalty)) {
        response->presence_penalty = presence_penalty->valuedouble;
    }
    cJSON *frequency_penalty = cJSON_GetObjectItem(root, "frequency_penalty");
    if (cJSON_IsNumber(frequency_penalty)) {
        response->frequency_penalty = frequency_penalty->valuedouble;
    }
    cJSON *input_user = cJSON_GetObjectItem(root, "input_user");
    if (cJSON_IsNull(input_user)) {
        response->input_user = NULL;
    }
    cJSON *service_tier = cJSON_GetObjectItem(root, "service_tier");
    if (cJSON_IsString(service_tier)) {
        response->service_tier = strdup(service_tier->valuestring);
    }

    cJSON *metadata = cJSON_GetObjectItem(root, "metadata");
    if (cJSON_IsObject(metadata)) {
        response->metadata = metadata; // 这里简单处理，实际可能需要更复杂的解析
    }
    cJSON *response_format = cJSON_GetObjectItem(root, "response_format");
    if (cJSON_IsNull(response_format)) {
        response->response_format = NULL;
    }

    cJSON_Delete(root);
    return response;
}

// 新增接口：获取聊天完成结果
chat_response_t *chat_get_chat_completion(const char *api_key, const char *completion_id) {
    printf("get chat with id: %s\n", completion_id);
    if (!api_key || !completion_id) {
        return NULL;
    }
    http_request_context_t *ctx = new_http_ctx();
    if (!ctx) {
        return NULL;
    }

    char full_path[256] = {0};
    snprintf(full_path, sizeof(full_path), "http://llm-gateway.vei.gtm.volcdns.com:30506%s/%s", chat_completions_path, completion_id);
    http_ctx_set_url(ctx, full_path);
    http_ctx_set_method(ctx, HTTP_GET);
    // 设置请求头
    http_ctx_add_header(ctx, "Content-Type", "application/json");
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", api_key);
    char *tmp1 = (char*) malloc(strlen(auth_header) + 1);
    memcpy(tmp1, auth_header, strlen(auth_header) + 1);
    http_ctx_set_bearer_token(ctx, tmp1);

    http_response_t *http_response = http_request(ctx);
    if (!http_response) {
        http_ctx_release(ctx);
        return NULL;
    }

    chat_response_t *response = parse_response(http_response->response_body);
    // onesdk_http_response_release(http_response);
    http_ctx_release(ctx);

    return response;
}
// 流式结构体解析


// 解析 ContentFilterResult 结构体
content_filter_result_t parse_content_filter_result(cJSON *result_json) {
    content_filter_result_t result = {0};
    cJSON *filtered = cJSON_GetObjectItem(result_json, "filtered");
    if (cJSON_IsBool(filtered)) {
        result.filtered = filtered->valueint;
    }
    return result;
}

// 解析 content_filter_results_t 结构体
content_filter_results_t parse_content_filter_results(cJSON *results_json) {
    content_filter_results_t results = {0};
    cJSON *hate = cJSON_GetObjectItem(results_json, "hate");
    if (cJSON_IsObject(hate)) {
        results.hate = parse_content_filter_result(hate);
    }
    cJSON *self_harm = cJSON_GetObjectItem(results_json, "self_harm");
    if (cJSON_IsObject(self_harm)) {
        results.self_harm = parse_content_filter_result(self_harm);
    }
    cJSON *sexual = cJSON_GetObjectItem(results_json, "sexual");
    if (cJSON_IsObject(sexual)) {
        results.sexual = parse_content_filter_result(sexual);
    }
    cJSON *violence = cJSON_GetObjectItem(results_json, "violence");
    if (cJSON_IsObject(violence)) {
        results.violence = parse_content_filter_result(violence);
    }
    return results;
}

// 解析 Delta 结构体
chat_stream_delta_t parse_delta(cJSON *delta_json) {
    // printf("parse delta\n");
    chat_stream_delta_t delta = {0};
    cJSON *role = cJSON_GetObjectItem(delta_json, "role");
    if (cJSON_IsString(role)) {
        delta.role = strdup(role->valuestring);
    }
    cJSON *content = cJSON_GetObjectItem(delta_json, "content");
    if (cJSON_IsString(content)) {
        delta.content = strdup(content->valuestring);
    }
    cJSON *tool_calls = cJSON_GetObjectItem(delta_json, "tool_calls");
    if (cJSON_IsArray(tool_calls)) {
        int tool_calls_count = cJSON_GetArraySize(tool_calls);
        delta.tool_calls_count = tool_calls_count;
        delta.tool_calls = (chat_tool_call_t *)malloc(tool_calls_count * sizeof(chat_tool_call_t));
        for (int i = 0; i < tool_calls_count; i++) {
            cJSON *call = cJSON_GetArrayItem(tool_calls, i);
            chat_tool_call_t *tc = &delta.tool_calls[i];
            memset(tc, 0, sizeof(chat_tool_call_t));
            // 解析 id, type, function, arguments
            tc->id = strdup(cJSON_GetObjectItem(call, "id")->valuestring); // call_xxxxxxxxx
            tc->type = strdup(cJSON_GetObjectItem(call, "type")->valuestring);
            cJSON *function = cJSON_GetObjectItem(call, "function");
            if (cJSON_IsObject(function)) {
                // 解析 function 对象
                cJSON *name = cJSON_GetObjectItem(function, "name");
                cJSON *arguments = cJSON_GetObjectItem(function, "arguments");
                if (cJSON_IsString(name)) {
                    tc->function.name = strdup(name->valuestring);
                }
                if (cJSON_IsString(arguments)) {
                    tc->function.arguments = strdup(arguments->valuestring);
                }
            }
            printf("DELETE delta: id: %s, type: %s, function.name: %s, function.arguments: %s\n", tc->id, tc->type, tc->function.name, tc->function.arguments);
        }
    }
    return delta;
}

// 解析单个选择项
chat_stream_choice_t parse_stream_choice(cJSON *choice_json) {
    chat_stream_choice_t choice = {0};
    cJSON *index = cJSON_GetObjectItem(choice_json, "index");
    if (cJSON_IsNumber(index)) {
        choice.index = index->valueint;
    }
    cJSON *delta = cJSON_GetObjectItem(choice_json, "delta");
    if (cJSON_IsObject(delta)) {
        choice.delta = parse_delta(delta);
    }
    cJSON *finish_reason = cJSON_GetObjectItem(choice_json, "finish_reason");
    if (cJSON_IsString(finish_reason)) {
        choice.finish_reason = strdup(finish_reason->valuestring);
    }
    cJSON *content_filter_results_t = cJSON_GetObjectItem(choice_json, "content_filter_results");
    if (cJSON_IsObject(content_filter_results_t)) {
        choice.content_filter_results = parse_content_filter_results(content_filter_results_t);
    }
    return choice;
}

// 解析流式响应 JSON
chat_stream_response_t *parse_stream_response(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        return NULL;
    }

    chat_stream_response_t *response = (chat_stream_response_t *)malloc(sizeof(chat_stream_response_t));
    if (!response) {
        cJSON_Delete(root);
        return NULL;
    }
    memset(response, 0, sizeof(chat_stream_response_t));

    cJSON *id = cJSON_GetObjectItem(root, "id");
    if (cJSON_IsString(id)) {
        response->id = strdup(id->valuestring);
    }
    cJSON *object = cJSON_GetObjectItem(root, "object");
    if (cJSON_IsString(object)) {
        response->object = strdup(object->valuestring);
    }
    cJSON *created = cJSON_GetObjectItem(root, "created");
    if (cJSON_IsNumber(created)) {
        response->created = created->valueint;
    }
    cJSON *model = cJSON_GetObjectItem(root, "model");
    if (cJSON_IsString(model)) {
        response->model = strdup(model->valuestring);
    }
    cJSON *choices = cJSON_GetObjectItem(root, "choices");

    if (cJSON_IsArray(choices)) {
        int choices_count = cJSON_GetArraySize(choices);
        // printf("parse_stream_response count= %d\n", choices_count);

        response->choices_count = choices_count;
        response->choices = (chat_stream_choice_t *)malloc(choices_count * sizeof(chat_stream_choice_t));
        for (int i = 0; i < choices_count; i++) {
            cJSON *choice_json = cJSON_GetArrayItem(choices, i);
            response->choices[i] = parse_stream_choice(choice_json);
        }
    }
    cJSON *system_fingerprint = cJSON_GetObjectItem(root, "system_fingerprint");
    if (cJSON_IsString(system_fingerprint)) {
        response->system_fingerprint = strdup(system_fingerprint->valuestring);
    }

    cJSON_Delete(root);
    return response;
}


// 内部的 SSE 回调函数，用于处理流式响应
static void infer_internal_sse_callback(sse_context_t *sse_ctx, bool is_last_chunk, void *cb_user_data) {
    chat_request_context_t *ctx = (chat_request_context_t *)cb_user_data;
    if (ctx && ctx->on_chat_stream_cb) {
        // 这里需要根据 sse_ctx 解析出部分响应数据
        // 假设 sse_ctx 中的数据是 JSON 格式的 OpenAI 部分响应
        // char *json_str = sse_ctx->data; // 假设数据存储在 sse_ctx->data 中
        // char buf[1024] = {0};
        // memset(buf, 0, sizeof(buf));
        // memcpy(buf, json_str, strlen(json_str));
        // printf("infer_internal_sse_callback %s\n", buf);
        chat_stream_response_t *partial_response = parse_stream_response(sse_ctx->data);
        if (partial_response && ctx->on_chat_stream_cb) {
            // printf("partial_response->choices_count = %d, %p\n", partial_response->choices_count,ctx);
            ctx->on_chat_stream_cb(partial_response, ctx->user_data);
        }
        if (partial_response) {
            chat_free_stream_response(partial_response);
        }
    }
}


// 流式响应完成的回调
static void infer_internal_complete_callback(void *cb_user_data) {
    chat_request_context_t *ctx = (chat_request_context_t *)cb_user_data;
    if (ctx) {
        ctx->on_chat_completed_cb(ctx->user_data);
    }

}

static void infer_internal_error_callback(int error_code, char *msg, void *user_data) {
    chat_request_context_t *ctx = (chat_request_context_t *)user_data;
    printf("infer_internal_error_callback user_data = %p,ctx = %p\n",user_data, ctx);
    if (ctx && ctx->on_chat_error_cb) {
        ctx->on_chat_error_cb(error_code, msg, ctx->user_data);
    }
}

// 发送非流式请求
chat_response_t *chat_send_non_stream_request(chat_request_context_t *ctx) {
    http_request_context_t *http_ctx = ctx->http_ctx;
    http_ctx_set_on_complete_cb(http_ctx, infer_internal_complete_callback, ctx); // 全部结束时回调
    http_ctx_set_on_get_sse_cb(http_ctx, infer_internal_sse_callback, ctx); //每个sse回调
    http_ctx_set_on_error_cb(http_ctx, infer_internal_error_callback, ctx);
    http_response_t *http_response = http_request(http_ctx);
    if (!http_response) {
        return NULL;
    }
    chat_response_t *response = parse_response(http_response->response_body);
    // _http_response_release(http_ctx->response);
    return response;
}

// 发送流式请求
// 如有需要，请调用wait_for_complete()等待请求完成
int chat_send_stream_request(chat_request_context_t *ctx, chat_stream_callback stream_callback, chat_completed_callback completed_callback, chat_error_callback error_callback, void *user_data) {
    if (ctx == NULL) {
        return -1;
    }
    http_request_context_t *http_ctx = ctx->http_ctx;
    if (http_ctx == NULL) {
        return -1;
    }
    ctx->on_chat_stream_cb = stream_callback;
    ctx->on_chat_completed_cb = completed_callback;
    ctx->on_chat_error_cb = error_callback;
    ctx->user_data = user_data;
    http_ctx_set_on_complete_cb(http_ctx, infer_internal_complete_callback, ctx); // 全部结束时回调
    http_ctx_set_on_get_sse_cb(http_ctx, infer_internal_sse_callback, ctx); //每个sse回调
    http_ctx_set_on_error_cb(http_ctx, infer_internal_error_callback, ctx);
    return http_request_async(http_ctx);
}

int chat_wait_complete(chat_request_context_t *ctx) {
    if (ctx == NULL) {
        return -1;
    }
    http_request_context_t *http_ctx = ctx->http_ctx;
    if (http_ctx == NULL) {
        return -1;
    }
    http_wait_complete(http_ctx);
    int http_code = -1;
    if (http_ctx->response != NULL) {
        http_code = http_ctx->response->error_code;
    }
    if (http_code >0 && http_code < 300 ) {
        return 0;
    }
    return http_code;
}


// 释放响应结构体内存
void chat_free_response(chat_response_t *response) {
    if (!response) {
        return;
    }
    // 释放 id 内存
    if (response->id) {
        free((void *)response->id);
    }
    // 释放 object 内存
    if (response->object) {
        free((void *)response->object);
    }
    // 释放 choices 数组元素内存
    for (int i = 0; i < response->choices_count; i++) {
        // if (response->choices[i].text) {
        //     free((void *)response->choices[i].text);
        // }
        if (response->choices == NULL ) {
            continue;
        }
        if (response->choices[i].finish_reason) {
            free((void *)response->choices[i].finish_reason);
        }
            // 释放tool_calls结构体
        chat_message_t *msg = &response->choices[i].message;
        if (msg->tool_calls) {
            for (int j = 0; j < msg->tool_calls_count; j++) {
                chat_tool_call_t *tc = &msg->tool_calls[j];
                if (tc->id) free(tc->id);
                if (tc->type) free(tc->type);
                if (tc->function.name) {
                    free(tc->function.name);
                }
                if (tc->function.arguments) {
                    free(tc->function.arguments);
                }
                // 移除 free(tc) 因为 tc 是指向数组元素的指针，不是单独分配的
            }
            // 改为释放整个 tool_calls 数组
            free(msg->tool_calls);
            free(msg->tool_call_id);
            free((void *)msg->role);
        }
    }
    // 释放 choices 数组内存
    if (response->choices) {
        free((void *)response->choices);
    }
    // 释放 model 内存
    if (response->model) {
        free((void *)response->model);
    }
    // 释放 system_fingerprint 内存
    if (response->system_fingerprint) {
        free((void *)response->system_fingerprint);
    }
    // 释放tool_choice
    if (response->tool_choice) {
        free(response->tool_choice);
    }
    // 释放 response 自身内存
    free((void *)response);
}



// 释放 chat_stream_delta_t结构体
void chat_free_stream_delta(chat_stream_delta_t*delta) {
    if (delta) {
        free((char *)delta->role);
        free((char *)delta->content);
    }
}

// 释放 chat_stream_choice_t 结构体
void chat_free_stream_choice(chat_stream_choice_t *choice) {
    if (choice) {
        chat_free_stream_delta(&choice->delta);
        free((char *)choice->finish_reason);
    }
}

// 释放 chat_stream_response_t 结构体
void chat_free_stream_response(chat_stream_response_t *response) {
    if (response) {
        free(response->id);
        free(response->object);
        free(response->model);
        free(response->system_fingerprint);
        for (int i = 0; i < response->choices_count; i++) {
            chat_free_stream_choice(&response->choices[i]);
        }
        free(response->choices);
        free(response);
    }
}
// 释放请求上下文
void chat_release_request_context(chat_request_context_t *ctx) {
    if (ctx) {
        if (ctx->http_ctx) {
            http_ctx_release(ctx->http_ctx);
            ctx->http_ctx = NULL;
        }
        if (ctx->completion_id) {
            free(ctx->completion_id);
            ctx->completion_id = NULL;
        }
        if (ctx->endpoint) {
            free(ctx->endpoint);
            ctx->endpoint = NULL;
        }
        if (ctx->api_key) {
            free(ctx->api_key);
            ctx->api_key = NULL;
        }
        free(ctx);
    }


}
#endif // ONESDK_ENABLE_AI