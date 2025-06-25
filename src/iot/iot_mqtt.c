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

#ifdef ONESDK_ENABLE_IOT

#include <stdlib.h>
#include "aws/common/byte_buf.h"
#include "aws/common/encoding.h"

#include "dynreg.h"
#include "util/hmac_sha256.h"
#include "util/util.h"
#include "iot_mqtt.h"
#include "libwebsockets.h"
#include "error_code.h"

static int callback_mqtt(struct lws *wsi, enum lws_callback_reasons reason,
        void *user, void *in, size_t len)
{
    lws_mqtt_publish_param_t *pub;
    /*
    * context 是全局的上下文，可以通过 lws_get_context 获取，用于得到 user。    
    * lws_client_connect_via_info 创建的 wsi 是一个具体的连接实例，参数中 wsi 
    * 是处理某个事件的上下文。因此要实时更新 wsi。
    */ 
    struct lws_context *context = lws_get_context(wsi);
    iot_mqtt_ctx_t *ctx = (iot_mqtt_ctx_t *)lws_context_user(context);
    ctx->wsi = wsi;

    switch (reason) {
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        lwsl_err("%s: CLIENT_CONNECTION_ERROR: %s\n", __func__,
            in ? (char *)in : "(null)");
        ctx->is_connected = false;
        break;

    case LWS_CALLBACK_MQTT_CLIENT_CLOSED:
        lwsl_err("%s: CLIENT_CLOSED\n", __func__);
        ctx->is_connected = false;
        if (ctx->config->auto_reconnect) {
            iot_mqtt_reconnect(ctx);
        }
        break;

    case LWS_CALLBACK_MQTT_CLIENT_ESTABLISHED:
        lwsl_info("%s: MQTT_CLIENT_ESTABLISHED\n", __func__);
        ctx->is_connected = true;
        ctx->waiting_for_puback = 0;
        ctx->waiting_for_suback = 0;
        lws_callback_on_writable(wsi);
        lws_set_timer_usecs(wsi, ctx->config->ping_interval*1000000);
        
        return 0;

    case LWS_CALLBACK_MQTT_SUBSCRIBED:
        lwsl_info("%s: MQTT_SUBSCRIBED\n", __func__);
        ctx->waiting_for_suback = 0;
        // 检查是否还有待处理的订阅
        lws_pthread_mutex_lock(&ctx->sub_topic_mutex);
        bool has_pending_subs = (ctx->pending_sub_list != NULL && ctx->pending_sub_list->count > 0);
        lws_pthread_mutex_unlock(&ctx->sub_topic_mutex);

        if (has_pending_subs) {
            lws_callback_on_writable(wsi); // 如果有，请求 writable 以处理下一批
        }

        lws_pthread_mutex_lock(&ctx->pub_topic_mutex);
        bool has_pending_pubs = (ctx->pending_pub_list != NULL && ctx->pending_pub_list->count > 0);
        lws_pthread_mutex_unlock(&ctx->pub_topic_mutex);
        if (has_pending_pubs) {
            lws_callback_on_writable(wsi);
        }

        break;

    case LWS_CALLBACK_MQTT_CLIENT_WRITEABLE:
        lwsl_info("%s: MQTT_CLIENT_WRITEABLE\n", __func__);
        
        // 处理待订阅列表
        if (!ctx) break;
        if (ctx->waiting_for_suback) break;
            
        lws_pthread_mutex_lock(&ctx->sub_topic_mutex);
        if (ctx->pending_sub_list != NULL && ctx->pending_sub_list->count > 0) {
            // 确定本次订阅的数量，最多8个
            size_t num_to_subscribe = ctx->pending_sub_list->count > 7 ? 7 : ctx->pending_sub_list->count;

            lws_mqtt_topic_elem_t *topics = (lws_mqtt_topic_elem_t *)malloc(
                num_to_subscribe * sizeof(lws_mqtt_topic_elem_t));
            if (topics == NULL) {
                lws_pthread_mutex_unlock(&ctx->sub_topic_mutex);
                return VOLC_ERR_MALLOC;
            }
            memset(topics, 0, num_to_subscribe * sizeof(lws_mqtt_topic_elem_t));
            // 填充本次要订阅的topic
            for (size_t i = 0; i < num_to_subscribe; i++) {
                topics[i].name = ctx->pending_sub_list->topic_maps[i].topic;
                topics[i].qos = ctx->pending_sub_list->topic_maps[i].qos;
            }
            lws_mqtt_subscribe_param_t sub_param = {
                .topic = topics,
                .num_topics = num_to_subscribe,
            };
            // 发送订阅请求
            if (lws_mqtt_client_send_subcribe(wsi, &sub_param)) {
                lwsl_err("%s: subscribe failed\n", __func__);
                free(topics);
                lws_pthread_mutex_unlock(&ctx->sub_topic_mutex);
                // 返回错误可能导致连接断开，这里可以选择不返回错误，下次重试
                // return VOLC_ERR_MQTT_SUB; // 或者其他合适的错误码
                return 0; // 暂时不返回错误，允许后续操作
            }
            // 释放临时的topic_elem_t数组
            free(topics);

            ctx->waiting_for_suback = 1; 

            // 将成功发送订阅请求的topic移入sub_topic_maps
            for (size_t i = 0; i < num_to_subscribe; i++) {
                iot_mqtt_topic_map_t *topic_map = &ctx->pending_sub_list->topic_maps[i];
                
                // 检查是否已存在于 sub_topic_maps
                bool is_duplicate = false;
                for (size_t j = 0; j < ctx->sub_topic_count; j++) {
                    // 使用 strcmp 进行精确匹配
                    if (!strcmp(topic_map->topic, ctx->sub_topic_maps[j].topic)) {
                        lwsl_notice("%s: topic '%s' already subscribed, skipping add to sub_topic_maps\n", __func__, topic_map->topic);
                        is_duplicate = true;
                        break;
                    }
                }
                // 如果是重复的，则释放待处理列表中的 topic 内存并跳过
                if (is_duplicate) {
                    free((void*)topic_map->topic); // 释放重复 topic 的内存
                    continue;
                }

                // 扩容 sub_topic_maps 数组
                iot_mqtt_topic_map_t *new_sub_topic_maps = (iot_mqtt_topic_map_t *)realloc(ctx->sub_topic_maps, 
                    (ctx->sub_topic_count + 1) * sizeof(iot_mqtt_topic_map_t));
                if (new_sub_topic_maps == NULL) {
                    // realloc 失败，保留原有 sub_topic_maps
                    lwsl_err("%s: realloc sub_topic_maps failed\n", __func__);
                    // 释放当前 topic_map 的 topic 内存，因为它无法被添加
                    free((void*)topic_map->topic);
                    // 这里不直接返回错误，允许处理剩余的 topic，但记录下内存问题
                    continue; // 继续处理下一个 topic
                }
                ctx->sub_topic_maps = new_sub_topic_maps;

                // 添加新的订阅信息到 sub_topic_maps
                // 注意：strdup 在 _iot_mqtt_append_sub_topic 中已经完成，这里直接转移所有权
                ctx->sub_topic_maps[ctx->sub_topic_count].topic = topic_map->topic; // 直接使用已分配的内存
                ctx->sub_topic_maps[ctx->sub_topic_count].message_callback = topic_map->message_callback;
                ctx->sub_topic_maps[ctx->sub_topic_count].event_callback = topic_map->event_callback;
                ctx->sub_topic_maps[ctx->sub_topic_count].user_data = topic_map->user_data;
                ctx->sub_topic_maps[ctx->sub_topic_count].qos = topic_map->qos; // 保存 qos
                ctx->sub_topic_count++;
            }

            // 从 pending_sub_list 中移除已处理的 topic
            size_t remaining_count = ctx->pending_sub_list->count - num_to_subscribe;
            if (remaining_count > 0) {
                // 将未处理的 topic 向前移动
                memmove(ctx->pending_sub_list->topic_maps, 
                        &ctx->pending_sub_list->topic_maps[num_to_subscribe], 
                        remaining_count * sizeof(iot_mqtt_topic_map_t));
                
                // 重新分配内存以缩小数组
                iot_mqtt_topic_map_t *shrunk_maps = (iot_mqtt_topic_map_t *)realloc(ctx->pending_sub_list->topic_maps, 
                                                                remaining_count * sizeof(iot_mqtt_topic_map_t));
                // 如果 realloc 成功或 remaining_count 为 0（此时 shrunk_maps 可能为 NULL），则更新指针
                if (shrunk_maps != NULL || remaining_count == 0) {
                     ctx->pending_sub_list->topic_maps = shrunk_maps;
                } else {
                    // realloc 失败，保留原有内存块，但 count 已更新
                    lwsl_warn("%s: realloc pending_sub_list->topic_maps failed, potential memory leak\n", __func__);
                }
            } else {
                // 所有 topic 都已处理，释放整个列表
                free(ctx->pending_sub_list->topic_maps);
                free(ctx->pending_sub_list);
                ctx->pending_sub_list = NULL;
            }

            // 更新待处理列表计数
            if (ctx->pending_sub_list) {
                ctx->pending_sub_list->count = remaining_count;
                // 如果还有待处理的订阅，请求再次变为可写状态
                if (remaining_count > 0) {
                    lws_callback_on_writable(wsi);
                }
            }

            lws_pthread_mutex_unlock(&ctx->sub_topic_mutex);
            return 0; // 一次只能处理一类事件，否则收到包id会被跳过。
        }
        lws_pthread_mutex_unlock(&ctx->sub_topic_mutex);

        lws_pthread_mutex_lock(&ctx->pub_topic_mutex);
        // 处理待发布列表
        if (ctx->pending_pub_list != NULL && !ctx->waiting_for_puback) {
            size_t i;
            for (i = 0; i < ctx->pending_pub_list->count; i++) {
                if (ctx->pending_pub_list->sent_list[i]) continue; // 跳过已发送的消息

                lwsl_debug("%s: publish start\n", __func__);
                lws_mqtt_publish_param_t *pub_param = &ctx->pending_pub_list->publish_params[i];
                if (pub_param->qos == QOS0) {
                    // 直接发送QOS0消息
                    if (lws_mqtt_client_send_publish(wsi, pub_param, 
                        pub_param->payload, pub_param->payload_len, true)) {
                        lwsl_err("%s: publish failed\n", __func__);
                    } else {
                        ctx->pending_pub_list->sent_list[i] = 1; // 标记为已发送
                    }
                } else if (pub_param->qos == QOS1) {
                    // 发送QOS1消息
                    if (lws_mqtt_client_send_publish(wsi, pub_param,
                        pub_param->payload, pub_param->payload_len, true)) {
                        lwsl_err("%s: publish failed\n", __func__);
                    } else {
                        ctx->waiting_for_puback = 1;
                        ctx->pending_pub_list->sent_list[i] = 1; // 标记为已发送
                        break; // 一次只发送一个QoS1消息
                    }
                }
            }
            if (i == ctx->pending_pub_list->count) {
                // 清空待发布列表
                free(ctx->pending_pub_list->publish_params);
                free(ctx->pending_pub_list->sent_list);
                free(ctx->pending_pub_list);
                ctx->pending_pub_list = NULL;
            }
        }

        lws_pthread_mutex_unlock(&ctx->pub_topic_mutex);
        return 0;

    case LWS_CALLBACK_MQTT_ACK:
        lwsl_info("%s: MQTT_ACK\n", __func__);
        if (ctx->waiting_for_puback) {
            ctx->waiting_for_puback = 0;
        }
        lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_MQTT_RESEND:
        lwsl_info("%s: MQTT_RESEND\n", __func__);
        lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_MQTT_CLIENT_RX:
        lwsl_info("%s: MQTT_CLIENT_RX\n", __func__);
        pub = (lws_mqtt_publish_param_t *)in;
        
        for (size_t i = 0; i < ctx->sub_topic_count; i++) {
            if (!strncmp(pub->topic, ctx->sub_topic_maps[i].topic, pub->topic_len)) {
                ctx->sub_topic_maps[i].message_callback(pub->topic, 
                    pub->payload, pub->payload_len, ctx->sub_topic_maps[i].user_data);
                break;
            }
            // match topic，以+号结尾的topic需要匹配
            // pub->topic是完整的topic，sub_topic_maps可能以+结尾，需要模糊匹配
            if (ctx->sub_topic_maps[i].topic[strlen(ctx->sub_topic_maps[i].topic)-1] == '+' && !strncmp(pub->topic, ctx->sub_topic_maps[i].topic, strlen(ctx->sub_topic_maps[i].topic)-1)) {
                ctx->sub_topic_maps[i].message_callback(pub->topic,
                    pub->payload, pub->payload_len, ctx->sub_topic_maps[i].user_data);
                break;
            }
    
        }        

        lwsl_hexdump_notice(pub->topic, pub->topic_len);
        lwsl_hexdump_notice(pub->payload, pub->payload_len);
        lws_callback_on_writable(wsi);
        return 0;

    case LWS_CALLBACK_TIMER:
        lwsl_debug("timer\n");
        if (ctx->is_connected) {
            lws_set_timer_usecs(wsi, 10*1000000);
            lws_callback_on_writable(wsi);
        }
        break;

    default:
        break;
    }

    return 0;
}

static const struct lws_protocols protocols[] = {
    {
        .name			= "mqtt",
        .callback		= callback_mqtt,
    },
    LWS_PROTOCOL_LIST_TERM
};

static struct aws_string *_iot_mqtt_get_username(iot_basic_config_t *basic_config) {
    struct aws_allocator *alloc = aws_alloc();
    aws_common_library_init(alloc);
    
    int length = strlen(basic_config->product_key) + strlen(basic_config->device_name) + 10;
    char clientIdStr[length];
    sprintf(clientIdStr, "%s|%s", basic_config->product_key, basic_config->device_name);
    
    return aws_string_new_from_c_str(alloc, clientIdStr);
}

static struct aws_string *_iot_mqtt_get_password(iot_basic_config_t *basic_config) {
    int32_t rnd = random_num();
    uint64_t ts = unix_timestamp();
    struct aws_allocator *alloc = aws_alloc();
    aws_common_library_init(alloc);

    onesdk_auth_type_t auth_type = basic_config->auth_type;
    if (basic_config->auth_type == ONESDK_AUTH_DEVICE_SECRET) {
        // mqtt password 认证类型仅支持0或1
        auth_type = ONESDK_AUTH_DYNAMIC_PRE_REGISTERED; 
    }
    struct iot_dynamic_register_basic_param registerParam = {
            .auth_type = auth_type,
            .timestamp = ts,
            .random_num = rnd,
            .product_key = basic_config->product_key,
            .device_name = basic_config->device_name
    };

    struct aws_string *sign = iot_hmac_sha256_encrypt(alloc, &registerParam, basic_config->device_secret);
    char values[500];
    sprintf(values, "%d|%ld|%llu|%s", auth_type, rnd, ts, aws_string_c_str(sign));
    aws_string_destroy_secure(sign);
    
    return aws_string_new_from_c_str(alloc, values);
}

static lws_retry_bo_t retry = {
    .secs_since_valid_ping		= IOT_DEFAULT_PING_INTERVAL_S, /* if idle, PINGREQ after secs */
    .secs_since_valid_hangup	= 65, /* hangup if still idle secs */
};

int iot_mqtt_init(iot_mqtt_ctx_t *ctx, iot_mqtt_config_t *config) {
    int ret = VOLC_OK;
    if (ctx == NULL || config == NULL) {
        return VOLC_ERR_INVALID_PARAM;
    }
    memset(ctx, 0, sizeof(iot_mqtt_ctx_t));
    
    ctx->config = config;
    if (ctx->config->basic_config == NULL) {
        return VOLC_ERR_INVALID_PARAM;
    }
    ctx->config->username = _iot_mqtt_get_username(ctx->config->basic_config);
    ctx->config->password = _iot_mqtt_get_password(ctx->config->basic_config);
    if (config->ping_interval <= 0) {
        ctx->config->ping_interval = IOT_DEFAULT_PING_INTERVAL_S;
    }

    // init lws context
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    // info.fd_limit_per_thread = 1 + 1 + 1;
    // info.register_notifier_list = na;
    info.retry_and_idle_policy = &retry;
    info.user = (void*)ctx;
    // info.client_ssl_ca_filepath = NULL;
    struct lws_context *context = lws_create_context(&info);
    if (context == NULL) {
        lwsl_err("lws init failed\n");
        return VOLC_ERR_INIT;
    }
    ctx->context = context;
    
    lws_pthread_mutex_init(&ctx->sub_topic_mutex);
    lws_pthread_mutex_init(&ctx->pub_topic_mutex);

    return ret;
}

void iot_mqtt_deinit(iot_mqtt_ctx_t *ctx) {
    aws_string_destroy_secure(ctx->config->username);
    aws_string_destroy_secure(ctx->config->password);

    if (ctx->context) {
        lws_context_destroy(ctx->context);
    }

    lws_pthread_mutex_destroy(&ctx->sub_topic_mutex);
    lws_pthread_mutex_destroy(&ctx->pub_topic_mutex);
}

int iot_mqtt_connect(iot_mqtt_ctx_t *ctx) {
    struct lws_client_connect_info i;
    memset(&i, 0, sizeof i);
    const char *username = aws_string_c_str(ctx->config->username);
    const char *password = aws_string_c_str(ctx->config->password);
    static lws_mqtt_client_connect_param_t client_connect_param = {
        .clean_start			= 1,
        .client_id_nofree		= 1,
        .username_nofree		= 1,
        .password_nofree		= 1,
    };
    client_connect_param.client_id = username;
    client_connect_param.username = username;
    client_connect_param.password = password;
    client_connect_param.keep_alive = ctx->config->keep_alive;
    
    i.mqtt_cp = &client_connect_param;
    i.address = ctx->config->mqtt_host;
    i.host = ctx->config->mqtt_host;
    i.protocol = "mqtt";
    i.context = ctx->context;
    i.method = "MQTT";
    i.alpn = "mqtt";
    i.port = 1883;

    if (ctx && ctx->config && ctx->config->enable_mqtts) {
        i.ssl_connection = LCCSCF_USE_SSL;
        i.ssl_connection |= LCCSCF_ALLOW_SELFSIGNED;
        i.port = 8883;
    }
    ctx->wsi = lws_client_connect_via_info(&i);
    if (!ctx->wsi) {
        lwsl_err("%s: Client Connect Failed\n", __func__);

        return 1;
    }

    return 0;
}

int iot_mqtt_disconnect(iot_mqtt_ctx_t *ctx) {
    int ret = VOLC_OK;

    lws_cancel_service(ctx->context);
    return ret;
}

static void _iot_mqtt_append_sub_topic(iot_mqtt_ctx_t *ctx, iot_mqtt_topic_map_t *topic_map) {
    lws_pthread_mutex_lock(&ctx->sub_topic_mutex);
    // 将topic_map存入待订阅列表
    if (ctx->pending_sub_list == NULL) {
        ctx->pending_sub_list = (iot_mqtt_pending_sub_list_t *)malloc(sizeof(iot_mqtt_pending_sub_list_t));
        ctx->pending_sub_list->topic_maps = NULL;
        ctx->pending_sub_list->count = 0;
    }
    // copy topic_map
    iot_mqtt_topic_map_t *topic_map_copy = (iot_mqtt_topic_map_t *)malloc(sizeof(iot_mqtt_topic_map_t));
    memset(topic_map_copy, 0, sizeof(iot_mqtt_topic_map_t));
    topic_map_copy->topic = strdup(topic_map->topic);
    topic_map_copy->message_callback = topic_map->message_callback;
    topic_map_copy->event_callback = topic_map->event_callback;
    topic_map_copy->user_data = topic_map->user_data;
    topic_map_copy->qos = topic_map->qos;
    ctx->pending_sub_list->topic_maps = (iot_mqtt_topic_map_t *)realloc(ctx->pending_sub_list->topic_maps, (ctx->pending_sub_list->count + 1) * sizeof(iot_mqtt_topic_map_t));
    ctx->pending_sub_list->topic_maps[ctx->pending_sub_list->count] = *topic_map_copy;
    ctx->pending_sub_list->count++;
    lws_pthread_mutex_unlock(&ctx->sub_topic_mutex);
}

int iot_mqtt_reconnect(iot_mqtt_ctx_t *ctx) {
    int ret = VOLC_OK;

    // 断开当前连接
    ret = iot_mqtt_disconnect(ctx);
    if (ret != VOLC_OK) {
        lwsl_err("%s: Disconnect failed\n", __func__);
        return ret;
    }

    // 重新建立连接
    ret = iot_mqtt_connect(ctx);
    if (ret != VOLC_OK) {
        lwsl_err("%s: Reconnect failed\n", __func__);
        return ret;
    }

    // 重新订阅topic
    for (int i = 0; i < ctx->sub_topic_count; i++) {
        iot_mqtt_topic_map_t *topic_map = &ctx->sub_topic_maps[i];
        _iot_mqtt_append_sub_topic(ctx, topic_map);
    }

    return ret;
}

int iot_mqtt_publish(iot_mqtt_ctx_t *ctx, const char *topic, const uint8_t *payload, size_t len, int qos) {
    int ret = VOLC_OK;

    // 组装pub_param，放入pending_pub_list
    lws_mqtt_publish_param_t *pub_param = malloc(sizeof(lws_mqtt_publish_param_t));
    memset(pub_param, 0, sizeof(lws_mqtt_publish_param_t));
    pub_param->topic = strdup(topic);
    pub_param->topic_len = strlen(topic);
    void* payload_byte = malloc(len);
    if (payload_byte == NULL) {
        // 内存分配失败，记录错误信息
        lwsl_err("%s: Memory allocation failed for payload\n", __func__);
        // 可以选择返回错误码或进行其他处理
        return VOLC_ERR_MALLOC;
    }
    memcpy(payload_byte, payload, len);
    pub_param->payload = payload_byte;
    pub_param->payload_len = len;
    pub_param->qos = qos;

    lws_pthread_mutex_lock(&ctx->pub_topic_mutex);
    if (ctx->pending_pub_list == NULL) {
        ctx->pending_pub_list = (iot_mqtt_pending_pub_list_t *)malloc(sizeof(iot_mqtt_pending_pub_list_t));
        if (ctx->pending_pub_list == NULL) {
            // 内存分配失败，记录错误信息
            lwsl_err("%s: Memory allocation failed for pending_pub_list\n", __func__);
            // 可以选择返回错误码或进行其他处理
            ret = VOLC_ERR_MALLOC;
            goto finish;
        }
        ctx->pending_pub_list->publish_params = NULL;
        ctx->pending_pub_list->sent_list = NULL;
        ctx->pending_pub_list->count = 0;
    }
    
    // 限制最大发布数量
    if (ctx->pending_pub_list->count > 10) {
        ret = VOLC_ERR_MQTT_PUB;
        lwsl_err("%s: publish failed\n", __func__);
        lws_pthread_mutex_unlock(&ctx->pub_topic_mutex);
        goto finish;
    }
        
    ctx->pending_pub_list->publish_params = (lws_mqtt_publish_param_t *)realloc(ctx->pending_pub_list->publish_params, (ctx->pending_pub_list->count + 1) * sizeof(lws_mqtt_publish_param_t));
    ctx->pending_pub_list->publish_params[ctx->pending_pub_list->count] = *pub_param;
    ctx->pending_pub_list->sent_list = (bool *)realloc(ctx->pending_pub_list->sent_list, (ctx->pending_pub_list->count + 1) * sizeof(bool));
    ctx->pending_pub_list->sent_list[ctx->pending_pub_list->count] = false;
    ctx->pending_pub_list->count++;

finish:
    lws_pthread_mutex_unlock(&ctx->pub_topic_mutex);
    if (ctx->is_connected) {
        // lwsl_err("ctx->wsi %p\n", ctx->wsi);
        // lws_callback_on_writable(ctx->wsi);
    }

    return ret;
}

int iot_mqtt_subscribe(iot_mqtt_ctx_t *ctx, iot_mqtt_topic_map_t *topic_map) {
    if (ctx == NULL || topic_map == NULL) {
        return VOLC_ERR_INVALID_PARAM;
    }
    
    // 检查topic是否已经订阅
    for (size_t i = 0; i < ctx->sub_topic_count; i++) {
        if (!strcmp(topic_map->topic, ctx->sub_topic_maps[i].topic)) {
            return VOLC_OK;
        }
    }

    _iot_mqtt_append_sub_topic(ctx, topic_map);

    return 0;
}

int iot_mqtt_run_event_loop(iot_mqtt_ctx_t* ctx, int timeout_ms) {
    if (ctx == NULL) {
        return VOLC_ERR_INVALID_PARAM;
    }
    return lws_service(ctx->context, timeout_ms);
}
#endif // ONESDK_ENABLE_IOT