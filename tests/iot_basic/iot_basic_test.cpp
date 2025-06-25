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
#include  "CppUTest/TestHarness_c.h"
#include "aigw/llm.h"
#include "protocols/http.h"
#include "iot_basic.h"
#include "infer_realtime_ws.h"
}

#define SAMPLE_HTTP_HOST "iot-cn-shanghai.iot.volces.com"
#define SAMPLE_HTTP_HOST "10.249.160.34:9996"
#define SAMPLE_INSTANCE_ID "6784dcf26c8dc8689881e67d"
#define SAMPLE_MQTT_HOST "6784dcf26c8dc8689881e67d.cn-shanghai.iot.volces.com"
#define SAMPLE_DEVICE_NAME "P1-9"
#define SAMPLE_DEVICE_SECRET "98cb52e94e437ee407dbed37"
#define SAMPLE_PRODUCT_KEY "6788bd810f9bad3f8ef674fa"
#define SAMPLE_PRODUCT_SECRET "84d2917973026d49be374ec4"

TEST_GROUP(iot_basic) {
    iot_basic_config_t basic_config = {
        .http_host = SAMPLE_HTTP_HOST,
        .instance_id = SAMPLE_INSTANCE_ID,
        .auth_type = ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED,
        .product_key = SAMPLE_PRODUCT_KEY,
        .product_secret = SAMPLE_PRODUCT_SECRET,
        .device_name = SAMPLE_DEVICE_NAME,
        .device_secret = SAMPLE_DEVICE_SECRET,
    };
    void setup() {
    }

    void teardown() {
        mock("iot_basic").clear();
        mock().clear();
    }
};

http_response_t *http_request(http_request_context_t *http_context) {
    mock("iot_basic").actualCall("http_request");
    char *mock_body = "{\"ResponseMetadata\":{\"Action\":\"GetLLMConfig\",\"Version\":\"2021-12-14\"},\"Result\":{\"URL\":\"http://llm-gateway.vei.gtm.volcdns.com:30506\",\"APIKey\":\"w/LdsxG/MgSAhVGD3WmPng==\"}}\r\n";
    http_response_t *mock_response = (http_response_t *)cpputest_malloc(sizeof(http_response_t));
    memset(mock_response, 0, sizeof(http_response_t));
    mock_response->body_size = strlen(mock_body)+1;
    mock_response->response_body = cpputest_strdup(mock_body);

    http_context->response = mock_response;

    return mock_response;
}

http_request_context_t *new_http_ctx() {
    mock("iot_basic").actualCall("new_http_ctx");
    http_request_context_t *http_context = (http_request_context_t *)cpputest_malloc(sizeof(http_request_context_t));
    memset(http_context, 0, sizeof(http_request_context_t));
    return http_context;
}


TEST(iot_basic, test_iot_basic_init_and_deinit) {
    mock("iot_basic").expectOneCall("http_request").ignoreOtherParameters();
    mock("iot_basic").expectOneCall("new_http_ctx").ignoreOtherParameters();

    iot_basic_ctx_t *basic_ctx = (iot_basic_ctx_t *)cpputest_malloc(sizeof(iot_basic_ctx_t));
    memset(basic_ctx, 0, sizeof(iot_basic_ctx_t));
    int ret = onesdk_iot_basic_init(basic_ctx, &basic_config);
    CHECK(ret == 0);

    aigw_ws_config_t *ws_config = (aigw_ws_config_t *)cpputest_malloc(sizeof(aigw_ws_config_t));
    memset(ws_config, 0, sizeof(aigw_ws_config_t));
    aigw_llm_config_t *llm_config = (aigw_llm_config_t *)cpputest_malloc(sizeof(aigw_llm_config_t));
    memset(llm_config, 0, sizeof(aigw_llm_config_t));
    ret = onesdk_iot_get_binding_aigw_info(llm_config, ws_config, basic_ctx);
    CHECK(ret == 0);


    mock("iot_basic").checkExpectations();
    aigw_ws_config_deinit(ws_config);
    onesdk_iot_basic_deinit(basic_ctx);
}
