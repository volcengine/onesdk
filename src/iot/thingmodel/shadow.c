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
#include "thing_model/shadow.h"
#include "iot/iot_utils.h"
#include "iot_log.h"
#include "util/util.h"
#include "util/aws_json.h"
#include "error_code.h"
#include "thing_model/iot_tm_api.h"
#include "thing_model/iot_tm_header.h"

// shadow report
/*
 * {
        "id":"",
        "version":1,
        "params":{
            "version":12313123,
            "report":{
                "key":"value"
            }
         }
    }
 */
void iot_shadow_post_init(iot_tm_msg_shadow_post_t** pty) {
    iot_shadow_post_init_with_id(pty, NULL);
}

// why no const
void iot_shadow_post_init_with_id(iot_tm_msg_shadow_post_t** pty, const char* id) {
    iot_tm_msg_shadow_post_t* shadowP = aws_mem_calloc(aws_alloc(),1, sizeof(iot_tm_msg_shadow_post_t));
    const char* real_id = id;
    if (real_id == NULL) {
        real_id = get_random_string_with_time_suffix(aws_alloc());
    }
    shadowP->id = real_id;
    shadowP->version = SDK_VERSION;
    shadowP->params = (void*) aws_json_value_new_object(aws_alloc());
    shadowP->report = (void*) aws_json_value_new_object(aws_alloc());
    *pty = shadowP;
}

void iot_shadow_post_add_param_num(iot_tm_msg_shadow_post_t* pty, const char* key, double value) {
    aws_json_add_num_val((struct aws_json_value*) pty->report, key, value);
}

void iot_shadow_post_add_param_object(iot_tm_msg_shadow_post_t* pty, const char* key, struct aws_json_value* value) {
    aws_json_add_json_obj((struct aws_json_value*) pty->report, key, value);
}

void iot_shadow_post_add_param_json_str(iot_tm_msg_shadow_post_t* pty, const char* key, const char* json_val) {
    struct aws_json_value* json_data = aws_json_value_new_from_string(aws_alloc(),
                                                                      aws_byte_cursor_from_c_str(json_val));
    iot_shadow_post_add_param_object(pty, key, json_data);
}

void iot_shadow_post_add_param_string(iot_tm_msg_shadow_post_t* pty, const char* key, const char* value) {
    aws_json_add_str_val((struct aws_json_value*) pty->report, key, value);
}

void* iot_shadow_post_payload(iot_tm_msg_shadow_post_t* pty) {
    if (pty->payload_root == NULL) {
        pty->payload_root = (void*) aws_json_value_new_object(aws_alloc());
        aws_json_add_str_val((struct aws_json_value*) pty->payload_root, "id", pty->id);
        aws_json_add_str_val((struct aws_json_value*) pty->payload_root, "version", pty->version);
        aws_json_add_num_val((struct aws_json_value*) pty->params, "version", (double) get_current_time_mil());
        aws_json_add_json_obj((struct aws_json_value*) pty->params, "report", (struct aws_json_value*) pty->report);
        aws_json_add_json_obj((struct aws_json_value*) pty->payload_root, "params",(struct aws_json_value*) pty->params);
    }
    return pty->payload_root;
}

void iot_shadow_post_free(iot_tm_msg_shadow_post_t* pty) {
    if (pty == NULL) {
        return;
    }
    if (pty->payload_root != NULL) {
        aws_json_value_destroy((struct aws_json_value*) pty->payload_root);
    }
    if (pty->params != NULL) {
        aws_json_value_destroy((struct aws_json_value*) pty->params);
    }
    if (pty->id != NULL) {
        aws_mem_release(aws_alloc(), (void*)pty->id);
    }
    aws_mem_release(aws_alloc(), pty);
}

int32_t _tm_send_shadow_post(void* handler, const char* topic, const void* msg_p) {
    iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
    iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
    int32_t ret = VOLC_OK;
    struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*)iot_shadow_post_payload(msg->data.shadow_post)));
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);

    iot_mqtt_publish(dm_handle->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);
    LOGD(TAG_IOT_MQTT, "_tm_send_shadow_post call topic = %s,  payload = %.*s", topic,
         AWS_BYTE_CURSOR_PRI(payload_cur));
    aws_byte_buf_clean_up(&payload_buf);

    return ret;
}

// shadow get
void iot_shadow_get_init(iot_tm_msg_shadow_get_t** pty){
    iot_shadow_get_init_with_id(pty, NULL);
}

void iot_shadow_get_init_with_id(iot_tm_msg_shadow_get_t** pty, const char* id) {
    iot_tm_msg_shadow_get_t* shadowP = malloc(sizeof(iot_tm_msg_shadow_get_t));
    const char* real_id = id;
    if (real_id == NULL) {
        real_id = get_random_string_id_c_str(aws_alloc());
    }
    shadowP->id = real_id;
    shadowP->version = SDK_VERSION;
    *pty = shadowP;
}


void* iot_shadow_get_payload(iot_tm_msg_shadow_get_t* pty) {
    if (pty->payload_root == NULL) {
        pty->payload_root = aws_json_value_new_object(aws_alloc());
        aws_json_add_str_val( (struct aws_json_value*) pty->payload_root, "id",pty->id);
        aws_json_add_str_val( (struct aws_json_value*) pty->payload_root, "version", pty->version);
    }
    return pty->payload_root;
//    return aws_json_obj_to_byte_buf(aws_alloc(), pty->payload_root);
}

void iot_shadow_get_free(iot_tm_msg_shadow_get_t* pty) {
    if (pty == NULL) {
        return;
    }
    if (pty->payload_root != NULL) {
        aws_json_value_destroy(pty->payload_root);
    }
    aws_mem_release(aws_alloc(), (void*)pty->id);
    aws_mem_release(aws_alloc(), pty);
}

int32_t _tm_send_shadow_get(void* handler, const char* topic, const void* msg_p) {
    iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
    iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
    int32_t ret = VOLC_OK;
    struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*)iot_shadow_get_payload(msg->data.shadow_get)));
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);

    iot_mqtt_publish(dm_handle->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);
    LOGD(TAG_IOT_MQTT, "_tm_send_shadow_get call topic = %s,  payload = %.*s", topic,
         AWS_BYTE_CURSOR_PRI(payload_cur));
    aws_byte_buf_clean_up(&payload_buf);
    return ret;
}

void _tm_recv_shadow_get_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
    struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
    struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

    LOGD(TAG_IOT_MQTT, "_tm_recv_shadow_get_reply_handler call topic = %.*s,  payload = %.*s", 
        AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
        AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
    iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
    if (dm_handle == NULL) {
        return;
    }
    if (NULL == dm_handle->recv_handler) {
        return;
    }
    iot_tm_recv_t recv;
    AWS_ZERO_STRUCT(recv);
    recv.type = IOT_TM_RECV_SHADOW_GET_REPLY;

    // parse product_key, device_name, module_key by topic value
    struct aws_array_list topic_split_data_list;
    aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
    aws_byte_cursor_split_on_char(&topic_byte_cursor, '/',&topic_split_data_list);

    struct aws_byte_cursor product_key_cur = {0};
    aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
    struct aws_byte_cursor device_name_cur = {0};
    aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
    recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);
    recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

    // package business data
    iot_tm_recv_shadow_get_reply_t shadow_get_data;
    struct aws_json_value* payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload_byte_cursor);
    double error_code = aws_json_get_num_val(payload_json, "Code");
    if (error_code != 0) {
        return;
    }
    struct aws_byte_cursor id_cur = aws_json_get_str_byte_cur_val(payload_json, "ID");
    struct aws_json_value* data_json = aws_json_get_json_obj(dm_handle->allocator, payload_json, "Data");
    double version = aws_json_get_num_val(data_json, "Version");
    struct aws_byte_buf desired_buf = aws_json_get_json_obj_to_bye_buf(dm_handle->allocator, data_json, "Desired");
    shadow_get_data.msg_id = aws_cur_to_char_str(dm_handle->allocator, &id_cur);
    shadow_get_data.version = (int64_t) version;
    shadow_get_data.desired_json_str = (char*) desired_buf.buffer;

    recv.data.shadow_get_reply = shadow_get_data;

    // callback business
    dm_handle->recv_handler(dm_handle, &recv, dm_handle->userdata);

    // release
    aws_mem_release(dm_handle->allocator, shadow_get_data.msg_id);
    aws_mem_release(dm_handle->allocator, recv.product_key);
    aws_mem_release(dm_handle->allocator, recv.device_name);
    aws_mem_release(dm_handle->allocator, payload_json);
    aws_mem_release(dm_handle->allocator, shadow_get_data.desired_json_str);

    // business need to send clear msg
    iot_mqtt_ctx_t* mqtt_handler = dm_handle->mqtt_handle;
    if (mqtt_handler == NULL) {
        return;
    }

    // send clear
    __send_shadow_clear(dm_handle);
}

// clear
void iot_shadow_clear_init(iot_tm_msg_shadow_clear_post_t** pty){
    iot_shadow_clear_init_with_id(pty, NULL);
}

void iot_shadow_clear_init_with_id(iot_tm_msg_shadow_clear_post_t** pty, const char* id){
    iot_tm_msg_shadow_clear_post_t* clearP = aws_mem_calloc(aws_alloc(),1, sizeof(iot_tm_msg_shadow_clear_post_t));
    const char* real_id = id;
    if (real_id == NULL) {
        real_id = get_random_string_id_c_str(aws_alloc());
    }
    clearP->id = real_id;
    clearP->version = SDK_VERSION;
    clearP->shadow_version = (int64_t) get_current_time_mil();
    *pty = clearP;
}

void iot_shadow_clear_free(iot_tm_msg_shadow_clear_post_t* pty) {
    if (pty == NULL) {
        return;
    }
    if (pty->payload_root != NULL) {
        aws_json_value_destroy(pty->payload_root);
    }

    aws_mem_release(aws_alloc(), (void*)pty->id);
    aws_mem_release(aws_alloc(), pty);
}

void* iot_shadow_clear_payload(iot_tm_msg_shadow_clear_post_t* pty){
    if (pty->payload_root == NULL) {
        pty->payload_root = aws_json_value_new_object(aws_alloc());
        char* clear_id = get_random_string_with_time_suffix(aws_alloc());
        aws_json_add_str_val( (struct aws_json_value*) pty->payload_root, "ID", clear_id);
        aws_json_add_str_val( (struct aws_json_value*) pty->payload_root, "Version",SDK_VERSION);
        aws_json_add_num_val( (struct aws_json_value*) pty->payload_root, "Params", (double)get_current_time_mil());
    }
    return pty->payload_root;
}

int32_t _tm_send_shadow_clear(void* handler, const char* topic, const void* msg_p) {
    iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
    iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
    int32_t ret = VOLC_OK;
    struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*)iot_shadow_clear_payload(msg->data.shadow_clear)));
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);

    iot_mqtt_publish(dm_handle->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);
    LOGD(TAG_IOT_MQTT, "_tm_send_shadow_clear call topic = %s,  payload = %.*s", topic,
         AWS_BYTE_CURSOR_PRI(payload_cur));
    aws_byte_buf_clean_up(&payload_buf);
    return ret;

}

void _tm_recv_shadow_set_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
    struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
    struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

    LOGD(TAG_IOT_MQTT, "_tm_recv_shadow_set_handler call topic = %.*s,  payload = %.*s", 
        AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
        AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
    iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
    if (dm_handle == NULL) {
        return;
    }
    if (NULL == dm_handle->recv_handler) {
        return;
    }
    iot_tm_recv_t recv;
    AWS_ZERO_STRUCT(recv);
    recv.type = IOT_TM_RECV_SHADOW_SET;

    // parse product_key, device_name, module_key by topic value
    struct aws_array_list topic_split_data_list;
    aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
    aws_byte_cursor_split_on_char(&topic_byte_cursor, '/',&topic_split_data_list);

    struct aws_byte_cursor product_key_cur = {0};
    aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
    struct aws_byte_cursor device_name_cur = {0};
    aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
    recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);
    recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

    // package business data
    iot_tm_recv_shadow_set_t shadow_set_data;
    struct aws_json_value* payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload_byte_cursor);
//    struct aws_byte_cursor id_cur = aws_json_get_str_byte_cur_val(payload_json, "ID");
    double version = aws_json_get_num_val(payload_json, "Version");
    struct aws_json_value* params_json = aws_json_get_json_obj(dm_handle->allocator, payload_json, "Params");
    shadow_set_data.shadow_version = (int64_t) version;
    struct aws_byte_buf desired_buf = aws_json_get_json_obj_to_bye_buf(dm_handle->allocator, payload_json, "Desired");
    shadow_set_data.desired_json_str = (char*) desired_buf.buffer;
    recv.data.shadow_set = shadow_set_data;

    // callback business
    dm_handle->recv_handler(dm_handle, &recv, dm_handle->userdata);

    // release
    aws_mem_release(dm_handle->allocator, recv.product_key);
    aws_mem_release(dm_handle->allocator, recv.device_name);
    aws_mem_release(dm_handle->allocator, payload_json);
    aws_mem_release(dm_handle->allocator, shadow_set_data.desired_json_str);

}

void __send_shadow_clear(void* handle) {
    iot_tm_handler_t* dm_handle = (iot_tm_handler_t*)handle;
    iot_tm_msg_t dm_msg = {0};
    dm_msg.type = IOT_TM_MSG_SHADOW_CLEAR;
    iot_tm_msg_shadow_clear_post_t* shadow_clear;
    iot_shadow_clear_init(&shadow_clear);
    dm_msg.data.shadow_clear = shadow_clear;
    iot_tm_send(dm_handle, &dm_msg);
    iot_shadow_clear_free(shadow_clear);
}

void _tm_recv_shadow_report_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
    // TODO: implement me
}
#endif