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


#ifndef ONESDK_IOT_BASIC_H
#define ONESDK_IOT_BASIC_H

#include <stdbool.h>

typedef enum onesdk_auth_type {
    /**
     * 一机一密，需要提供ProductKey、DeviceName、DeviceSecret
     */
    ONESDK_AUTH_DEVICE_SECRET = -1,
    /**
     * 一型一密预注册，需要提供ProductKey、ProductSecret、DeviceName
     */
    ONESDK_AUTH_DYNAMIC_PRE_REGISTERED = 0,
    /**
     * 一型一密免预注册，需要提供ProductKey、ProductSecret、Name
     */
    ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED = 1,

    ONESDK_AUTH_UNKNOWN = 100
} onesdk_auth_type_t;

/**
 * @brief Device Basic meta info
 */
typedef struct iot_basic_config {
    // connection config
    char *http_host;
    char *instance_id;

    // auth config
    onesdk_auth_type_t auth_type;
    char *product_key;
    char *product_secret;
    char *device_name;
    char *device_secret;

    // certificate config
    bool verify_ssl;              // 是否验证SSL证书
    char *ssl_ca_cert ;           // CA证书，base64编码
    char *ssl_ca_path;      // CA证书路径
} iot_basic_config_t;

typedef struct iot_basic_ctx {
    iot_basic_config_t *config;
    
} iot_basic_ctx_t;

int onesdk_iot_basic_init(iot_basic_ctx_t *, const iot_basic_config_t *config);

iot_basic_config_t* iot_config_dup(const iot_basic_config_t* src);

typedef enum iot_basic_option {
    IOT_BASIC_HTTP_HOST,
    IOT_BASIC_INSTANCE_ID,
    IOT_BASIC_VERIFY_MODE,
    IOT_BASIC_PRODUCT_KEY,
    IOT_BASIC_PRODUCT_SECRET,
    IOT_BASIC_DEVICE_KEY,
    IOT_BASIC_DEVICE_SECRET,
    IOT_BASIC_VERIFY_SSL,
    IOT_BASIC_SSL_CA_PATH,
} iot_basic_option_t;

/**
 * @brief 设置大模型网关连接配置信息
 * @param ctx iot上下文
 * @param option 配置项
 * @param value 配置值
 * @return 0 成功，其他失败
 */
int onesdk_iot_basic_set_option(iot_basic_ctx_t *ctx, iot_basic_option_t option, void *value);

const char *iot_auth_type_c_str(const onesdk_auth_type_t auth_type);

int onesdk_iot_basic_deinit(iot_basic_ctx_t *);

#endif //ONESDK_IOT_BASIC_H
