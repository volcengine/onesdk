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
#include "thing_model/property.h"
#include "iot/iot_utils.h"
#include "iot_log.h"
#include "util/util.h"
#include "aws/common/json.h"
#include "aws/common/string.h"
#include "util/aws_json.h"
#include "error_code.h"

#include "thing_model/iot_tm_api.h"
#include "thing_model/iot_tm_header.h"

void iot_property_post_init(iot_tm_msg_property_post_t **pty) {
    iot_property_post_init_with_id(pty,get_random_string_id_c_str(aws_alloc()));
}

void iot_property_post_init_with_id(iot_tm_msg_property_post_t **pty,const char *id) {
    iot_tm_msg_property_post_t *propertyP = malloc( sizeof(iot_tm_msg_property_post_t));
    memset(propertyP, 0, sizeof(iot_tm_msg_property_post_t));
    propertyP->id = id;
    propertyP->version = SDK_VERSION;
    propertyP->params = (void*) aws_json_value_new_object(aws_alloc());
    *pty = propertyP;
}

void iot_property_post_add_param_num(iot_tm_msg_property_post_t *pty, const char *key, double value) {
    struct aws_json_value *params = aws_json_value_new_object(aws_alloc());
    aws_json_add_num_val(params, "value", value);
    aws_json_add_num_val(params, "time", (double) unix_timestamp_ms());
    aws_json_add_json_obj((struct aws_json_value*) pty->params, key, params);
}

void iot_property_post_add_param_object(iot_tm_msg_property_post_t *pty, const char *key, struct aws_json_value *value) {
    struct aws_json_value *value_data_json = aws_json_value_new_object(aws_alloc());
    aws_json_add_json_obj(value_data_json, "value", value);
    aws_json_add_num_val(value_data_json, "time", (double) unix_timestamp_ms());
    aws_json_add_json_obj((struct aws_json_value*) pty->params, key, value_data_json);
}

void iot_property_post_add_param_json_str(iot_tm_msg_property_post_t *pty, const char *key, const char *json_val) {
    struct aws_json_value *value_data_json = aws_json_value_new_object(aws_alloc());
    struct aws_json_value *json_data = aws_json_value_new_from_string(aws_alloc(), aws_byte_cursor_from_c_str(json_val));
    aws_json_add_json_obj(value_data_json, "value", json_data);
    aws_json_add_num_val(value_data_json, "time", (double) unix_timestamp_ms());
    aws_json_add_json_obj((struct aws_json_value*) pty->params, key, value_data_json);
}

void iot_property_post_add_param_string(iot_tm_msg_property_post_t *pty, const char *key, const char *value) {
    struct aws_json_value *value_data_json = aws_json_value_new_object(aws_alloc());
    aws_json_add_str_val_1(aws_alloc(), value_data_json, "value", value);
    aws_json_add_num_val(value_data_json, "time", (double) unix_timestamp_ms());
    aws_json_add_json_obj((struct aws_json_value*) pty->params, key, value_data_json);
}

void* iot_property_post_payload(iot_tm_msg_property_post_t *pty) {
    if (pty->payload_root == NULL) {
        pty->payload_root = aws_json_value_new_object(aws_alloc());
        aws_json_add_str_val_1(aws_alloc(), pty->payload_root, "id", pty->id);
        aws_json_add_str_val_1(aws_alloc(), (struct aws_json_value*) pty->payload_root, "version", pty->version);
        aws_json_add_json_obj((struct aws_json_value*) pty->payload_root, "params", pty->params);
    }
    return pty->payload_root;
//    return aws_json_obj_to_byte_buf(aws_alloc(), pty->payload_root);
}

void iot_property_post_free(iot_tm_msg_property_post_t *pty) {
    if (pty->payload_root != NULL) {
        aws_json_value_destroy(pty->payload_root);
    } else if (pty->params != NULL) {
        aws_json_value_destroy(pty->params);
    }
    aws_mem_release(aws_alloc(), (void*)pty->id);
    aws_mem_release(aws_alloc(), pty);
}

/**
 * 发送属性
 */
int32_t _tm_send_property_post(void *handler, const char *topic, const void *msg_p) {
    // 发送数据给服务端
    iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) handler;
    iot_tm_msg_t *msg = (iot_tm_msg_t *) msg_p;

    // 需要这种格式
    // //{"key":{"value", 123, "time" :123}}
    int ret = VOLC_OK;
    struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*) iot_property_post_payload(msg->data.property_post)));
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);

    iot_mqtt_publish(dm_handle->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);
    LOGD(TAG_IOT_MQTT, "_tm_send_property_post call topic = %s,  payload = %.*s", topic,
         AWS_BYTE_CURSOR_PRI(payload_cur));

    aws_byte_buf_clean_up(&payload_buf);
    return ret;
}


// 属性设置回复 接口初始化
void iot_property_set_post_reply_init(iot_tm_msg_property_set_post_reply_t **post_reply, const char *id, int32_t code) {
    iot_tm_msg_property_set_post_reply_t *reply = malloc( sizeof(iot_tm_msg_property_set_post_reply_t));
    AWS_ZERO_STRUCT(*reply);
    reply->id = id;
    reply->code = code;
    *post_reply = reply;
}

void* iot_property_set_post_reply_payload(iot_tm_msg_property_set_post_reply_t *pty) {
    if (pty->payload_root == NULL) {
        pty->payload_root = aws_json_value_new_object(aws_alloc());
        aws_json_add_str_val_1(aws_alloc(), pty->payload_root, "id", pty->id);
        aws_json_add_num_val((struct aws_json_value*) pty->payload_root, "code", pty->code);
    }
    return pty->payload_root;
}


void iot_property_set_post_reply_free(iot_tm_msg_property_set_post_reply_t *pty) {
    if (pty->payload_root != NULL) {
        aws_json_value_destroy(pty->payload_root);
    }
    aws_mem_release(aws_alloc(), pty);
}


void __tm_send_server_property_set_reply(void *iot_tm_handler, const char* msg_id) {
    iot_tm_handler_t *tm = (iot_tm_handler_t *) iot_tm_handler;
    iot_tm_msg_t dm_msg = {0};
    dm_msg.type = IOT_TM_MSG_PROPERTY_SET_REPLY;
    iot_tm_msg_property_set_post_reply_t *post_reply;
    iot_property_set_post_reply_init(&post_reply, msg_id, 0);
    dm_msg.data.property_set_post_reply = post_reply;
    iot_tm_send(tm, &dm_msg);
    iot_property_set_post_reply_free(post_reply);
}

/**
 * 回复服务端下发的属性设置
 */
int32_t _tm_send_property_set_post_reply(void *handler, const char *topic, const void *msg_p) {
    iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) handler;
    iot_tm_msg_t *msg = (iot_tm_msg_t *) msg_p;

    // 需要这种格式
    // //{"key":{"value", 123, "time" :123}}
    int ret = VOLC_OK;
    struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, iot_property_set_post_reply_payload(msg->data.property_set_post_reply));
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
    iot_mqtt_publish(dm_handle->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);
    LOGD(TAG_IOT_MQTT, "_tm_send_property_set_post_reply call topic = %s,  payload = %.*s", topic,
         AWS_BYTE_CURSOR_PRI(payload_cur));

    return ret;
}



/**
 * 接收服务端下发的属性设置
 */
void _tm_recv_property_set_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
    struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
    struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);
    // "{\"ID\":\"6388d1a24a621d7833716e3f\",\"Version\":\"1.0.0\",\"Params\":{\"default:ErrorCurrentThreshold\":0.1,\"default:ErrorPowerThreshold\":0}}";
    LOGD(TAG_IOT_MQTT, "_tm_recv_property_set_handler call topic = %.*s,  payload = %.*s", 
        AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
        AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
    // 回调给业务
    iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) pUserData;
    if (NULL == dm_handle->recv_handler) {
        return;
    }
    iot_tm_recv_t recv;
    AWS_ZERO_STRUCT(recv);
    recv.type = IOT_TM_RECV_PROPERTY_SET;

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


    // 数据封装
    iot_tm_recv_property_set_t property_set_data;
    AWS_ZERO_STRUCT(property_set_data);
    struct aws_json_value *payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload_byte_cursor);
    struct aws_string* id_cur = aws_json_get_string1_val(dm_handle->allocator,payload_json, "ID");
    struct aws_byte_buf param_buf = aws_json_get_json_obj_to_bye_buf(dm_handle->allocator, payload_json, "Params");
    property_set_data.params = (char *) param_buf.buffer;
    property_set_data.params_len = param_buf.len;
    property_set_data.msg_id = (char*)aws_string_c_str(id_cur);

    // 回调给业务
    recv.data.property_set = property_set_data;
    dm_handle->recv_handler(dm_handle, &recv, dm_handle->userdata);

    // reply server
    __tm_send_server_property_set_reply(dm_handle, property_set_data.msg_id);


    // 回收内存
    aws_mem_release(dm_handle->allocator, id_cur);
    aws_mem_release(dm_handle->allocator, recv.product_key);
    aws_mem_release(dm_handle->allocator, recv.device_name);
    aws_byte_buf_clean_up(&param_buf);
    aws_json_value_destroy(payload_json);
    aws_array_list_clean_up(&topic_split_data_list);


}

/**
 * 接收服务端下发的 属性设置回复
 */
void _tm_recv_property_set_post_reply(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
    struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
    struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

    // payload = {"ID":"2301669963423725","Code":0,"Data":{}}
    LOGD(TAG_IOT_MQTT, "_tm_recv_property_set_post_reply call topic = %.*s,  payload = %.*s", 
        AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
        AWS_BYTE_CURSOR_PRI(payload_byte_cursor));

    iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) pUserData;
    if (NULL == dm_handle->recv_handler) {
        return;
    }
    iot_tm_recv_t recv;
    AWS_ZERO_STRUCT(recv);
    recv.type = IOT_TM_RECV_PROPERTY_SET_POST_REPLY;

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


    // 数据封装
    iot_tm_recv_property_set_post_reply post_reply;
    struct aws_json_value *payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload_byte_cursor);
    struct aws_byte_cursor id_cur = aws_json_get_str_byte_cur_val(payload_json, "ID");
    double code = aws_json_get_num_val(payload_json, "Code");

    post_reply.msg_id = aws_cur_to_char_str(dm_handle->allocator, &id_cur);
    post_reply.code = (int32_t) code;
    recv.data.property_set_post_reply = post_reply;

    // 回调给业务
    dm_handle->recv_handler(dm_handle, &recv, dm_handle->userdata);

    // 回收内存
    aws_mem_release(dm_handle->allocator, post_reply.msg_id);
    aws_mem_release(dm_handle->allocator, recv.product_key);
    aws_mem_release(dm_handle->allocator, recv.device_name);
    aws_json_value_destroy(payload_json);
    aws_array_list_clean_up(&topic_split_data_list);
}


#endif
