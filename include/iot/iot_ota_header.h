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

#ifndef ONESDK_IOT_IOT_OTA_HEADER_H
#define ONESDK_IOT_IOT_OTA_HEADER_H
#ifdef ONESDK_ENABLE_IOT

#include "iot/iot_ota_api.h"
#include "iot_ota_utils.h"

#include "aws/common/hash_table.h"
#include "aws/common/json.h"

typedef void (http_download_callback)(void *download_handler, struct http_download_file_response *ctx, void *user_data);
typedef void (http_download_rev_data_callback)(void *download_handler, uint8_t *data_prt, size_t len, int32_t percent, void *user_data);

// TODO 待完善
typedef struct iot_http_download_handler {
    const char* download_file_path;
    size_t download_file_size;
    size_t downloaded_size;
} iot_http_download_handler_t;

void _callback_download_response(iot_http_download_handler_t *download_handler);

int _write_down_file(iot_http_download_handler_t *download_handler, const struct aws_byte_cursor *data);

typedef struct iot_ota_handler {
    iot_mqtt_ctx_t *mqtt_handle;
    void *mqtt_client;

    struct aws_allocator *allocator;
    void *lock;
    struct iot_kv_ctx *kv_ctx;
    iot_ota_device_info_t *device_info_array;
    int32_t device_info_array_size;
    char *download_dir;
    int32_t auto_request_ota_info_interval_sec;

    iot_ota_get_job_info_callback *get_job_info_callback;
    void *get_job_info_callback_user_data;

    iot_ota_download_complete_callback *ota_download_complete_callback;
    void *ota_download_complete_callback_user_data;

    iot_ota_rev_data_progress_callback *iot_ota_rev_data_progress_callback;
    void *iot_ota_rev_data_progress_callback_user_data;

    struct aws_hash_table task_hash_map;
} iot_ota_handler_t;

typedef struct ota_process_status {
    enum ota_upgrade_device_status_enum upgrade_device_status;
    int32_t error_code;
    char *result_desc;
} ota_process_status_t;

typedef struct iot_ota_job_task_info {
    iot_ota_handler_t *ota_handler;
    iot_ota_job_info_t *server_job_info;
    struct aws_string *decode_url;  // 从aws_string转换成char *
    // iot_http_download_handler_t *download_handler;

    int retry_time;
    bool is_pending_to_retry; // 等待重试
    enum ota_upgrade_device_status_enum upgrade_device_status;
    char *ota_file_path;
    size_t download_file_size;
    size_t downloaded_size;
    // 下载完成, 安装失败如何出? 是否需要序列化? 先不考虑
} iot_ota_job_task_info_t;

iot_ota_job_info_t *_ota_data_json_to_ota_job_info(struct aws_allocator *allocator, struct aws_json_value *data_json);

void _mqtt_sub_ota_info(iot_ota_handler_t *handle);

void _ota_rev_ota_notify_info(const char* topic, const uint8_t *payload, size_t len, void *user_data);

void _mqtt_sub_ota_upgrade_post_reply(iot_ota_handler_t *ota_handler);

void _ota_rev_ota_upgrade_post_reply(const char* topic, const uint8_t *payload, size_t len, void *user_data);

void job_info_release(iot_ota_handler_t *handler, iot_ota_job_info_t *job_info);

void iot_ota_save_job_task_info(iot_ota_job_task_info_t *task_info);
void iot_ota_delete_job_task_info(iot_ota_job_task_info_t *task_info);

int32_t iot_ota_report_version(iot_ota_handler_t *handler, iot_ota_device_info_t *device_info_array, int device_info_array_size);

int32_t iot_ota_report_progress(iot_ota_handler_t *handler, char *jobId, ota_process_status_t *status);

int32_t iot_ota_report_progress_success(iot_ota_handler_t *handler, char *jobId, enum ota_upgrade_device_status_enum upgrade_device_status);

int32_t iot_ota_report_progress_failed(iot_ota_handler_t *handler, char *jobId, enum ota_upgrade_device_status_enum upgrade_device_status, int32_t error_code, char *result_desc);

void request_device_job_info_inner(iot_ota_handler_t *handler);

#endif //ONESDK_IOT_IOT_OTA_HEADER_H
#endif //ONESDK_ENABLE_IOT