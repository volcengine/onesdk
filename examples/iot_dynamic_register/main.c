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

#include <stdio.h>

#include "dynreg.h"

#define SAMPLE_HTTP_HOST "iot-cn-shanghai.iot.volces.com"
#define SAMPLE_INSTANCE_ID "***"
#define SAMPLE_MQTT_HOST "***"
#define SAMPLE_DEVICE_NAME "***"
#define SAMPLE_DEVICE_SECRET "***"
// #define SAMPLE_DEVICE_NAME "***"
// #define SAMPLE_DEVICE_SECRET "***"
// #define SAMPLE_PRODUCT_KEY "***"
#define SAMPLE_PRODUCT_KEY "***"
#define SAMPLE_PRODUCT_SECRET "***"

// #define SAMPLE_HTTP_HOST "open-boe-stable.volcengineapi.com"
// #define SAMPLE_INSTANCE_ID "***"
// #define SAMPLE_MQTT_HOST "***"
// #define SAMPLE_DEVICE_NAME "***"
// #define SAMPLE_DEVICE_SECRET "***"
// #define SAMPLE_PRODUCT_KEY "***"
// #define SAMPLE_PRODUCT_SECRET "***"

void main() {
    iot_ctx_t ctx = {
        .host = SAMPLE_HTTP_HOST,
        .http_host = SAMPLE_HTTP_HOST,
        .port = 443,
        .instance_id = SAMPLE_INSTANCE_ID,
        .product_key = SAMPLE_PRODUCT_KEY,
        .product_secret = SAMPLE_PRODUCT_SECRET,
        .device_name = SAMPLE_DEVICE_NAME,
        .auth_type = ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED,
    };

    int ret = dynamic_register(&ctx);
    if (ret != 0) {
        printf("dynamic register failed");
    }
    printf("dynamic register success");
    printf("device secret is %s", ctx.device_secret);
    return ;
}