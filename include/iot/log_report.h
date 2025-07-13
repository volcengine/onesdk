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

#ifndef ONESDK_IOT_LOG_REPORT_H
#define ONESDK_IOT_LOG_REPORT_H
#ifdef ONESDK_ENABLE_IOT
#include "iot_log.h"
#include "iot_mqtt.h"

// 日志上报
typedef struct log_handler {
    iot_mqtt_ctx_t *mqtt_handle;
    struct aws_allocator *allocator;
    bool log_report_switch;
    platform_mutex_t lock;
    enum onesdk_log_level lowest_level; // 最新上报level
    // struct aws_hash_table* stream_id_config_map;
    bool log_report_config_topic_ready;
    bool stream_log_config_topic_ready;
    bool local_log_config_topic_ready;
} log_handler_t;

log_handler_t *aiot_log_init(void);

int aiot_log_set_mqtt_handler(log_handler_t *handle, iot_mqtt_ctx_t *mqtt_handle);

void aiot_log_set_report_switch(log_handler_t *handle, bool is_upload_log, enum onesdk_log_level lowest_level);

void aiot_log_deinit(log_handler_t *handle);

#endif // ONESDK_ENABLE_IOT
#endif //ONESDK_IOT_LOG_REPORT_H
