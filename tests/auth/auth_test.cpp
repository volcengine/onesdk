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

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C"
{
  #include "CppUTest/TestHarness_c.h"
  #include "aigw/auth.h"
  #include "protocols/http.h"
}

#define SAMPLE_HTTP_HOST "iot-cn-shanghai.iot.volces.com"
#define SAMPLE_INSTANCE_ID "636cf76df3c90c196d13bf7a"
#define SAMPLE_MQTT_HOST "636cf76df3c90c196d13bf7a.cn-shanghai.iot.volces.com"
#define SAMPLE_DEVICE_NAME "zhangbo-test-003"
#define SAMPLE_DEVICE_SECRET "d9cad6c4a44c2889bc690996"
#define SAMPLE_PRODUCT_KEY "677ccb2de6430bbd157fbe55"
#define SAMPLE_PRODUCT_SECRET "caf4931613298d6de43db5a5"

TEST_GROUP(auth) {
    iot_basic_ctx_t *basic_ctx;
    void setup() {
        // 动态分配内存
        basic_ctx = (iot_basic_ctx_t *)cpputest_malloc(sizeof(iot_basic_ctx_t));
        memset(basic_ctx, 0, sizeof(iot_basic_ctx_t));
        basic_ctx->config = (iot_basic_config_t *)cpputest_malloc(sizeof(iot_basic_config_t));
        memset(basic_ctx->config, 0, sizeof(iot_basic_config_t));

        // 初始化结构体成员
        basic_ctx->config->http_host = cpputest_strdup(SAMPLE_HTTP_HOST);
        basic_ctx->config->instance_id = cpputest_strdup(SAMPLE_INSTANCE_ID);
        basic_ctx->config->product_key =cpputest_strdup(SAMPLE_PRODUCT_KEY);
        basic_ctx->config->product_secret =cpputest_strdup(SAMPLE_PRODUCT_SECRET);
        basic_ctx->config->device_name =cpputest_strdup(SAMPLE_DEVICE_NAME);
        basic_ctx->config->device_secret =cpputest_strdup("98cb52e94e437ee407dbed37"); // P1
        basic_ctx->config->auth_type = ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED;
    }

    void teardown() {
        onesdk_iot_basic_deinit(basic_ctx);
    }
};

TEST(auth, test_auth) {

    http_request_context_t *client = device_auth_client(NULL, basic_ctx);
    CHECK(client != NULL);
    http_ctx_release(client);
}