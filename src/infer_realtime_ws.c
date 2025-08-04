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

#include <stddef.h>
#include <stdlib.h>

#include "infer_realtime_ws.h"
#include "aigw/auth.h"
#include "iot/dynreg.h"
#include "util/util.h"
#include "aigw/llm.h"
#include "plat/platform.h"

#include "aws/common/json.h"

#include "error_code.h"
#include "cJSON.h"

#define MAX_RECONNECT_TIMES 3

static int reconnect_times = 0;

static int aigw_lws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                void* user, void* in, size_t len) {
    aigw_ws_ctx_t* ctx = user;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            lwsl_user("%s: established\n", __func__);
            ctx->connected = true;
            ctx->try_connect = true;
            lws_callback_on_writable(ctx->active_conn);
            lws_set_timer_usecs(wsi, 1000000);
            ctx->ping_count = 0;
            reconnect_times = 0;
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            lwsl_err("CLIENT_CONNECTION_ERROR: %s\n", in ? (char*)in : "(null)");
            ctx->connected = false;
            if (reconnect_times < MAX_RECONNECT_TIMES) {
                lwsl_user("Reconnecting... %d\n", reconnect_times++);
                aigw_ws_connect(ctx);
            } else {
                lwsl_err("Reconnect failed\n");
            }
            break;
        case LWS_CALLBACK_CLIENT_CLOSED:
            lwsl_user("CLIENT_CONNECTION_CLOSED\n");
            ctx->connected = false;
            break;
        case LWS_CALLBACK_CLOSED:
            lwsl_user("LWS_CALLBACK_CLOSED\n");
            ctx->connected = false;
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            // lwsl_hexdump_notice(in, len);
            if (ctx->callback) {
                ctx->callback((char*)in, len, ctx->userdata);
            }
            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            if (!ctx->connected) {
                lwsl_err("Connection closed\n");
                return -1;
            }
            lws_pthread_mutex_lock(&ctx->lock);
            const my_item_t* msg = lws_ring_get_element(ctx->send_ring, &ctx->tail);
            if (!msg) {
                lwsl_user("No message to send\n");
                goto skip;
            }
            int m = lws_write(ctx->active_conn, ((unsigned char *)msg->value + LWS_PRE),
                msg->len, LWS_WRITE_TEXT);
            if (m < (int)msg->len) {
                lws_pthread_mutex_unlock(&ctx->lock);
                lwsl_err("Failed to send message\n");
                return -1;
            } else {
                lwsl_user("Sent message: %.*s\n", (int)msg->len, (char*)msg->value);
            }
skip:
            lws_ring_consume_single_tail(ctx->send_ring, &ctx->tail, 1);
            if (lws_ring_get_element(ctx->send_ring, &ctx->tail)) {
                lws_callback_on_writable(ctx->active_conn);
            }

            lws_pthread_mutex_unlock(&ctx->lock);
            break;
        }
        case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: {
            lwsl_user("LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: adding auth");
            unsigned char** p = in, *end = (*p) + len;
            char auth_hdr[1024];
            sprintf(auth_hdr, "Bearer %s", ctx->config->api_key);
            size_t auth_hdr_len = strlen(auth_hdr);
            if (lws_add_http_header_by_token (wsi,
                WSI_TOKEN_HTTP_AUTHORIZATION,
                (unsigned char *)auth_hdr,(int)auth_hdr_len, p, end) < 0) {
                lwsl_err("Failed to add header\n");
                return -1;
            }
            int32_t rnd = random_num();
            uint64_t ts = unix_timestamp();

            struct aws_allocator *alloc = aws_alloc();
            aws_common_library_init(alloc);

            struct iot_dynamic_register_basic_param registerParam = {
                .instance_id = ctx->iot_device_config->instance_id,
                .auth_type = ctx->iot_device_config->auth_type,
                .timestamp = ts,
                .random_num = rnd,
                .product_key = ctx->iot_device_config->product_key,
                .device_name = ctx->iot_device_config->device_name
            };

            aigw_auth_header_t *auth_header = aigw_auth_header_new(&registerParam,
                ctx->iot_device_config->device_secret);

            lws_add_http_header_by_name(wsi, (const unsigned char *)HEADER_SIGNATURE, (const unsigned char *)auth_header->signature,
                strlen(auth_header->signature), p, end);
            lws_add_http_header_by_name(wsi, (const unsigned char *)HEADER_AUTH_TYPE, (const unsigned char *)auth_header->auth_type,
                strlen(auth_header->auth_type), p, end);
            lws_add_http_header_by_name(wsi, (const unsigned char *)HEADER_DEVICE_NAME, (const unsigned char *)auth_header->device_name,
                strlen(auth_header->device_name), p, end);
            lws_add_http_header_by_name(wsi, (const unsigned char *)HEADER_PRODUCT_KEY, (const unsigned char *)auth_header->product_key,
                strlen(auth_header->product_key), p, end);
            lws_add_http_header_by_name(wsi, (const unsigned char *)HEADER_RANDOM_NUM, (const unsigned char *)auth_header->random_num,
                strlen(auth_header->random_num), p, end);
            lws_add_http_header_by_name(wsi, (const unsigned char *)HEADER_TIMESTAMP, (const unsigned char *)auth_header->timestamp,
                strlen(auth_header->timestamp), p, end);
            
            aigw_auth_header_free(auth_header);

            char *hw_id = plat_hardware_id();
            lwsl_user("hardware_id: %s\n", hw_id);
            lws_add_http_header_by_name(wsi, (const unsigned char *)HEADER_AIGW_HARDWARE_ID, (const unsigned char *)hw_id, strlen(hw_id), p, end);
            free(hw_id);

            break;
        }
        case LWS_CALLBACK_TIMER:
            if (ctx->connected) {
                if (ctx->config->send_ping && ctx->ping_count++ == ctx->config->ping_interval_s) {
                    ctx->ping_count = 0;
                    lwsl_user("Sending ping message\n");
                    char ping[LWS_PRE+4];
                    memset(ping, 0, LWS_PRE+4);
                    memcpy(ping+4, "ping", 4);
                    int m = lws_write(ctx->active_conn, (unsigned char *)ping, 4, LWS_WRITE_PING);
                    if (m < 4) {
                        lwsl_err("Failed to send ping message\n");
                    }
                }
                // 每秒触发writable，及时消费ring
                lws_set_timer_usecs(wsi, 1000000);
                if (lws_ring_get_element(ctx->send_ring, &ctx->tail)) {
                    lws_callback_on_writable(ctx->active_conn);
                }    
            }
            break;
        default:
            break;
    }
    return 0;
}

static const struct lws_protocols protocols[] = {
    {
        "ws_realtime",
        aigw_lws_callback,
        0,
        0,
        0,
        NULL,
        0
    },
    LWS_PROTOCOL_LIST_TERM // 必须以 NULL 结尾
};

static int copy_config(aigw_ws_config_t *dst, const aigw_ws_config_t *src) {
    if (!src || !dst) {
        return VOLC_ERR_INVALID_PARAM;
    }
    if (src->url) {
        dst->url = (const char *)strdup(src->url);
    }
    if (src->api_key) {
        dst->api_key = (const char *)strdup(src->api_key);
    }
    if (src->path) {
        dst->path = (const char *)strdup(src->path);
    }
    if (src->ca) {
        dst->ca = (const char *)strdup(src->ca);
    }
    dst->reconnect_interval_ms = src->reconnect_interval_ms;
    dst->verify_ssl = src->verify_ssl;
    dst->send_ping = src->send_ping;
    dst->ping_interval_s = src->ping_interval_s;
    return VOLC_OK;
}

void destroy_item(void *data) {
    my_item_t *item = (my_item_t *) data;
    if (item->value) {
        free(item->value);
        item->value = NULL;
    }
}

int aigw_ws_init(aigw_ws_ctx_t *ctx, const aigw_ws_config_t *config) {
    if (!ctx) {
        ctx = malloc(sizeof(aigw_ws_ctx_t));
        if (!ctx) {
            return VOLC_ERR_MALLOC;
        }
    }
    memset(ctx, 0, sizeof(aigw_ws_ctx_t));

    struct lws_context_creation_info info;;
    memset(&info, 0, sizeof(info));
    // if (config->verify_ssl) {
        info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    // }
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    if (config->ca) {
        info.client_ssl_ca_mem = config->ca;
        info.client_ssl_ca_mem_len = strlen(config->ca);
    }
    // info.fd_limit_per_thread = 1 + 1 + 1 + 1;
    ctx->lws_ctx = lws_create_context(&info);
    ctx->config = malloc(sizeof(aigw_ws_config_t));
    if (!ctx->config) {
        return VOLC_ERR_MALLOC;
    }
    memset(ctx->config, 0, sizeof(aigw_ws_config_t));
    copy_config(ctx->config, config);
    ctx->send_ring = lws_ring_create(sizeof(my_item_t), 50, destroy_item);
    lws_pthread_mutex_init(&ctx->lock);
    return VOLC_OK;
}

void iot_device_config_free(iot_basic_config_t * iot_device_config) {
    if (NULL == iot_device_config) {
        return ;
    }
    if (iot_device_config->http_host) {
        free((void*)iot_device_config->http_host);
        iot_device_config->http_host = NULL;
    }
    if (iot_device_config->http_host) {
        free((void*)iot_device_config->http_host);
        iot_device_config->http_host = NULL;
    }
    if (iot_device_config->instance_id) {
        free((void*)iot_device_config->instance_id);
        iot_device_config->instance_id = NULL;
    }
    if (iot_device_config->product_key) {
        free((void*)iot_device_config->product_key);
        iot_device_config->product_key = NULL;
    }
    if (iot_device_config->product_secret) {
        free((void*)iot_device_config->product_secret);
        iot_device_config->product_secret = NULL;
    }
    if (iot_device_config->device_name) {
        free((void*)iot_device_config->device_name);
        iot_device_config->device_name = NULL;
    }
    if (iot_device_config->device_secret) {
        free((void*)iot_device_config->device_secret);
        iot_device_config->device_secret = NULL;
    }
    if (iot_device_config->ssl_ca_path) {
        free((void*)iot_device_config->ssl_ca_path);
        iot_device_config->ssl_ca_path = NULL;
    }
    if (iot_device_config->ssl_ca_cert) {
        free((void*)iot_device_config->ssl_ca_cert);
        iot_device_config->ssl_ca_cert = NULL;
    }
    free(iot_device_config);
    iot_device_config = NULL;
}

void aigw_ws_deinit(aigw_ws_ctx_t* ctx) {
    lws_context_destroy(ctx->lws_ctx);
    if (ctx->send_ring) {
        lws_ring_destroy(ctx->send_ring);
    }
    if (ctx->config) {
        aigw_ws_config_deinit(ctx->config);
    }
    if (ctx->iot_device_config) {
        iot_device_config_free(ctx->iot_device_config);
    }
    lws_pthread_mutex_destroy(&ctx->lock);
    free(ctx);
    ctx = NULL;
}

int aigw_ws_connect(aigw_ws_ctx_t* ctx) {
    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof(ccinfo));
    const char *address, *prot, *path;
    int port = 0;
    char *url = strdup(ctx->config->url);
    lws_parse_uri(url, &prot, &address, &port, &path);
    ccinfo.address = strdup(address);
    ccinfo.port = port;
    ccinfo.context = ctx->lws_ctx;
    ccinfo.path = ctx->config->path;
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.local_protocol_name = "ws_realtime";
    if (!strcmp(prot, "https") || !strcmp(prot, "wss")) {
        ccinfo.ssl_connection = LCCSCF_USE_SSL;
    }
    if (!ctx->config->verify_ssl) {
        ccinfo.ssl_connection |= LCCSCF_ALLOW_SELFSIGNED; // 允许自签证书
        ccinfo.ssl_connection |= LCCSCF_ALLOW_EXPIRED; // 允许过期证书
        ccinfo.ssl_connection |= LCCSCF_ALLOW_INSECURE; // 不验证证书
   }
    ccinfo.pwsi = &ctx->active_conn;
    ccinfo.userdata = ctx;
    free(url);
    ctx->active_conn = lws_client_connect_via_info(&ccinfo);

    free((void*)ccinfo.address);
    if (!ctx->active_conn) {
        return VOLC_ERR_CONNECT;
    }
    return VOLC_OK;
}

void aigw_ws_disconnect(aigw_ws_ctx_t* ctx) {
    if (!ctx->connected) {
        return;
    }
    lws_close_reason(ctx->active_conn, LWS_CLOSE_STATUS_GOINGAWAY, (unsigned char *)"seeya", 5);
}

int aigw_ws_send_request(aigw_ws_ctx_t* ctx, const char* json_str) {
    if (json_str == NULL) {
        return VOLC_OK;
    }

    if (!ctx->connected && ctx->try_connect) {
        aigw_ws_connect(ctx);
        ctx->try_connect = false;
        sleep(1);
    }
    my_item_t msg;
    // need to add LWS_PRE before json_str
    size_t json_len = strlen(json_str);
    msg.value = malloc(json_len + LWS_PRE);
    if (!msg.value) {
        lwsl_err("Failed to allocate memory for message\n");
        return VOLC_ERR_MALLOC_FAILED;
    }
    memset((char*)msg.value, 0, json_len + LWS_PRE);
    memcpy((char*)msg.value+LWS_PRE, json_str, json_len);
    msg.len = strlen(json_str);
    lws_pthread_mutex_lock(&ctx->lock);
    int n = (int)lws_ring_insert(ctx->send_ring, &msg, 1);
    if (n != 1) {
        destroy_item(&msg);
        lwsl_err("Failed to send message: %s\n", json_str);
    }
    lws_pthread_mutex_unlock(&ctx->lock);
    if (ctx->connected) {
        lws_callback_on_writable(ctx->active_conn);
    }
    return VOLC_OK;
}

void aigw_ws_set_option(aigw_ws_ctx_t *ctx, aigw_ws_option_t option, void* value) {
    switch (option) {
        case AIGW_WS_URL:
            ctx->config->url = (char*)value;
            break;
        case AIGW_WS_PATH:
            ctx->config->path = (char*)value;
            break;
        case AIGW_WS_API_KEY:
            ctx->config->api_key = (char*)value;
            break;
        case AIGW_WS_RECONNECT_INTERVAL: {
            int temp_val = *(int*)value;
            ctx->config->reconnect_interval_ms = temp_val;
            break;
        }
        case AIGW_WS_VERIFY_SSL: {
            int temp_val = *(int*)value;
            ctx->config->verify_ssl = (temp_val != 0);
            break;
        }
        case AIGW_WS_SSL_CA_PATH:
            ctx->config->ca = (char*)value;
            break;
        case AIGW_WS_IOT_CONFIG:
            ctx->iot_device_config = iot_config_dup((iot_basic_config_t*)value);
            break;
        default:
            break;
    }
}

int aigw_ws_session_update(aigw_ws_ctx_t* ctx, const aigw_ws_session_t* session) {
    if (!session) {
        return VOLC_ERR_INVALID_PARAM;
    }
    // 创建根 JSON 对象
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "session.update");

    // 创建 "session" 对象
    cJSON* session_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "session", session_obj);

    // 添加 "modalities" 数组
    cJSON* modalities_array = cJSON_CreateArray();
    if (session->modality_audio_enabled) {
        cJSON_AddItemToArray(modalities_array, cJSON_CreateString("audio"));
    }
    if (session->modality_text_enabled) {
        cJSON_AddItemToArray(modalities_array, cJSON_CreateString("text"));
    }
    cJSON_AddItemToObject(session_obj, "modalities", modalities_array);

    // 添加其他字段
    cJSON_AddStringToObject(session_obj, "instructions", session->instructions);
    cJSON_AddStringToObject(session_obj, "voice", session->voice);
    cJSON_AddStringToObject(session_obj, "input_audio_format", session->input_audio_format);
    cJSON_AddStringToObject(session_obj, "output_audio_format", session->output_audio_format);
    cJSON_AddStringToObject(session_obj, "tool_choice", "auto");
    cJSON_AddNullToObject(session_obj, "turn_detection");

    // 添加 "input_audio_transcription" 对象
    cJSON* transcription_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(transcription_obj, "model", "any");
    cJSON_AddItemToObject(session_obj, "input_audio_transcription", transcription_obj);

    // 添加 "tools" 数组
    cJSON* tools_array = cJSON_CreateArray();
    if (session->tools != NULL) {
        cJSON* tool_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(tool_obj, "type", session->tools->type);
        cJSON_AddStringToObject(tool_obj, "name", session->tools->name);
        cJSON_AddStringToObject(tool_obj, "description", session->tools->description);

        // 解析 "parameters" 字符串为 JSON 对象
        cJSON* parameters_obj = cJSON_Parse(session->tools->parameters);
        cJSON_AddItemToObject(tool_obj, "parameters", parameters_obj);

        cJSON_AddItemToArray(tools_array, tool_obj);
    }
    cJSON_AddItemToObject(session_obj, "tools", tools_array);

    // 添加 "temperature" 字段
    cJSON_AddNumberToObject(session_obj, "temperature", 0.8);

    // 生成 JSON 字符串
    char * json_str = NULL;
    json_str = cJSON_PrintUnformatted(root);

    // 释放内存
    cJSON_Delete(root);

    int ret = aigw_ws_send_request(ctx, json_str);
    if (json_str)
        free(json_str);
    return ret;
}

int aigw_ws_input_audio_buffer_append(aigw_ws_ctx_t* ctx, const char* buffer, size_t len) {
    if (!buffer || len == 0) {
        printf("append buffer is empty, ignored\n");
        return VOLC_OK;
    }
    // 创建根 JSON 对象
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "input_audio_buffer.append");
    // 添加 "audio" 字段
    cJSON_AddStringToObject(root, "audio", buffer);
    // 生成 JSON 字符串
    const char* json_str = cJSON_PrintUnformatted(root);
    // 释放内存
    cJSON_Delete(root);
    int ret = aigw_ws_send_request(ctx, json_str);
    free((void*)json_str);
    return ret;
}

int aigw_ws_input_audio_buffer_commit(aigw_ws_ctx_t* ctx) {
    // 创建根 JSON 对象
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "input_audio_buffer.commit");
    // 生成 JSON 字符串
    const char* json_str = cJSON_PrintUnformatted(root);
    // 释放内存
    cJSON_Delete(root);
    int ret = aigw_ws_send_request(ctx, json_str);
    free((void*)json_str);
    return ret;
}

int aigw_ws_translation_session_update(aigw_ws_ctx_t* ctx, const aigw_ws_translation_session_t* session) {
    if (!session) {
        return VOLC_ERR_INVALID_PARAM;
    }
    // 创建根 JSON 对象
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "session.update");
    // 创建 "session" 对象
    cJSON* session_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "session", session_obj);

    // 添加 "input_audio_format" 字段
    if (session->input_audio_format) {
        cJSON_AddStringToObject(session_obj, "input_audio_format", session->input_audio_format);
    }

    // 添加 "modalities" 数组
    cJSON* modalities_array = cJSON_CreateArray();
    if (session->modalities && session->num_modalities > 0) {
        for (size_t i = 0; i < session->num_modalities; i++) {
            if (session->modalities[i]) {
                cJSON_AddItemToArray(modalities_array, cJSON_CreateString(session->modalities[i]));
            }
        }
    }
    cJSON_AddItemToObject(session_obj, "modalities", modalities_array);

    // 创建 "input_audio_translation" 对象
    cJSON* translation_config_obj = cJSON_CreateObject();
    if (session->input_audio_translation.source_language) {
        cJSON_AddStringToObject(translation_config_obj, "source_language", session->input_audio_translation.source_language);
    }
    if (session->input_audio_translation.target_language) {
        cJSON_AddStringToObject(translation_config_obj, "target_language", session->input_audio_translation.target_language);
    }

    // 创建 "add_vocab" 对象
    cJSON* add_vocab_obj = cJSON_CreateObject();
    // 添加 "hot_word_list" 数组
    cJSON* hot_word_list_array = cJSON_CreateArray();
    if (session->input_audio_translation.add_vocab.hot_word_list && session->input_audio_translation.add_vocab.num_hot_words > 0) {
        for (size_t i = 0; i < session->input_audio_translation.add_vocab.num_hot_words; i++) {
            if (session->input_audio_translation.add_vocab.hot_word_list[i]) {
                cJSON_AddItemToArray(hot_word_list_array, cJSON_CreateString(session->input_audio_translation.add_vocab.hot_word_list[i]));
            }
        }
    }
    cJSON_AddItemToObject(add_vocab_obj, "hot_word_list", hot_word_list_array);

    // 添加 "glossary_list" 数组
    cJSON* glossary_list_array = cJSON_CreateArray();
    if (session->input_audio_translation.add_vocab.glossary_list && session->input_audio_translation.add_vocab.num_glossary_items > 0) {
        for (size_t i = 0; i < session->input_audio_translation.add_vocab.num_glossary_items; i++) {
            cJSON* glossary_item_obj = cJSON_CreateObject();
            if (session->input_audio_translation.add_vocab.glossary_list[i].input_audio_transcription) {
                cJSON_AddStringToObject(glossary_item_obj, "input_audio_transcription", session->input_audio_translation.add_vocab.glossary_list[i].input_audio_transcription);
            }
            if (session->input_audio_translation.add_vocab.glossary_list[i].input_audio_translation) {
                cJSON_AddStringToObject(glossary_item_obj, "input_audio_translation", session->input_audio_translation.add_vocab.glossary_list[i].input_audio_translation);
            }
            cJSON_AddItemToArray(glossary_list_array, glossary_item_obj);
        }
    }
    cJSON_AddItemToObject(add_vocab_obj, "glossary_list", glossary_list_array);
    cJSON_AddItemToObject(translation_config_obj, "add_vocab", add_vocab_obj);
    cJSON_AddItemToObject(session_obj, "input_audio_translation", translation_config_obj);

    // 生成 JSON 字符串
    char *json_str = cJSON_PrintUnformatted(root);
    // 释放内存
    cJSON_Delete(root);

    int ret = aigw_ws_send_request(ctx, json_str);
    if (json_str) {
        free(json_str);
    }
    return ret;
}

int aigw_ws_input_audio_done(aigw_ws_ctx_t* ctx) {
    // 创建根 JSON 对象
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "input_audio.done");
    // 生成 JSON 字符串
    const char* json_str = cJSON_PrintUnformatted(root);
    // 释放内存
    cJSON_Delete(root);
    int ret = aigw_ws_send_request(ctx, json_str);
    free((void*)json_str);
    return ret;
}

int aigw_ws_conversation_item_create(aigw_ws_ctx_t* ctx, char* func_call_id) {
    // 创建根 JSON 对象
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "conversation_item.create");
    // 创建 "item" 对象
    cJSON* item_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "item", item_obj);
    // 添加 "call_id" 字段
    cJSON_AddStringToObject(item_obj, "call_id", func_call_id);
    cJSON_AddStringToObject(item_obj, "type", "function_call_output");
    // 创建 "output" 对象
    cJSON* output_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(item_obj, "output", output_obj);
    // 添加 "output_type" 字段
    cJSON_AddStringToObject(output_obj, "result", "打开成功");
    // 生成 JSON 字符串
    const char* json_str = cJSON_PrintUnformatted(root);
    // 释放内存
    cJSON_Delete(root);
    int ret = aigw_ws_send_request(ctx, json_str);
    free((void*)json_str);
    return ret;
}

int aigw_ws_response_create(aigw_ws_ctx_t* ctx) {
    // 创建根 JSON 对象
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "response.create");
    // 创建 "response" 对象
    cJSON* response_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "response", response_obj);
    // 添加 “modalities” 字段
    cJSON* modalities_array = cJSON_CreateArray();
    cJSON_AddItemToArray(modalities_array, cJSON_CreateString("text"));
    cJSON_AddItemToArray(modalities_array, cJSON_CreateString("audio"));
    cJSON_AddItemToObject(response_obj, "modalities", modalities_array);
    // 生成 JSON 字符串
    const char* json_str = cJSON_PrintUnformatted(root);
    // 释放内存
    cJSON_Delete(root);
    int ret = aigw_ws_send_request(ctx, json_str);
    free((void*)json_str);
    return ret;
}

int aigw_ws_response_cancel(aigw_ws_ctx_t* ctx) {
    // 创建根 JSON 对象
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "response.cancel");
    // 生成 JSON 字符串
    const char* json_str = cJSON_PrintUnformatted(root);
    // 释放内存
    cJSON_Delete(root);
    int ret = aigw_ws_send_request(ctx, json_str);
    free((void*)json_str);
    return ret;
}

int aigw_ws_run_event_loop(aigw_ws_ctx_t* ctx, int timeout_ms) {
    if (!ctx) {
        return VOLC_ERR_INVALID_PARAM;
    }
    return lws_service(ctx->lws_ctx, timeout_ms);
}

void aigw_ws_register_callback(aigw_ws_ctx_t* ctx, aigw_ws_message_cb callback, void* userdata) {
    ctx->callback = callback;
    ctx->userdata = userdata;
}

void aigw_ws_config_deinit(aigw_ws_config_t *config) {
    if (!config) return;

    // 释放所有可能动态分配的字符串资源
    if (config->url) {
        free((void *)config->url);
        config->url = NULL;
    }
    if (config->path) {
        free((void *)config->path);
        config->path = NULL;
    }
    if (config->api_key) {
        free((void *)config->api_key);
        config->api_key = NULL;
    }
    if (config->ca) {
        free((void*)config->ca);
        config->ca = NULL;
    }
    // 最后释放结构体本身
    free(config);
    config = NULL;
}

int onesdk_iot_get_binding_aigw_info(aigw_llm_config_t *llm_config, aigw_ws_config_t * aigw_ws_config, iot_basic_ctx_t *ctx) {
    // 调用iot平台接口获取绑定的网关信息
    if (llm_config == NULL) {
        return VOLC_ERR_INVALID_PARAM;
    }
    if (llm_config->url != NULL)
        aigw_ws_config->url = strdup(llm_config->url);
    if (llm_config->api_key!= NULL)
        aigw_ws_config->api_key = strdup(llm_config->api_key);
    aigw_ws_config->path = strdup("/v1/realtime?model=AG-voice-chat-agent");
    aigw_ws_config->verify_ssl = ctx->config->verify_ssl;
    if (ctx->config->ssl_ca_cert != NULL)
        aigw_ws_config->ca = strdup(ctx->config->ssl_ca_cert);
    return VOLC_OK;
}

#endif // ONESDK_ENABLE_AI