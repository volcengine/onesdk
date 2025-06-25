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

#include <stdlib.h>
#include <string.h>

#include "onesdk.h"
#include "error_code.h"
#include "infer_realtime_ws.h"
#include "iot_log.h"

#define TAG_ONESDK "onesdk"

int onesdk_init(onesdk_ctx_t * ctx, const onesdk_config_t *config) {
#ifdef ONESDK_ENABLE_IOT
    iot_log_init("");
#endif
    int ret = VOLC_OK;
    if (NULL == ctx) {
        ctx = malloc(sizeof(onesdk_ctx_t));
        if (NULL == ctx) {
            return VOLC_ERR_MALLOC;
        }
    }
    memset(ctx, 0, sizeof(onesdk_ctx_t));

    // init iot basic ctx
    iot_basic_ctx_t *iot_basic_ctx = malloc(sizeof(iot_basic_ctx_t));
    if (NULL == iot_basic_ctx) {
        return VOLC_ERR_MALLOC;
    }
    memset(iot_basic_ctx, 0, sizeof(iot_basic_ctx_t));

    if (NULL == config || NULL == config->device_config) {
        onesdk_iot_basic_deinit(iot_basic_ctx);
        return VOLC_ERR_INVALID_PARAM;
    }
    ctx->config = (onesdk_config_t *)config;
    ret = onesdk_iot_basic_init(iot_basic_ctx, config->device_config);
    if (VOLC_OK != ret) {
        onesdk_iot_basic_deinit(iot_basic_ctx);
        return ret;
    }
    ctx->iot_basic_ctx = iot_basic_ctx;
#if defined(ONESDK_ENABLE_AI) || defined(ONESDK_ENABLE_AI_REALTIME)
    // fetch llm config
    ret = onesdk_fetch_config(ctx);
    if (VOLC_OK != ret) {
        onesdk_iot_basic_deinit(iot_basic_ctx);
        return ret;
    }

    // init chat ctx
    aigw_llm_config_t *llm_config = ctx->config->aigw_llm_config;
#endif
#ifdef ONESDK_ENABLE_AI

    char *endpoint = llm_config->url;
    char *api_key = llm_config->api_key;
    onesdk_chat_context_t *chat_ctx = onesdk_chat_context_init(endpoint, api_key, ctx);
    if (NULL == chat_ctx) {
        onesdk_iot_basic_deinit(iot_basic_ctx);
        return VOLC_ERR_MALLOC;
    }
    chat_ctx->iot_basic_ctx = ctx->iot_basic_ctx;
    ctx->chat_ctx = chat_ctx;

#endif
#ifdef ONESDK_ENABLE_AI_REALTIME
    // init ws config
    aigw_ws_config_t *aigw_ws_config = malloc(sizeof(aigw_ws_config_t));
    if (NULL == aigw_ws_config) {
        onesdk_iot_basic_deinit(iot_basic_ctx);
        return VOLC_ERR_MALLOC;
    }
    memset(aigw_ws_config, 0, sizeof(aigw_ws_config_t));

    // get binding gateway info from iot platform
    ret = onesdk_iot_get_binding_aigw_info(llm_config, aigw_ws_config, iot_basic_ctx);
    if (VOLC_OK != ret) {
        onesdk_iot_basic_deinit(iot_basic_ctx);
        aigw_ws_config_deinit(aigw_ws_config);
        return ret;
    }
    if (config->aigw_path != NULL) {
        free((void*)aigw_ws_config->path);
        aigw_ws_config->path = strdup(config->aigw_path);
    }
    aigw_ws_config->send_ping = config->send_ping;
    aigw_ws_config->ping_interval_s = config->ping_interval_s;
    if (config->ping_interval_s <= 0) {
        aigw_ws_config->ping_interval_s = PING_INTERVAL_S;
    }

    // init ws ctx
    aigw_ws_ctx_t *aigw_ws_ctx = malloc(sizeof(aigw_ws_ctx_t));
    if (NULL == aigw_ws_ctx) {
        onesdk_iot_basic_deinit(iot_basic_ctx);
        aigw_ws_config_deinit(aigw_ws_config);
        return VOLC_ERR_MALLOC;
    }
    memset(aigw_ws_ctx, 0, sizeof(aigw_ws_ctx_t));

    ret = aigw_ws_init(aigw_ws_ctx, aigw_ws_config);  // deep copy ws_config to ws_ctx
    aigw_ws_config_deinit(aigw_ws_config);
    if (VOLC_OK!= ret) {
        onesdk_iot_basic_deinit(iot_basic_ctx);
        free(aigw_ws_ctx);
        return ret;
    }

    ctx->aigw_ws_ctx = aigw_ws_ctx;

    aigw_ws_set_option(aigw_ws_ctx, AIGW_WS_IOT_CONFIG, ctx->iot_basic_ctx->config);

#endif

#ifdef ONESDK_ENABLE_IOT
    if (config->mqtt_config == NULL) {
        LOGI(TAG_ONESDK, "define ONESDK_ENABLE_IOT but mqtt_config is NULL, won't init mqtt");
        return VOLC_OK;  // ignore mqtt_config is NULL cas
    }
    // init iot_mqtt
    iot_mqtt_ctx_t *iot_mqtt_ctx = malloc(sizeof(iot_mqtt_ctx_t));
    if (NULL == iot_mqtt_ctx) {
        return VOLC_ERR_INIT;
    }
    memset(iot_mqtt_ctx, 0, sizeof(iot_mqtt_ctx_t));
    ret = iot_mqtt_init(iot_mqtt_ctx, config->mqtt_config);
    if (VOLC_OK!= ret) {
        onesdk_iot_basic_deinit(iot_basic_ctx);
        free(iot_mqtt_ctx);
        return ret;
    }
    ctx->iot_mqtt_ctx = iot_mqtt_ctx;
    iot_mqtt_ctx->config->basic_config = iot_basic_ctx->config;
#endif
    return ret;
}

int onesdk_connect(onesdk_ctx_t *ctx) {
    int ret = VOLC_OK;
#ifdef ONESDK_ENABLE_AI_REALTIME
    // connect iot_ws
    ret = aigw_ws_connect(ctx->aigw_ws_ctx);
    if (VOLC_OK!= ret) {
        return ret;
    }
#endif
#ifdef ONESDK_ENABLE_IOT
    // connect iot_mqtt
    if (ctx->iot_mqtt_ctx == NULL) {
        LOGI(TAG_ONESDK, "define ONESDK_ENABLE_IOT but mqtt_config is NULL, won't init mqtt");
        return VOLC_OK;  // ignore mqtt_config is NULL cas
    }
    ret = iot_mqtt_connect(ctx->iot_mqtt_ctx);
    if (VOLC_OK!= ret) {
        return ret;
    }
#endif
    return ret;
}

int onesdk_disconnect(onesdk_ctx_t *ctx) {
    int ret = VOLC_OK;

#ifdef ONESDK_ENABLE_AI
    onesdk_chat_release_context(ctx);
#endif
#ifdef ONESDK_ENABLE_AI_REALTIME
    // disconnect iot_ws
    aigw_ws_disconnect(ctx->aigw_ws_ctx);
#endif
#ifdef ONESDK_ENABLE_IOT
    // disconnect iot_mqtt
    iot_mqtt_disconnect(ctx->iot_mqtt_ctx);
#endif
    return ret;
}

int onesdk_deinit(const onesdk_ctx_t *ctx) {
#ifdef ONESDK_ENABLE_AI_REALTIME
    // deinit iot_ws
    if (NULL != ctx->aigw_ws_ctx) {
        aigw_ws_deinit(ctx->aigw_ws_ctx);
    }
    if (NULL != ctx->rt_json_buf) {
        free(ctx->rt_json_buf);
    }
#endif
#if defined(ONESDK_ENABLE_AI) || defined(ONESDK_ENABLE_AI_REALTIME)
    if (ctx->config != NULL && ctx->config->aigw_llm_config != NULL) {
        aigw_llm_config_destroy(ctx->config->aigw_llm_config);
    }
#endif
#ifdef ONESDK_ENABLE_AI
    // release chat ctx
    if (NULL != ctx->chat_ctx) {
        onesdk_chat_release_context((onesdk_ctx_t *)ctx);
    }
#endif
#ifdef ONESDK_ENABLE_IOT
    if (ctx->iot_log_upload_switch) {
        // release log upload
        aiot_log_deinit(ctx->iot_log_handler);
    }
    if (ctx->iot_ota_switch) {
        // release ota
        iot_ota_deinit(ctx->iot_ota_handler);
    }
    if (NULL != ctx->iot_mqtt_ctx) {
        iot_mqtt_deinit(ctx->iot_mqtt_ctx);
    }
#endif

    if (NULL != ctx->iot_basic_ctx) {
        onesdk_iot_basic_deinit(ctx->iot_basic_ctx);
    }
#ifdef ONESDK_ENABLE_IOT
    iot_log_deinit();
#endif
    return VOLC_OK;
}

#if defined(ONESDK_ENABLE_AI_REALTIME) || defined(ONESDK_ENABLE_AI)
int onesdk_fetch_config(onesdk_ctx_t *ctx) {
    // 调用iot平台接口获取绑定的网关信息
    if (ctx->config == NULL) {
        printf("onesdk_fetch_config: config is NULL\n");
        return VOLC_ERR_INVALID_PARAM;
    }
    onesdk_config_t *config = ctx->config;
    aigw_llm_config_t *aigw_cfg = malloc(sizeof(aigw_llm_config_t));
    memset(aigw_cfg, 0, sizeof(aigw_llm_config_t));

    int ret = get_llm_config(ctx->iot_basic_ctx, aigw_cfg);
    if (ret != 0) {
        return VOLC_ERR_INVALID_PARAM;
    }
    config->aigw_llm_config = aigw_cfg;
    return VOLC_OK;
}
#endif