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



#ifndef ONESDK_ONESDK_H
#define ONESDK_ONESDK_H

#include "onesdk_config.h"
#include <stddef.h>
#include "iot_basic.h"
#include "error_code.h"


#ifdef ONESDK_ENABLE_AI
#include "onesdk_chat.h"
#include "infer_inner_chat.h"
#endif

#include "aigw/llm.h"
#include "volc_ca.h"

#ifdef ONESDK_ENABLE_AI_REALTIME
#include "infer_inner_chat.h"
#include "infer_realtime_ws.h"
#define PING_INTERVAL_S 110
#endif

#ifdef ONESDK_ENABLE_IOT
#include "iot_mqtt.h"
#include "iot/iot_ota_api.h"
#include "iot/log_report.h"
#include "thing_model/iot_tm_api.h"
#include "thing_model/property.h"
#endif

/**
 * @brief 协议连接配置（可扩展）
 */
typedef struct onesdk_config {
    // device config
    iot_basic_config_t *device_config;

#ifdef ONESDK_ENABLE_IOT
    // mqtt config
    iot_mqtt_config_t* mqtt_config;
#endif

#if defined(ONESDK_ENABLE_AI) || defined(ONESDK_ENABLE_AI_REALTIME)
    // llm config from iot
    aigw_llm_config_t *aigw_llm_config;
#endif

#ifdef ONESDK_ENABLE_AI_REALTIME
    // aigw ws config
    const char* aigw_path;
    bool send_ping;
    int ping_interval_s;
#endif

} onesdk_config_t;

#ifdef ONESDK_ENABLE_AI_REALTIME
typedef struct {
    void (*on_audio)(const void *audio_data, size_t audio_len, void *user_data);
    void (*on_text)(const void *text_data, size_t text_len, void *user_data);
    void (*on_error)(const char* error_code, const char *error_msg, void *user_data);
    void (*on_transcript_text)(const void *transcript_data, size_t transcript_len, void *user_data);        // 转录文本流式回调（源语种）
    void (*on_translation_text)(const void *translation_data, size_t translation_len, void *user_data);     // 翻译文本流式回调（目标语种）
    void (*on_response_done)(void *user_data);
} onesdk_rt_event_cb_t;

#endif
/**
 * @brief 上下文对象（线程安全设计）
 */
typedef struct {
    onesdk_config_t *config;
    iot_basic_ctx_t *iot_basic_ctx;
    void* user_data;
#ifdef ONESDK_ENABLE_AI
    onesdk_chat_context_t *chat_ctx;
#endif
#ifdef ONESDK_ENABLE_AI_REALTIME
    aigw_ws_ctx_t *aigw_ws_ctx;
    onesdk_rt_event_cb_t *rt_event_cb;
    // 新增JSON解析相关字段
    char *rt_json_buf;
    size_t rt_json_buf_size;
    size_t rt_json_buf_capacity;
    char *rt_json_pos;
#endif
#ifdef ONESDK_ENABLE_IOT
    iot_mqtt_ctx_t *iot_mqtt_ctx;

    bool iot_log_upload_switch;
    log_handler_t *iot_log_handler;

    bool iot_ota_switch;
    iot_ota_handler_t *iot_ota_handler;

    bool iot_tm_switch;
    iot_tm_handler_t *iot_tm_handler;
#endif
} onesdk_ctx_t;

int onesdk_fetch_config(onesdk_ctx_t *ctx);

// common interfaces
int onesdk_init(onesdk_ctx_t *, onesdk_config_t *config);

int onesdk_connect(onesdk_ctx_t *ctx);

int onesdk_disconnect(onesdk_ctx_t *ctx);

int onesdk_deinit(const onesdk_ctx_t *ctx);

#ifdef ONESDK_ENABLE_AI

/**
 * @brief 发送请求（流式）
 *
 * @param ctx
 * @param request
 * @param output
 * @param output_len
 * @return int 0 成功，在同步请求时, 请配置output和output_len, 用于接收response
 */
int onesdk_chat(onesdk_ctx_t *ctx, onesdk_chat_request_t *request, char *output, size_t *output_len);

/**
 * @brief 发送流式请求
 * 
 * @param ctx
 * @param request
 * @param cbs
 * @return int 0 成功，在同步请求时, 请配置output和output_len, 用于接收response
 */
int onesdk_chat_stream(onesdk_ctx_t *ctx, onesdk_chat_request_t *request, onesdk_chat_callbacks_t *cbs);

/**
 * @brief 设置回调
 * 
 * @param ctx
 * @param cbs
 * @param user_data
 * 
 * @return int 0 成功，其他失败
 * 
 */
void onesdk_chat_set_callbacks(onesdk_ctx_t *ctx, onesdk_chat_callbacks_t *cbs, void *user_data);

/**
 * @brief 设置函数调用数据
 *
 * @param ctx
 * @param response_out
 * @return int 0 成功，其他失败
 */
void onesdk_chat_set_functioncall_data(onesdk_ctx_t *ctx, onesdk_chat_response_t **response_out);
    /**
 * @brief 等待异步请求完成
 * 
 * @param ctx
 * @return int 0 成功，其他失败
 */
int onesdk_chat_wait(onesdk_ctx_t *ctx);

/**
 * @brief 释放上下文
 *
 * @param ctx
 * @return int 0 成功，其他失败
 */
void onesdk_chat_release_context(onesdk_ctx_t *ctx);

#endif


#ifdef ONESDK_ENABLE_AI_REALTIME
int onesdk_rt_set_event_cb(onesdk_ctx_t *ctx, onesdk_rt_event_cb_t *cb);
int onesdk_rt_session_keepalive(onesdk_ctx_t *ctx);

// chat_agent interfaces
int onesdk_rt_session_update(onesdk_ctx_t *ctx, aigw_ws_session_t *session);
int onesdk_rt_audio_send(onesdk_ctx_t *ctx, const char *audio_data, size_t len, bool commit);
int onesdk_rt_audio_response_cancel(onesdk_ctx_t *ctx);

// translation_agent interfaces
int onesdk_rt_translation_session_update(onesdk_ctx_t *ctx, aigw_ws_translation_session_t *session);
int onesdk_rt_translation_audio_send(onesdk_ctx_t *ctx, const char *audio_data, size_t len, bool commit);
#endif

#ifdef ONESDK_ENABLE_IOT

int onesdk_iot_keepalive(onesdk_ctx_t *ctx);

// --- log related interfaces--- 
int onesdk_iot_enable_log_upload(onesdk_ctx_t *ctx);

// --- ota related interfaces ---
int onesdk_iot_enable_ota(onesdk_ctx_t *ctx, const char *download_dir);
// TODO: 确认传数组还是单个传。
int onesdk_iot_ota_set_device_module_info(onesdk_ctx_t *ctx, const char *module_name, const char *module_version);
void onesdk_iot_ota_set_download_complete_cb(onesdk_ctx_t *ctx, iot_ota_download_complete_callback *cb);

typedef enum ota_upgrade_device_status_enum {
    OTA_UPGRADE_DEVICE_STATUS_SUCCESS = 0,
    OTA_UPGRADE_DEVICE_STATUS_FAILED = 1,
    OTA_UPGRADE_DEVICE_STATUS_DOWNLOADING = 2,
} ota_upgrade_device_status_enum_t;

int onesdk_iot_ota_report_install_status(onesdk_ctx_t *ctx, char *jobID, ota_upgrade_device_status_enum_t status);

// --- Thing Model related interfaces ---

int onesdk_iot_tm_init(onesdk_ctx_t *ctx);
void onesdk_iot_tm_deinit(onesdk_ctx_t *ctx);
int onesdk_iot_tm_set_recv_cb(onesdk_ctx_t *ctx, iot_tm_recv_handler_t *cb, void *user_data);
iot_tm_handler_t* onesdk_iot_get_tm_handler(onesdk_ctx_t *ctx);

int onesdk_iot_tm_custom_topic_sub(onesdk_ctx_t *ctx, const char *topic_suffix);
int onesdk_iot_tm_custom_topic_pub(onesdk_ctx_t *ctx, const char* topic_suffix, const char *payload);
#endif // ONESDK_ENABLE_IOT

#endif //ONESDK_ONESDK_H