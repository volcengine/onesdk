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

#include "iot/log_report.h"
#include "iot_log.h"
#include "error_code.h"
#include "util/aws_json.h"
#include "util/util.h"
#include "iot/iot_utils.h"

// 实时流 超时时间
#define STREAM_LOG_TIME_OUT_MIL 3 * 60 * 60

// 上报 日志
static void s_common_log_report(log_handler_t *log_handler, struct aws_array_list *pending_log_lines) {
    if (log_handler->log_report_switch != true) {
        // 开关开启
        return;
    }

    struct aws_json_value *logReportJson = aws_json_value_new_object(log_handler->allocator);

    char *id_string = get_random_string_with_time_suffix(log_handler->allocator);
    aws_json_add_str_val_1(log_handler->allocator, logReportJson, "id", id_string);
    aws_mem_release(log_handler->allocator, id_string);


    aws_json_add_str_val_1(log_handler->allocator, logReportJson, "version", SDK_VERSION);
    struct aws_json_value *data = aws_json_value_new_array(log_handler->allocator);
    size_t line_count = aws_array_list_length(pending_log_lines);
    int real_count = 0;

    for (size_t i = 0; i < line_count; ++i) {
        struct iot_log_obj *logObj = NULL;
        AWS_FATAL_ASSERT(aws_array_list_get_at(pending_log_lines, &logObj, i) == AWS_OP_SUCCESS);
        if (logObj == NULL) {
            continue;
        }

        if (logObj->log_level > log_handler->lowest_level) {
            // 不符合要求的 日志 不需要
            continue;
        }

        real_count++;
        struct aws_json_value *iten_json = aws_json_value_new_object(log_handler->allocator);
        aws_json_add_num_val1(log_handler->allocator, iten_json, "CreateTime", logObj->time);
        aws_json_add_str_val_1(log_handler->allocator, iten_json, "LogLevel", logObj->logLevelStr);
        aws_json_add_str_val_1(log_handler->allocator, iten_json, "Type", logObj->logType);
        aws_json_add_aws_string_val1(log_handler->allocator, iten_json, "Content", logObj->logContent);
        aws_json_value_add_array_element(data, iten_json);
    }


    if (real_count <= 0) {
        // 没有符合要求的日志,  忽略此次上报
        aws_json_value_destroy(logReportJson);
        return;
    }

    aws_json_value_add_to_object(logReportJson, aws_byte_cursor_from_c_str("data"), data);
    struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(log_handler->allocator, logReportJson);
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);

    struct aws_string *product_key = aws_string_new_from_c_str(log_handler->allocator, 
        log_handler->mqtt_handle->config->basic_config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(log_handler->allocator, 
        log_handler->mqtt_handle->config->basic_config->device_name);
    char * topic = iot_get_common_topic(log_handler->allocator, "sys/%s/%s/log/batch/report", product_key, device_name);

    iot_mqtt_publish(log_handler->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);

    aws_byte_buf_clean_up(&payload_buf);
    aws_json_value_destroy(logReportJson);
    aws_mem_release(log_handler->allocator, topic);
    aws_string_destroy(product_key);
    aws_string_destroy(device_name);
}

// 日志流处理
void s_log_report_on_log_save_fn(struct aws_array_list *log_lines, void *userdata) {
    // 普通日志上报
    log_handler_t *log_handler = (log_handler_t *) userdata;
    lws_pthread_mutex_lock(&log_handler->lock);
    s_common_log_report(log_handler, log_lines);
    lws_pthread_mutex_unlock(&log_handler->lock);
}

log_handler_t *aiot_log_init(void) {
    struct aws_allocator *allocator = aws_alloc();
    aws_common_library_init(allocator);
    log_handler_t *log_handler = aws_mem_acquire(allocator, sizeof(log_handler_t));
    AWS_ZERO_STRUCT(*log_handler);
    log_handler->allocator = allocator;
    log_handler->log_report_switch = false;
    log_handler->lowest_level = DEBUG;
    // log_handler->stream_id_config_map = HAL_Malloc(sizeof(struct aws_hash_table));
    // aws_hash_table_init(log_handler->stream_id_config_map, log_handler->allocator, 10, aws_hash_c_string,
    //                     aws_hash_callback_c_str_eq, NULL, NULL);
    lws_pthread_mutex_init(&log_handler->lock);
    iot_log_set_on_log_save_fn(s_log_report_on_log_save_fn, (void*)log_handler);
    return log_handler;
}

void aiot_log_deinit(log_handler_t *handle) {
    if (handle == NULL) {
        return;
    }

    lws_pthread_mutex_destroy(&handle->lock);
}

static void s_log_rev_log_report_config(const char* topic, const uint8_t *payload, size_t len, void *user_data) {
    struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
    struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

    LOGD(TAG_IOT_MQTT, "s_log_rev_log_report_config call topic = %.*s  payload = %.*s",
         AWS_BYTE_CURSOR_PRI(topic_byte_cursor),
         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
//    {"id":"a6b7c2dd221209150431","version":"1.0","data":{"Switch":true,"LowestLevel":"debug"}}

    log_handler_t *log_handler = (log_handler_t *) user_data;
    lws_pthread_mutex_lock(&log_handler->lock);

    struct aws_json_value *payload_json = aws_json_value_new_from_string(log_handler->allocator, payload_byte_cursor);
    struct aws_json_value *data_json = aws_json_value_get_from_object(payload_json, aws_byte_cursor_from_c_str("data"));
    bool log_switch = aws_json_get_bool_val(data_json, "Switch");
    LOGD(TAG_IOT_MQTT, "s_log_rev_log_report_config call log_switch = %d ", log_switch);
    log_handler->log_report_switch = log_switch;
    struct aws_byte_cursor lowest_level_cur = aws_json_get_str_byte_cur_val(data_json, "LowestLevel");
    log_handler->lowest_level = (enum onesdk_log_level) _log_string_to_level(&lowest_level_cur);
    aws_json_value_destroy(payload_json);
    lws_pthread_mutex_unlock(&log_handler->lock);
}

static void s_sub_log_report_config_topic(log_handler_t *handler) {
    log_handler_t *log_handle = (log_handler_t *) handler;
    if (log_handle->mqtt_handle == NULL) {
        return;
    }
    struct aws_string *product_key = aws_string_new_from_c_str(handler->allocator, 
        handler->mqtt_handle->config->basic_config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(handler->allocator, 
        handler->mqtt_handle->config->basic_config->device_name);
    char *topic = iot_get_common_topic(log_handle->allocator, "sys/%s/%s/log/batch/config", 
        product_key, device_name);
    // iot_mqtt_sub(log_handle->mqtt_handle, topic, 1, s_log_rev_log_report_config, log_handle);
    
    iot_mqtt_subscribe(log_handle->mqtt_handle, &(iot_mqtt_topic_map_t){
        .topic = topic,
        .message_callback = s_log_rev_log_report_config,
        // .event_callback = _iot_mqtt_event_callback,
        .user_data = (void*)log_handle,
        .qos = IOT_MQTT_QOS1,
    });
    aws_mem_release(log_handle->allocator, topic);
    aws_string_destroy_secure(product_key);
    aws_string_destroy_secure(device_name);
}

int aiot_log_set_mqtt_handler(log_handler_t *handle, iot_mqtt_ctx_t *mqtt_handle) {
    int ret = 0;
    handle->mqtt_handle = mqtt_handle;
    s_sub_log_report_config_topic(handle);
    return VOLC_OK;
}

void aiot_log_set_report_switch(log_handler_t *handle, bool is_upload_log, enum onesdk_log_level lowest_level) {
    handle->lowest_level = lowest_level;
    handle->log_report_switch = is_upload_log;
}
#endif