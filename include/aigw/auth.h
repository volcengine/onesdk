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

#ifndef AIGW_AUTH_H
#define AIGW_AUTH_H

#include "protocols/http.h"
#include "iot_basic.h"
#include "iot/dynreg.h"

#define HEADER_SIGNATURE "X-Signature"
#define HEADER_AUTH_TYPE "X-Auth-Type"
#define HEADER_DEVICE_NAME "X-Device-Name"
#define HEADER_PRODUCT_KEY "X-Product-Key"
#define HEADER_RANDOM_NUM  "X-Random-Num"
#define HEADER_TIMESTAMP "X-Timestamp"

http_request_context_t *device_auth_client(http_request_context_t *http_ctx, iot_basic_ctx_t *iot_basic_ctx);

typedef struct {
    char *signature;
    char *auth_type;
    char *device_name;
    char *product_key;
    char *random_num;
    char *timestamp;
} aigw_auth_header_t;

aigw_auth_header_t *aigw_auth_header_new(iot_dynamic_register_basic_param_t *param, const char *device_secret);

void aigw_auth_header_free(aigw_auth_header_t *header);

#endif // AIGW_AUTH_H