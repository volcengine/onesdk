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

// #include "CppUTest/TestHarness.h"
// #include "CppUTestExt/MockSupport.h"
//
// extern "C"
// {
//     #include "onesdk.h"
// }
//
// // #define SAMPLE_HTTP_HOST "iot-cn-shanghai.iot.volces.com"
// #define SAMPLE_HTTP_HOST "10.249.160.34:9996"
// #define SAMPLE_INSTANCE_ID "6784dcf26c8dc8689881e67d"
// #define SAMPLE_MQTT_HOST "6784dcf26c8dc8689881e67d.cn-shanghai.iot.volces.com"
// #define SAMPLE_DEVICE_NAME "P1-9"
// #define SAMPLE_DEVICE_SECRET ""
// #define SAMPLE_PRODUCT_KEY "6788bd810f9bad3f8ef674fa"
// #define SAMPLE_PRODUCT_SECRET "84d2917973026d49be374ec4"
//
// static void mock_event_cb( const void *event_data, size_t data_len, void *userdata) {
//
// }
//
// TEST_GROUP(onesdk_rt) {
//     onesdk_ctx_t *ctx;
//     onesdk_rt_event_cb_t *event_cb;
//     void setup() {
//         ctx = static_cast<onesdk_ctx_t *>(malloc(sizeof(onesdk_ctx_t)));
//         auto *onesdk_config = static_cast<onesdk_config_t *>(malloc(sizeof(onesdk_config_t)));
//         auto *basic_config = static_cast<iot_basic_config_t *>(malloc(sizeof(iot_basic_config_t)));
//         onesdk_config->device_config = basic_config;
//         basic_config->http_host = SAMPLE_HTTP_HOST;
//         basic_config->instance_id = SAMPLE_INSTANCE_ID;
//         basic_config->product_key = SAMPLE_PRODUCT_KEY;
//         basic_config->product_secret = SAMPLE_PRODUCT_SECRET;
//         basic_config->device_key = SAMPLE_DEVICE_NAME;
//         basic_config->device_secret = SAMPLE_DEVICE_SECRET;
//         basic_config->verify_mode = verify_mode_t(3);
//         basic_config->verify_ssl = true;
//         basic_config->ssl_ca_path = "./tests/onesdk_rt/ca.crt";
//         onesdk_init(ctx, onesdk_config);
//         event_cb = static_cast<onesdk_rt_event_cb_t *>(malloc(sizeof(onesdk_rt_event_cb_t)));
//         event_cb->on_audio = mock_event_cb;
//         onesdk_rt_set_event_cb(ctx, event_cb);
//     }
//
//     void teardown() {
//         onesdk_deinit(ctx);
//         free(event_cb);
//         mock().clear();
//     }
// };
//
// TEST(onesdk_rt, test_onesdk_rt_set_event_cb) {
//     // 调用 onesdk_rt_set_event_cb 函数设置回调
//     int ret = onesdk_rt_set_event_cb(ctx, event_cb);
//     CHECK_EQUAL(0, ret); // 假设返回 0 表示设置成功
//
//     // 模拟一个事件，调用事件处理函数，触发回调
//     const char *test_event_data = "Test event data";
//     size_t event_data_len = strlen(test_event_data);
//
//     // 假设存在一个函数可以触发事件
//     mock().expectOneCall("mock_event_cb")
//           .withParameter("ctx", ctx)
//           .withParameter("event_data", const_cast<char*>(test_event_data))
//           .withParameter("data_len", event_data_len);
//
//     // 这里需要调用一个能触发事件的函数，例如：
//     // onesdk_rt_trigger_event(ctx, const_cast<char*>(test_event_data), event_data_len);
//     // 请根据实际情况替换为正确的触发事件的函数
//
//     // 检查模拟的回调函数是否被调用
//     mock().checkExpectations();
// }