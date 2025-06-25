/*
 * Copyright 2022-2024 Beijing Volcano Engine Technology Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "onesdk_config.h"
#ifdef ONESDK_ENABLE_IOT
#include "aws/common/allocator.h"
#include "aws/common/json.h"
#include "aws/common/string.h"
#include "iot/iot_utils.h"
#include "iot_log.h"
#include "thing_model/event.h"
#include "iot_mqtt.h"
#include "thing_model/iot_tm_api.h"
#include "thing_model/iot_tm_header.h"
#include <stdlib.h>
#include "util/util.h"
#include "util/aws_json.h"
#include "error_code.h"


/**
 * 初始化 aiot_tm_msg_event_post_t 结构体
 */
void iot_tm_msg_event_post_init(iot_tm_msg_event_post_t **event_post, const char* moduleKey, const char* identifier) {
    // iot_tm_msg_event_post_t *event = malloc( sizeof(iot_tm_msg_event_post_t));
    iot_tm_msg_event_post_t *event = (iot_tm_msg_event_post_t*)malloc(sizeof(iot_tm_msg_event_post_t));
    struct aws_allocator *alloc = aws_alloc();
    const char *random_id_str = get_random_string_with_time_suffix(alloc);
    event->id = random_id_str;
    event->version = SDK_VERSION;
    event->module_key = moduleKey;
    event->identifier = identifier;
    event->params = (void*) aws_json_value_new_object(alloc);
    *event_post = event;
}

/**
 *  向 aiot_tm_msg_event_post_t.params json 中 添加 value 为 int  的数据
 */
void iot_tm_msg_event_post_param_add_num(iot_tm_msg_event_post_t *event_post, char *key, double value) {

    aws_json_add_num_val1(aws_alloc(), (struct aws_json_value*)event_post->params, key, value);
}

/**
 *  向 aiot_tm_msg_event_post_t.params json 中 添加 value 为 string  的数据
 */
void iot_tm_msg_event_post_param_add_string(iot_tm_msg_event_post_t *event_post, char *key, char *value) {
    aws_json_add_str_val_1(aws_alloc(), (struct aws_json_value*)event_post->params, key, value);
}

void iot_tm_msg_event_post_set_prams_json_str(iot_tm_msg_event_post_t *event_post, char *param_json_str) {
    if (event_post->params != NULL) {
        aws_json_value_destroy((struct aws_json_value*)event_post->params);
    }
    event_post->params = (void*) aws_json_value_new_from_string(aws_alloc(), aws_byte_cursor_from_c_str(param_json_str));
}

/**
 * 释放 aiot_tm_msg_event_post_t 占用的内存
 */
void iot_tm_msg_event_post_free(iot_tm_msg_event_post_t *event_post) {
    if (event_post->payloadRoot != NULL) {
        aws_json_value_destroy((struct aws_json_value*)event_post->payloadRoot);
    } else if (event_post->params != NULL) {
        aws_json_value_destroy((struct aws_json_value*)event_post->params);
    }
    aws_mem_release(aws_alloc(), event_post);
}

/**
 * aiot_tm_msg_event_post_t 转换成 mqtt payload 数据
 */
void* iot_tm_msg_event_post_payload(iot_tm_msg_event_post_t *event) {
    if (event->payloadRoot != NULL) {
        aws_json_value_destroy((struct aws_json_value* )event->payloadRoot);
    }
    event->payloadRoot = (void*) aws_json_value_new_object(aws_alloc());
//    struct aws_string *time_str = get_date_format_iso_8601_data_str(aws_alloc());
    aws_json_add_str_val_1(aws_alloc(), (struct aws_json_value*)event->payloadRoot, "ID", event->id);
    aws_json_add_str_val_1(aws_alloc(), (struct aws_json_value*)event->payloadRoot, "Version", event->version);
//    aws_json_add_aws_string_val1(aws_alloc(), (struct aws_json_value*)event->payloadRoot, "time", time_str);

    struct aws_json_value *params = aws_json_value_new_object(aws_alloc());
    aws_json_add_num_val1(aws_alloc(), params, "Time", (double) get_current_time_mil());
    aws_json_add_json_obj(params, "Value", (struct aws_json_value*)event->params);

    aws_json_add_json_obj((struct aws_json_value*) event->payloadRoot, "Params", params);
    return event->payloadRoot;
}

/**
 * 发送 event
 */
int32_t _tm_send_event_post(void *handler, const char *topic, const void *msg_p) {
    // 发送数据给服务端
    iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) handler;
    iot_tm_msg_t *msg = (iot_tm_msg_t *) msg_p;

    int ret = VOLC_OK;
    struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, (struct aws_json_value* )iot_tm_msg_event_post_payload(msg->data.event_post));
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
    // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(topic);
    // uint16_t packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
    //                                                         AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
    //                                                         _tm_mqtt_post_on_complete_fn,
    //                                                         NULL);
    // PublishParams pubParams = DEFAULT_PUB_PARAMS;
    // pubParams.qos           = QOS1;
    // pubParams.payload_len   = payload_cur.len;
    // pubParams.payload       = (void *)payload_cur.ptr;
    // int packet_id = IOT_MQTT_Publish(dm_handle->mqtt_client, topic, &pubParams);
    iot_mqtt_publish(dm_handle->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);

    LOGD(TAG_IOT_MQTT, "_tm_send_event_post call topic = %.*s, payload = %.*s", topic,
        AWS_BYTE_CURSOR_PRI(payload_cur));

    // 回收内存数据
    aws_byte_buf_clean_up(&payload_buf);
    return ret;
}


void _tm_recv_event_post_reply(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
    struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
    struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

    LOGD(TAG_IOT_MQTT, "_tm_recv_event_post_reply call topic = %.*s,  payload = %.*s", 
        AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
        AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
    iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) pUserData;
    if (NULL == dm_handle->recv_handler) {
        return;
    }

    iot_tm_recv_t recv;
    AWS_ZERO_STRUCT(recv);
    recv.type = IOT_TM_RECV_EVENT_POST_REPLY;

    // 数据封装

    // 基于 topic 获取 product_key device_name module_key
    struct aws_array_list topic_split_data_list;
    aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8, sizeof(struct aws_byte_cursor));
    aws_byte_cursor_split_on_char(&topic_byte_cursor, '/', &topic_split_data_list);

    struct aws_byte_cursor product_key_cur = {0};
    aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
    recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);

    struct aws_byte_cursor device_name_cur = {0};
    aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
    recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

    struct aws_byte_cursor module_key_cur = {0};
    aws_array_list_get_at(&topic_split_data_list, &module_key_cur, 5);

    struct aws_byte_cursor identifier_cur = {0};
    aws_array_list_get_at(&topic_split_data_list, &identifier_cur, 6);

    iot_tm_recv_event_post_reply_t event_post_reply;
    struct aws_json_value *payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload_byte_cursor);
    event_post_reply.msg_id = aws_json_get_str(dm_handle->allocator, payload_json, "ID");
    event_post_reply.code = (int32_t) aws_json_get_num_val(payload_json, "Code");
    event_post_reply.module_key = aws_cur_to_char_str(dm_handle->allocator, &module_key_cur);
    event_post_reply.identifier = aws_cur_to_char_str(dm_handle->allocator, &identifier_cur);
    recv.data.event_post_reply = event_post_reply;


    // 回调给业务
    dm_handle->recv_handler(dm_handle, &recv, dm_handle->userdata);

    // 回收内存
    aws_mem_release(dm_handle->allocator, event_post_reply.msg_id);
    aws_mem_release(dm_handle->allocator, event_post_reply.module_key);
    aws_mem_release(dm_handle->allocator, event_post_reply.identifier);
    aws_mem_release(dm_handle->allocator, recv.product_key);
    aws_mem_release(dm_handle->allocator, recv.device_name);
    aws_json_value_destroy(payload_json);
    aws_array_list_clean_up(&topic_split_data_list);


}
#endif