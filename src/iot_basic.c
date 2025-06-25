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

#include "error_code.h"
#include "iot_basic.h"
#include "iot/dynreg.h"
#include "aigw/llm.h"

iot_basic_config_t* iot_config_dup(const iot_basic_config_t* src);
void iot_config_free(iot_basic_config_t* config);

int onesdk_iot_basic_init(iot_basic_ctx_t *ctx, const iot_basic_config_t *config) {
    ctx->config = iot_config_dup(config);
    int ret = dynamic_register(ctx);
    if (ret != 0) {
        return VOLC_ERR_INVALID_PARAM;
    }

    return VOLC_OK;
}

int onesdk_iot_basic_deinit(iot_basic_ctx_t *ctx) {
    if (!ctx) {
        return VOLC_OK;
    }
    iot_config_free(ctx->config);
    ctx->config = NULL;
    free(ctx);
    ctx = NULL;
    return VOLC_OK;
}

/**
 * @brief 深层拷贝配置结构体
 * @param src 源配置结构体指针
 * @return 新分配的配置结构体指针，需要调用者手动释放
 */
iot_basic_config_t* iot_config_dup(const iot_basic_config_t* src) {
    if (!src) return NULL;

    iot_basic_config_t* dst = malloc(sizeof(iot_basic_config_t));
    if (!dst) return NULL;
    memset(dst, 0, sizeof(iot_basic_config_t));

    // 复制非指针成员
    dst->auth_type = src->auth_type;
    dst->verify_ssl = src->verify_ssl;

    // 字符串字段深拷贝（带空指针检查）
    #define COPY_STR_FIELD(field) \
        dst->field = src->field ? strdup(src->field) : NULL

    COPY_STR_FIELD(http_host);
    COPY_STR_FIELD(instance_id);
    COPY_STR_FIELD(product_key);
    COPY_STR_FIELD(product_secret);
    COPY_STR_FIELD(device_name);
    COPY_STR_FIELD(device_secret);
    COPY_STR_FIELD(ssl_ca_path);
    COPY_STR_FIELD(ssl_ca_cert);

    #undef COPY_STR_FIELD

    // 错误回滚处理
    if ((src->http_host && !dst->http_host) ||
        (src->instance_id && !dst->instance_id) ||
        (src->product_key && !dst->product_key) ||
        (src->product_secret && !dst->product_secret) ||
        (src->device_name && !dst->device_name) ||
        (src->device_secret && !dst->device_secret) ||
        (src->ssl_ca_path && !dst->ssl_ca_path) ||
        (src->ssl_ca_cert &&!dst->ssl_ca_cert) ) {

        iot_config_free(dst); // 需要配套的释放函数
        return NULL;
    }

    return dst;
}

/**
 * @brief 释放深层拷贝产生的配置结构体
 */
void iot_config_free(iot_basic_config_t* config) {
    if (config) {
        if (config->http_host != NULL) {
            free((void *)config->http_host);
            config->http_host = NULL;
        }
        if (config->instance_id != NULL) {
            free((void *)config->instance_id);
            config->instance_id = NULL;
        }
        if (config->product_key != NULL) {
            free((void *)config->product_key);
            config->product_key = NULL;
        }
        if (config->product_secret != NULL) {
            free((void *)config->product_secret);
            config->product_secret = NULL;
        }
        if (config->device_name != NULL) {
            free((void *)config->device_name);
            config->device_name = NULL;
        }
        if (config->device_secret != NULL) {
            free((void *)config->device_secret);
            config->device_secret = NULL;
        }
        if (config->ssl_ca_path != NULL) {
            free((void *)config->ssl_ca_path);
            config->ssl_ca_path = NULL;
        }
        if (config->ssl_ca_cert != NULL) {
            free((void *)config->ssl_ca_cert);
            config->ssl_ca_cert = NULL;
        }
        free(config);
        config = NULL;
    }
}

int onesdk_iot_basic_set_option(iot_basic_ctx_t *ctx, iot_basic_option_t option, void *value) {
    if (!ctx) {
        return VOLC_ERR_INVALID_PARAM;
    }
    switch (option) {
        case IOT_BASIC_HTTP_HOST:
            ctx->config->http_host = strdup(value);
            break;
        case IOT_BASIC_INSTANCE_ID:
            ctx->config->instance_id = strdup(value);
            break;
        case IOT_BASIC_VERIFY_MODE:
            ctx->config->auth_type = *(onesdk_auth_type_t *)value;
            break;
        case IOT_BASIC_PRODUCT_KEY:
            ctx->config->product_key = strdup(value);
            break;
        case IOT_BASIC_PRODUCT_SECRET:
            ctx->config->product_secret = strdup(value);
            break;
        case IOT_BASIC_DEVICE_KEY:
            ctx->config->device_name = strdup(value);
            break;
        case IOT_BASIC_DEVICE_SECRET:
            ctx->config->device_secret = strdup(value);
            break;
        case IOT_BASIC_VERIFY_SSL:
            ctx->config->verify_ssl = *(bool *)value;
            break;
        case IOT_BASIC_SSL_CA_PATH:
            ctx->config->ssl_ca_path = strdup(value);
            break;
        default:
            return VOLC_ERR_INVALID_PARAM;
    }
    return VOLC_OK;
}

const char *iot_auth_type_c_str(const onesdk_auth_type_t auth_type) {
    switch (auth_type) {
        case ONESDK_AUTH_DEVICE_SECRET:
            return "-1";
        case ONESDK_AUTH_DYNAMIC_PRE_REGISTERED:
            return "0";
        case ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED:
            return "1";
        default:
            return "UNKNOWN";
    }
}