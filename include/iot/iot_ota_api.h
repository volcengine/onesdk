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

#ifndef ONESDK_IOT_IOT_OTA_API_H
#define ONESDK_IOT_IOT_OTA_API_H
#ifdef ONESDK_ENABLE_IOT

#include <stdint.h>
#include "iot_mqtt.h"

#define TAG_OTA "ota"

#define OTA_RETRY_INTERVAL_SEC  5
#define OTA_AUTO_REQUEST_INFO_INTERVAL_SEC  600


typedef struct {
    char *module;
    char *version;
} iot_ota_device_info_t;

typedef struct {
    char *ota_job_id;
    int32_t timeout_in_minutes;
    uint64_t size;
    char *dest_version;
    char *url;
    char *module;
    char *sign;
    char *file_path;
} iot_ota_job_info_t;


typedef void(iot_ota_get_job_info_callback)(void *iot_ota_handler_t, iot_ota_job_info_t *job_info, void *user_data);

typedef void(iot_ota_download_complete_callback)(void *iot_ota_handler_t,
                                                 int error_code, iot_ota_job_info_t *job_info,
                                                 const char *ota_file_path,
                                                 void *user_data);

typedef void(iot_ota_rev_data_progress_callback)(void *iot_ota_handler_t, iot_ota_job_info_t *job_info, uint8_t *data_prt, size_t len, int32_t percent, void *user_data);

typedef struct iot_ota_handler iot_ota_handler_t;

typedef struct ota_process_status ota_process_status_t;

typedef struct iot_ota_job_task_info iot_ota_job_task_info_t;


iot_ota_handler_t *iot_ota_init(void);

void iot_ota_deinit(iot_ota_handler_t *handler);

int32_t iot_ota_set_mqtt_handler(iot_ota_handler_t *handle, iot_mqtt_ctx_t *mqtt_handle);

int32_t iot_ota_set_download_dir(iot_ota_handler_t *handle, const char *download_dir);

int32_t iot_ota_set_auto_request_ota_info_interval_sec(iot_ota_handler_t *handle, int32_t interval);

int32_t iot_ota_set_get_job_info_callback(iot_ota_handler_t *handle, iot_ota_get_job_info_callback *callback, void *user_data);

int32_t iot_ota_set_download_complete_callback(iot_ota_handler_t *handle, iot_ota_download_complete_callback *callback, void *user_data);

int32_t iot_ota_set_rev_data_progress_callback(iot_ota_handler_t *handle, iot_ota_rev_data_progress_callback *callback, void *user_data);

int32_t iot_ota_set_device_module_info(iot_ota_handler_t *handle, iot_ota_device_info_t *device_info_array, int device_info_array_size);

// start download ota task
void iot_ota_start_download(iot_ota_handler_t *handle, iot_ota_job_info_t *job_info);

int32_t iot_ota_report_installing(iot_ota_handler_t *handler, char *jobId);

int32_t iot_ota_report_install_success(iot_ota_handler_t *handler, char *jobId);

int32_t iot_ota_report_install_failed(iot_ota_handler_t *handler, char *jobId, char *result_desc);

int32_t iot_ota_request_ota_job_info(iot_ota_handler_t *handler, iot_ota_device_info_t *device_info, char *job_id);

// int32_t iot_start_auto_request_ota_info(iot_ota_handler_t *handler);
#endif //ONESDK_IOT_IOT_OTA_API_H
#endif //ONESDK_ENABLE_IOT