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

#include "iot_mqtt.h"
#include "iot_log.h"

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

// relese实例
#define SAMPLE_HTTP_HOST "10.249.160.34:9996"
#define SAMPLE_INSTANCE_ID "***"
// #define SAMPLE_MQTT_HOST "***"
#define SAMPLE_MQTT_HOST "***"
#define SAMPLE_DEVICE_NAME "***"
#define SAMPLE_DEVICE_SECRET "***"
#define SAMPLE_PRODUCT_KEY "***"
#define SAMPLE_PRODUCT_SECRET "***"
#define SAMPLE_SUB_TOPIC "sys/%s/%s/thingmodel/service/preset/propertySet/post"
#define SAMPLE_PUB_TOPIC "sys/%s/%s/custom/custom"

static void _iot_mqtt_event_callback(iot_mqtt_event_type_t event_type, void *user_data) {
    printf("event_type: %d\n", event_type);
}

static void _iot_mqtt_message_callback(const char* topic, const uint8_t *payload, size_t len, void *user_data) {
    printf("message_callback: topic = %s, payload = %s\n", topic, payload);
}

#ifdef _WIN32
DWORD WINAPI keepalive_thread(LPVOID arg) {
    iot_mqtt_ctx_t *ctx = (iot_mqtt_ctx_t *)arg;
    int n;
    while (n >= 0)
        n = iot_mqtt_run_event_loop(ctx, 1000);
    return 0;
}
#else
void* keepalive_thread(void *arg) {
    iot_mqtt_ctx_t *ctx = (iot_mqtt_ctx_t *)arg;
    int n;
    while (n >= 0)
        n = iot_mqtt_run_event_loop(ctx, 1000);
    return NULL;
}
#endif

int main() {
    iot_log_init("");
    LOGI("main", "start");
    lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO, NULL);
    iot_mqtt_ctx_t *iot_mqtt_ctx = malloc(sizeof(iot_mqtt_ctx_t));
    memset(iot_mqtt_ctx, 0, sizeof(iot_mqtt_ctx_t));
    iot_mqtt_config_t *iot_mqtt_config = malloc(sizeof(iot_mqtt_config_t));
    memset(iot_mqtt_config, 0, sizeof(iot_mqtt_config_t));
    iot_mqtt_config->mqtt_host = SAMPLE_MQTT_HOST;
    iot_mqtt_config->basic_config = &(iot_basic_config_t){
        .http_host = SAMPLE_HTTP_HOST,
        .instance_id = SAMPLE_INSTANCE_ID,
        .auth_type = ONESDK_AUTH_DYNAMIC_PRE_REGISTERED,
        .product_key = SAMPLE_PRODUCT_KEY,  
        .product_secret = SAMPLE_PRODUCT_SECRET,
        .device_name = SAMPLE_DEVICE_NAME,
        .device_secret = SAMPLE_DEVICE_SECRET,
        .verify_ssl = false,
        .ssl_ca_path = NULL,
    };
    iot_mqtt_config->keep_alive = 60;
    int ret = iot_mqtt_init(iot_mqtt_ctx, iot_mqtt_config);
    if (ret) {
        printf("iot_mqtt_init failed\n");
        return 1;
    }
    printf("username: %s\n", aws_string_c_str(iot_mqtt_config->username));
    printf("password: %s\n", aws_string_c_str(iot_mqtt_config->password));

    // create a keep alive thread
#ifdef _WIN32
    HANDLE keep_alive_thread = CreateThread(NULL, 0, keepalive_thread, iot_mqtt_ctx, 0, NULL);
    if (keep_alive_thread == NULL) {
        printf("Failed to create keep alive thread\n");
        return 1;
    }
#else
    pthread_t keep_alive_thread;
    pthread_create(&keep_alive_thread, NULL, keepalive_thread, iot_mqtt_ctx);
#endif
    if (iot_mqtt_connect(iot_mqtt_ctx)) {
        printf("iot_mqtt_connect failed\n");
        goto finish;
    }    

    char topic[128];
    memset(topic, 0, sizeof(topic));
    snprintf(topic, sizeof(topic), SAMPLE_SUB_TOPIC, SAMPLE_PRODUCT_KEY, SAMPLE_DEVICE_NAME);
    
    // TODO: 确定是否需要分配释放内存
    iot_mqtt_subscribe(iot_mqtt_ctx, &(iot_mqtt_topic_map_t){
        .topic = topic,
        .message_callback = _iot_mqtt_message_callback,
        .event_callback = _iot_mqtt_event_callback,
        .user_data = (void*)iot_mqtt_ctx,
        .qos = IOT_MQTT_QOS1,
    });
    
    char pub_topic[128];
    memset(pub_topic, 0, sizeof(pub_topic));
    snprintf(pub_topic, sizeof(pub_topic), SAMPLE_PUB_TOPIC, SAMPLE_PRODUCT_KEY, SAMPLE_DEVICE_NAME);

    for (int i = 0; i< 10; i++) {
        char payload[128];
        memset(payload, 0, sizeof(payload));
        snprintf(payload, sizeof(payload), "{\"test\": \"%d\"}", i);
        iot_mqtt_publish(iot_mqtt_ctx, pub_topic, payload, strlen(payload), QOS1);
        sleep(1);
    }

    if (iot_mqtt_disconnect(iot_mqtt_ctx)) {
        printf("iot_mqtt_disconnect failed\n");
        goto finish;
    }

    sleep(10);
    iot_mqtt_reconnect(iot_mqtt_ctx);

    sleep(100);
finish:
    // 清理线程
#ifdef _WIN32
    TerminateThread(keep_alive_thread, 0);
    WaitForSingleObject(keep_alive_thread, INFINITE);
    CloseHandle(keep_alive_thread);
#else
    pthread_cancel(keep_alive_thread);
    pthread_join(keep_alive_thread, NULL);
#endif
    iot_mqtt_deinit(iot_mqtt_ctx);
    free(iot_mqtt_ctx);
    free(iot_mqtt_config);
    return 1;
}