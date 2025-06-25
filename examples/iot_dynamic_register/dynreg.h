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

#ifndef IOT_DEVREG_H
#define IOT_DEVREG_H

#include <stdint.h>
#include "protocols/http.h"

typedef struct {
    int32_t CodeN;
    char *Code;
    char *Message;
} iot_http_response_meta_data_error_t;


typedef struct {
    char *action;
    char *version;
    iot_http_response_meta_data_error_t responseMetaDataError;
} iot_http_response_meta_data_t;


typedef struct {
    int32_t len;
    char *payload;
} iot_http_response_dynamic_register_result_t;


typedef struct {
    iot_http_response_dynamic_register_result_t result;
    iot_http_response_meta_data_t meta_info;
} iot_http_response_dynamic_register_t;

typedef enum {
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
    ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED = 1
} onesdk_auth_type_t;

struct iot_dynamic_register_basic_param {
    const char *instance_id;
    const char *product_key;
    const char *device_name;
    int32_t random_num;
    uint64_t timestamp;
    onesdk_auth_type_t auth_type;
};

typedef struct iot_ctx {
    const char *host;
    const char  *http_host;
    int32_t port;
    const char *instance_id;
    const char *product_key;
    const char *product_secret;
    const char *device_name;
    const char *device_secret;
    onesdk_auth_type_t auth_type;
} iot_ctx_t;

int dynamic_register(iot_ctx_t *ctx);

#endif // IOT_DEVREG_H