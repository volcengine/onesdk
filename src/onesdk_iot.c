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

#include "error_code.h"
#include "iot_log.h"
#include "iot/iot_utils.h"
#include <stdio.h>
#include "onesdk.h"
#include "iot_mqtt.h"
#include "thing_model/iot_tm_api.h" // 假设物模型核心头文件
#include "thing_model/custom_topic.h" // 包含自定义 Topic 头文件

int onesdk_iot_keepalive(onesdk_ctx_t *ctx) {
    int ret = VOLC_OK;
    if (NULL == ctx || NULL == ctx->iot_mqtt_ctx) {
        return VOLC_ERR_INIT;
    }
    int n = 0;
    while (n >= 0) {
        n = iot_mqtt_run_event_loop(ctx->iot_mqtt_ctx, 1000);
    }
    return ret;
}

int onesdk_iot_enable_log_upload(onesdk_ctx_t *ctx) {
    int ret = VOLC_OK;
    if (NULL == ctx || NULL == ctx->iot_mqtt_ctx) {
        return VOLC_ERR_INIT;
    }
    log_handler_t *log_handler = aiot_log_init();
    ret = aiot_log_set_mqtt_handler(log_handler, ctx->iot_mqtt_ctx);
    if (VOLC_OK != ret) {
        aiot_log_deinit(log_handler);
        return ret;
    }
    ctx->iot_log_upload_switch = true;
    ctx->iot_log_handler = log_handler;
    return ret;
}

static void _ota_get_job_info_cb(void *handler, iot_ota_job_info_t *job_info, void *user_data) {
    iot_ota_start_download(handler, job_info);
}

int onesdk_iot_enable_ota(onesdk_ctx_t *ctx, const char *download_dir) {
    int ret = VOLC_OK;
    if (NULL == ctx || NULL == ctx->iot_mqtt_ctx) {
        return VOLC_ERR_INIT;
    }
    iot_ota_handler_t *ota_handler = iot_ota_init();
    ret = iot_ota_set_mqtt_handler(ota_handler, ctx->iot_mqtt_ctx);
    if (VOLC_OK!= ret) {
        iot_ota_deinit(ota_handler);
        return ret;
    }
    ctx->iot_ota_switch = true;
    ctx->iot_ota_handler = ota_handler;
    iot_ota_set_get_job_info_callback(ota_handler, _ota_get_job_info_cb, ota_handler);
    iot_ota_set_download_dir(ota_handler, download_dir);
    return ret;
}

int onesdk_iot_ota_set_device_module_info(onesdk_ctx_t *ctx, const char *module_name, const char *module_version) {
    iot_ota_device_info_t device_info_array[1];
    device_info_array[0].module = (char*)module_name;
    device_info_array[0].version = (char*)module_version;
    int ret = iot_ota_set_device_module_info(ctx->iot_ota_handler, device_info_array, 1);
    return ret;
}

void onesdk_iot_ota_set_download_complete_cb(onesdk_ctx_t *ctx, iot_ota_download_complete_callback *cb) {
    iot_ota_set_download_complete_callback(ctx->iot_ota_handler, cb, ctx);
}

int onesdk_iot_ota_report_install_status(onesdk_ctx_t *ctx, char *jobID, ota_upgrade_device_status_enum_t status) {
    switch (status) {
        case OTA_UPGRADE_DEVICE_STATUS_SUCCESS:
            return iot_ota_report_install_success(ctx->iot_ota_handler, jobID);
        case OTA_UPGRADE_DEVICE_STATUS_FAILED:
            return iot_ota_report_install_failed(ctx->iot_ota_handler, jobID, "");
        case OTA_UPGRADE_DEVICE_STATUS_DOWNLOADING:
            return iot_ota_report_installing(ctx->iot_ota_handler, jobID);
        default:
            return VOLC_ERR_INVALID_PARAM;
    }
}

// --- 开始: 物模型 (Thing Model) 函数实现 ---

/**
 * @brief 初始化物模型模块
 *
 * @param ctx onesdk 上下文指针
 * @return int 成功返回 VOLC_OK，失败返回错误码
 */
int onesdk_iot_tm_init(onesdk_ctx_t *ctx) {
    int ret = VOLC_OK;
    if (NULL == ctx || NULL == ctx->iot_mqtt_ctx) {
        return VOLC_ERR_INIT;
    }
    // 检查是否已初始化
    if (ctx->iot_tm_handler != NULL) {
        return VOLC_OK; // 已经初始化
    }

    iot_tm_handler_t *tm_handler = iot_tm_init();
    if (NULL == tm_handler) {
        return VOLC_ERR_INIT;
    }

    ret = iot_tm_set_mqtt_handler(tm_handler, ctx->iot_mqtt_ctx);
    if (VOLC_OK != ret) {
        iot_tm_deinit(tm_handler);
        return ret;
    }

    ctx->iot_tm_handler = tm_handler;
    ctx->iot_tm_switch = true; // 标记物模型功能已启用

    return VOLC_OK;
}

/**
 * @brief 释放物模型模块资源
 *
 * @param ctx onesdk 上下文指针
 */
void onesdk_iot_tm_deinit(onesdk_ctx_t *ctx) {
    if (NULL == ctx || NULL == ctx->iot_tm_handler) {
        return; // 若上下文或物模型句柄为空，直接返回
    }

    iot_tm_deinit(ctx->iot_tm_handler);
    ctx->iot_tm_handler = NULL;
    ctx->iot_tm_switch = false;
}

/**
 * @brief 设置物模型消息接收回调函数
 *
 * @param ctx onesdk 上下文指针
 * @param cb 回调函数指针
 * @param user_data 用户自定义数据，将会在回调时透传
 * @return int 成功返回 VOLC_OK，失败返回错误码
 */
int onesdk_iot_tm_set_recv_cb(onesdk_ctx_t *ctx, iot_tm_recv_handler_t *cb, void *user_data) {
    if (NULL == ctx || NULL == ctx->iot_tm_handler) {
        LOGE(TAG_IOT_MQTT, "Thing Model not initialized or context is NULL");
        return VOLC_ERR_INIT; // 或者 VOLC_ERR_INVALID_PARAM
    }
    if (NULL == cb) {
        return VOLC_ERR_INVALID_PARAM;
    }

    // 设置接收回调
    iot_tm_set_tm_recv_handler_t(ctx->iot_tm_handler, cb, user_data);
    return VOLC_OK;
}

/**
 * @brief 获取底层的物模型处理器 iot_tm_handler_t
 *
 * @param ctx OneSDK 上下文
 * @return iot_tm_handler_t* 物模型处理器指针，如果物模型未初始化则返回 NULL
 * @note 用户获取到 handler 后，可以直接调用 iot_tm_api.h 中定义的函数，
 *       例如 iot_tm_set_tm_recv_handler_t, iot_tm_send, iot_tm_property_post_init 等。
 */
iot_tm_handler_t* onesdk_iot_get_tm_handler(onesdk_ctx_t *ctx) {
    if (NULL == ctx || NULL == ctx->iot_tm_handler) {
        LOGE(TAG_IOT_MQTT, "Thing Model not initialized or context is NULL");
        return NULL;
    }
    return ctx->iot_tm_handler;
}

/**
 * @brief 订阅物模型的自定义 Topic
 *
 * @param ctx onesdk 上下文指针
 * @param topic_suffix 自定义 Topic 的后缀 (例如 "user/get")
 * @return int 成功返回 VOLC_OK，失败返回错误码
 */
int onesdk_iot_tm_custom_topic_sub(onesdk_ctx_t *ctx, const char *topic_suffix) {
     if (NULL == ctx || NULL == ctx->iot_tm_handler) {
        LOGE(TAG_IOT_MQTT, "Thing Model not initialized or context is NULL");
        return VOLC_ERR_INIT;
    }
    if (NULL == topic_suffix || strlen(topic_suffix) == 0) {
        return VOLC_ERR_INVALID_PARAM;
    }

    // 调用底层 SDK 的自定义 Topic 订阅函数
    int ret = tm_sub_custom_topic(ctx->iot_tm_handler, topic_suffix);
    if (VOLC_OK != ret) {
       LOGE(TAG_IOT_MQTT, "Failed to subscribe to custom topic '%s', ret = %d", topic_suffix, ret);
    }
    return ret;
}

/**
 * @brief 发布自定义消息
 *
 * @param ctx onesdk 上下文指针
 * @param msg 要发布的消息结构体指针
 * @return int 成功返回 VOLC_OK，失败返回错误码
 */
int onesdk_iot_tm_custom_topic_pub(onesdk_ctx_t *ctx, const char* topic_suffix, const char *payload) {
    if (NULL == ctx || NULL == ctx->iot_tm_handler) {
        return VOLC_ERR_INIT;
    }
    if (NULL == topic_suffix || strlen(topic_suffix) == 0 || NULL == payload || strlen(payload) == 0) {
        return VOLC_ERR_INVALID_PARAM;
    }

    iot_tm_msg_custom_topic_post_t *custom_topic_post;
    iot_tm_msg_aiot_tm_msg_custom_topic_post_init(&custom_topic_post, (char*)topic_suffix, (char*)payload);
    iot_tm_msg_t dm_msg = {
        .type = IOT_TM_MSG_CUSTOM_TOPIC,
        .data = {
            .custom_topic_post = custom_topic_post,
        },
    };
    int ret = iot_tm_send(ctx->iot_tm_handler, &dm_msg);
    iot_tm_msg_aiot_tm_msg_custom_topic_post_free(custom_topic_post);
     if (VOLC_OK != ret) {
       LOGE(TAG_IOT_MQTT, "Failed to publish Thing Model message, type = %d, ret = %d", dm_msg.type, ret);
    }
    return ret;
}

#endif // ONESDK_ENABLE_IOT