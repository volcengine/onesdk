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
#include "iot_log.h"
#include "thing_model/iot_tm_api.h"
#include "thing_model/iot_tm_header.h"
#include "thing_model/device_delay.h"
#include "iot/iot_utils.h"
#include "error_code.h"

// static void _device_delay_mqtt_event_callback(void *pclient, MQTTEventType event_type, void *user_data)
// {
//     switch (event_type) {
//         case MQTT_EVENT_SUBCRIBE_SUCCESS:
//             Log_i("delay topic subscribe success");
//         // h_osc->topic_ready = true;
//         break;

//         case MQTT_EVENT_SUBCRIBE_TIMEOUT:
//             Log_i("delay topic subscribe timeout");
//         // h_osc->topic_ready = false;
//         break;

//         case MQTT_EVENT_SUBCRIBE_NACK:
//             Log_i("delay topic subscribe NACK");
//         // h_osc->topic_ready = false;
//         break;
//         case MQTT_EVENT_UNSUBSCRIBE:
//             Log_i("delay topic has been unsubscribed");
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

int _sub_device_delay_info(void *handler) {
    iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) handler;
    struct aws_string *product_key = aws_string_new_from_c_str(dm_handle->allocator, 
        dm_handle->mqtt_handle->config->basic_config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(dm_handle->allocator, 
        dm_handle->mqtt_handle->config->basic_config->device_name);    
    char *topic = iot_get_common_topic(dm_handle->allocator, "sys/%s/%s/delay/+/post", 
        product_key, device_name);

    iot_mqtt_subscribe(dm_handle->mqtt_handle, &(iot_mqtt_topic_map_t){
        .topic = topic,
        .message_callback = _tm_recv_device_delay_info,
        // .event_callback = _device_delay_mqtt_event_callback;
        .user_data = (void*)dm_handle,
        .qos = IOT_MQTT_QOS1,
    });
    aws_string_destroy_secure(product_key);
    aws_string_destroy_secure(device_name);
    aws_mem_release(dm_handle->allocator, topic);
    return VOLC_OK;
}

void _tm_recv_device_delay_info(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
    struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
    struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

    LOGD(TAG_IOT_MQTT, "_tm_recv_device_delay_info call topic = %.*s,  payload = %.*s", 
        AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
        AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
    iot_tm_handler_t *dm_handle = pUserData;

    struct aws_array_list topic_split_data_list;
    aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 4, sizeof(struct aws_byte_cursor));
    aws_byte_cursor_split_on_char(&topic_byte_cursor, '/', &topic_split_data_list);

    struct aws_byte_cursor uuid_cur = {0};
    aws_array_list_get_at(&topic_split_data_list, &uuid_cur, 4);
    _tm_send_device_delay_reply_data(dm_handle, &uuid_cur, payload_byte_cursor);
    aws_array_list_clean_up(&topic_split_data_list);
}

int32_t _tm_send_device_delay_reply_data(void *handler, struct aws_byte_cursor *uuid_cur, struct aws_byte_cursor payload) {
    iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) handler;
    int ret = VOLC_OK;

    struct aws_string *uuid_string = aws_string_new_from_cursor(dm_handle->allocator, uuid_cur);
    struct aws_string *product_key = aws_string_new_from_c_str(dm_handle->allocator, 
        dm_handle->mqtt_handle->config->basic_config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(dm_handle->allocator, 
        dm_handle->mqtt_handle->config->basic_config->device_name);    
    char *reply_topic = iot_get_topic_with_1_param(dm_handle->allocator, "sys/%s/%s/delay/%s/post_reply",
                                                    product_key, device_name,
                                                    uuid_string);

    
    iot_mqtt_publish(dm_handle->mqtt_handle, reply_topic, (uint8_t *)payload.ptr, payload.len, IOT_MQTT_QOS1);


    // 回收内存数据
    aws_mem_release(dm_handle->allocator, reply_topic);
    aws_string_destroy_secure(uuid_string);
    aws_string_destroy_secure(product_key);
    aws_string_destroy_secure(device_name);
    return ret;
}
#endif