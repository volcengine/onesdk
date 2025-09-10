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

#include "thing_model/custom_topic.h"
#include "thing_model/iot_tm_api.h"
#include "thing_model/iot_tm_header.h"
#include "iot/iot_utils.h"
#include "util/aws_json.h"
#include "error_code.h"
#include "iot_log.h"
#include <stdlib.h>
#include <string.h>

void iot_tm_msg_aiot_tm_msg_custom_topic_post_init(iot_tm_msg_custom_topic_post_t **custom_topic_post, char *custom_topic_suffix, char *payload_json) {
    iot_tm_msg_custom_topic_post_t *data = (iot_tm_msg_custom_topic_post_t *)malloc(sizeof(iot_tm_msg_custom_topic_post_t));
    memset(data, 0, sizeof(iot_tm_msg_custom_topic_post_t));
    data->custom_topic_suffix = custom_topic_suffix;
    data->params = payload_json;

    *custom_topic_post = data;
}

void iot_tm_msg_aiot_tm_msg_custom_topic_post_free(iot_tm_msg_custom_topic_post_t *custom_topic_post) {
    if (custom_topic_post) {
        free(custom_topic_post);
    }
}

char* iot_tm_msg_aiot_tm_msg_custom_topic_post_payload(iot_tm_msg_custom_topic_post_t *custom_topic_post) {
    return custom_topic_post->params;
}

int32_t _tm_send_custom_topic_post_data(void *handler, const char *topic, const void *msg_p) {
    iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) handler;
    iot_tm_msg_t *msg = (iot_tm_msg_t *) msg_p;

    int ret = VOLC_OK;
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_c_str(iot_tm_msg_aiot_tm_msg_custom_topic_post_payload(msg->data.custom_topic_post));
    iot_mqtt_publish(dm_handle->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);

    return ret;
}

// static void _custom_topic_mqtt_event_callback(void *pclient, MQTTEventType event_type, void *user_data)
// {
//     // OTA_MQTT_Struct_t *h_osc = (OTA_MQTT_Struct_t *)user_data;

//     switch (event_type) {
//         case MQTT_EVENT_SUBCRIBE_SUCCESS:
//             Log_d("OTA topic subscribe success");
//         // h_osc->topic_ready = true;
//         break;

//         case MQTT_EVENT_SUBCRIBE_TIMEOUT:
//             Log_i("OTA topic subscribe timeout");
//         // h_osc->topic_ready = false;
//         break;

//         case MQTT_EVENT_SUBCRIBE_NACK:
//             Log_i("OTA topic subscribe NACK");
//         // h_osc->topic_ready = false;
//         break;
//         case MQTT_EVENT_UNSUBSCRIBE:
//             Log_i("OTA topic has been unsubscribed");
//         // h_osc->topic_ready = false;
//         ;
//         break;
//         case MQTT_EVENT_CLIENT_DESTROY:
//             Log_i("mqtt client has been destroyed");
//         // h_osc->topic_ready = false;
//         ;
//         break;
//         default:
//             return;
//     }
// }

int32_t tm_sub_custom_topic(void *handler, const char *topic_suffix) {
    // 订阅 topic
    iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) handler;
    struct aws_string *topic_suffix_string = aws_string_new_from_c_str(
        dm_handle->allocator, topic_suffix);
    struct aws_string *product_key = aws_string_new_from_c_str(dm_handle->allocator, 
        dm_handle->mqtt_handle->config->basic_config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(dm_handle->allocator, 
        dm_handle->mqtt_handle->config->basic_config->device_name);    
    char *topic = iot_get_topic_with_1_param(dm_handle->allocator, 
        "sys/%s/%s/custom/%s", product_key, device_name, topic_suffix_string);

    iot_mqtt_subscribe(dm_handle->mqtt_handle, &(iot_mqtt_topic_map_t){
        .topic = topic,
        .message_callback = _tm_recv_custom_topic,
        // .event_callback = _custom_topic_mqtt_event_callback;
        .user_data = (void*)dm_handle,
        .qos = IOT_MQTT_QOS1,
    });

    aws_mem_release(dm_handle->allocator, topic);
    aws_string_destroy_secure(product_key);
    aws_string_destroy_secure(device_name);
    aws_string_destroy_secure(topic_suffix_string);

    return 0;
}

void _tm_recv_custom_topic(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
    struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
    struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

    LOGI(TAG_IOT_MQTT, "_tm_recv_custom_topic call topic = %.*s,  payload = %.*s", 
        AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
        AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
    iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) pUserData;
    if (NULL == dm_handle->recv_handler) {
        return;
    }

    iot_tm_recv_t recv = {0};
    recv.type = IOT_TM_RECV_CUSTOM_TOPIC;

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

    struct aws_byte_cursor custom_topic_cur = {0};
    aws_array_list_get_at(&topic_split_data_list, &custom_topic_cur, 4);

    // 数据封装
    iot_tm_recv_custom_topic_t custom_topic_data = {0};
    custom_topic_data.params_json_str = aws_cur_to_char_str(dm_handle->allocator, &payload_byte_cursor);
    custom_topic_data.custom_topic_suffix = aws_cur_to_char_str(dm_handle->allocator, &custom_topic_cur);
    recv.data.custom_topic = custom_topic_data;


    // 回调给业务
    dm_handle->recv_handler(dm_handle, &recv, dm_handle->userdata);
    aws_mem_release(dm_handle->allocator, custom_topic_data.params_json_str);
    aws_mem_release(dm_handle->allocator, recv.product_key);
    aws_mem_release(dm_handle->allocator, recv.device_name);
    aws_array_list_clean_up(&topic_split_data_list);
}
#endif