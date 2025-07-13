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

#include "iot_log.h"
#include "onesdk.h"
#include "thing_model/iot_tm_api.h"
#include "thing_model/property.h"

#define TEST_ONESDK_IOT "test_onesdk_iot"

// online vei_dev账户
#define SAMPLE_HTTP_HOST "https://iot-cn-shanghai.iot.volces.com"
#define SAMPLE_INSTANCE_ID "***"
// #define SAMPLE_MQTT_HOST "***"
#define SAMPLE_MQTT_HOST "***"
#define SAMPLE_DEVICE_NAME "***"
#define SAMPLE_DEVICE_SECRET "***"
#define SAMPLE_PRODUCT_KEY "***"
#define SAMPLE_PRODUCT_SECRET "***"

// // relese实例
// #define SAMPLE_HTTP_HOST "10.249.160.34:9996"
// #define SAMPLE_INSTANCE_ID "***"
// // #define SAMPLE_MQTT_HOST "localhost"
// #define SAMPLE_MQTT_HOST "***"
// #define SAMPLE_DEVICE_NAME "***"
// #define SAMPLE_DEVICE_SECRET "***"
// #define SAMPLE_PRODUCT_KEY "***"
// #define SAMPLE_PRODUCT_SECRET "***"

void* keepalive_thread(void *arg) {
    onesdk_ctx_t *ctx = (onesdk_ctx_t *)arg;
    onesdk_iot_keepalive(ctx);
    return NULL;
}

void test_aiot_ota_download_complete_t(void *handler,
    int error_code, iot_ota_job_info_t *job_info,
    const char *ota_file_path,
    void *user_data) {
    DEVICE_LOGI(TEST_ONESDK_IOT, "test_aiot_ota_download_complete_t error_code = %d ota_file_path = %s", error_code, ota_file_path);
    onesdk_ctx_t *ctx = (onesdk_ctx_t *)user_data;
    if (error_code == 0) {
        // 此处执行升级过程，假定升级成功
        onesdk_iot_ota_set_device_module_info(ctx, job_info->module, job_info->dest_version);
        onesdk_iot_ota_report_install_status(ctx, job_info->ota_job_id, OTA_UPGRADE_DEVICE_STATUS_SUCCESS);
        // 升级失败
        // onesdk_iot_ota_report_install_status(ctx, job_info->ota_job_id, OTA_UPGRADE_DEVICE_STATUS_FAILED);
    }
}

void test_aiot_tm_recv_handler(void *handler, const iot_tm_recv_t *recv, void *userdata) {
    // TODO: property_set_reply + custom_topic + event_post + service
    onesdk_ctx_t *ctx = (onesdk_ctx_t *)userdata;
    if (ctx == NULL || ctx->iot_tm_handler == NULL) {
        DEVICE_LOGE(TEST_ONESDK_IOT, "test_aiot_tm_recv_handler userdata is invalid");
        return;
    }

    switch (recv->type) {
        case IOT_TM_RECV_PROPERTY_SET_POST_REPLY: {
            DEVICE_LOGI(TEST_ONESDK_IOT, "test_aiot_tm_recv_handler property_set_reply.msg_id = %s property_set_reply.code = %d",
                        recv->data.property_set_post_reply.msg_id,
                        recv->data.property_set_post_reply.code);
        }
        break;

        case IOT_TM_RECV_PROPERTY_SET: {
            DEVICE_LOGI(TEST_ONESDK_IOT, "test_aiot_tm_recv_handler property_set.msg_id = %s service_call.params_json_str = %.*s", 
                recv->data.property_set.msg_id, recv->data.property_set.params_len, recv->data.property_set.params);
        }
        break;

        case IOT_TM_RECV_CUSTOM_TOPIC: {
            DEVICE_LOGI(TEST_ONESDK_IOT, "收到自定义 Topic 消息: topic = %s, payload = %s",
                        recv->data.custom_topic.custom_topic_suffix,
                        recv->data.custom_topic.params_json_str);
        }
        break;

        case IOT_TM_RECV_EVENT_POST_REPLY: {
            DEVICE_LOGI(TEST_ONESDK_IOT, "test_aiot_tm_recv_handler property_set id = %s, code = %d module_key = %s, identifier = %s",
                        recv->data.event_post_reply.msg_id,
                        recv->data.event_post_reply.code,
                        recv->data.event_post_reply.module_key,
                        recv->data.event_post_reply.identifier);
        }
        break;

        case IOT_TM_RECV_SERVICE_CALL: {
            DEVICE_LOGI(TEST_ONESDK_IOT, "test_aiot_tm_recv_handler service_call.uuid = %s service_call.params_json_str = %s", recv->data.service_call.topic_uuid,
                        recv->data.service_call.params_json_str);
        }
        break;

        default:
            DEVICE_LOGW(TEST_ONESDK_IOT, "收到未处理的 TM 消息类型: %d", recv->type);
            break;
    }
}

static char global_sign_root_ca[] = "-----BEGIN CERTIFICATE-----\n"
"MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G\n"
"A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp\n"
"Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4\n"
"MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG\n"
"A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n"
"hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8\n"
"RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT\n"
"gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm\n"
"KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd\n"
"QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ\n"
"XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw\n"
"DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o\n"
"LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU\n"
"RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp\n"
"jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK\n"
"6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX\n"
"mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs\n"
"Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH\n"
"WD9f\n"
"-----END CERTIFICATE-----\n";

int main() {
    // 初始化配置
    iot_basic_config_t device_config = {
        .http_host = SAMPLE_HTTP_HOST,
        .instance_id = SAMPLE_INSTANCE_ID,
        .auth_type = ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED,
        .product_key = SAMPLE_PRODUCT_KEY,
        .product_secret = SAMPLE_PRODUCT_SECRET,
        .device_name = SAMPLE_DEVICE_NAME,
        .device_secret = SAMPLE_DEVICE_SECRET,
        .verify_ssl = true,
        .ssl_ca_cert = digicert_global_root_g2,
    };
    iot_mqtt_config_t mqtt_config = {
        .mqtt_host = SAMPLE_MQTT_HOST,
        .basic_config = &device_config,
        .keep_alive = 60,
        .auto_reconnect = true,
    };
    onesdk_config_t config = {
        .device_config = &device_config,
        .mqtt_config = &mqtt_config,
    };
    
    onesdk_ctx_t *ctx = malloc(sizeof(onesdk_ctx_t));
    memset(ctx, 0, sizeof(onesdk_ctx_t));

    int ret = onesdk_init(ctx, &config);
    if (ret) {
        printf("onesdk_init failed\n");
        return 1;
    }
    ret = onesdk_iot_enable_log_upload(ctx);
    if (ret) {
        printf("onesdk_iot_enable_log_upload failed\n");
        return 1;
    }
    ret = onesdk_iot_enable_ota(ctx, "./");
    if (ret) {
        printf("onesdk_iot_enable_ota failed\n");
        return 1;
    }
    onesdk_iot_ota_set_device_module_info(ctx, "default", "1.0.0");
    onesdk_iot_ota_set_download_complete_cb(ctx, test_aiot_ota_download_complete_t);
    ret = onesdk_connect(ctx);
    if (ret) {
        printf("onesdk_connect failed\n");
        return 1;
    }

#ifdef _WIN32
    // Windows 下使用 Windows 线程 API
    HANDLE keep_alive_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)keepalive_thread, ctx, 0, NULL);
    if (keep_alive_thread == NULL) {
        printf("Failed to create keep alive thread\n");
        return 1;
    }
#else
    pthread_t keep_alive_thread;
    pthread_create(&keep_alive_thread, NULL, keepalive_thread, ctx);
#endif

    ret = onesdk_iot_tm_init(ctx);
    if (ret) {
        printf("onesdk_iot_tm_init failed\n");
        return 1;
    }

    iot_tm_handler_t* tm_handler = onesdk_iot_get_tm_handler(ctx);
    if (tm_handler==NULL) {
        printf("onesdk_iot_get_tm_handler failed\n");
        return 1;
    }

    onesdk_iot_tm_set_recv_cb(ctx, test_aiot_tm_recv_handler, ctx);
    // TODO：property + event + service call + device delay
    // TODO：ntp + shadow + webshell + gateway
    onesdk_iot_tm_custom_topic_sub(ctx, "get");

    // 构造属性上报消息
    iot_tm_msg_t property_post_msg = {0};
    property_post_msg.type = IOT_TM_MSG_PROPERTY_POST;
    iot_tm_msg_property_post_t *property_post;
    iot_property_post_init(&property_post);
    // 添加属性参数
    iot_property_post_add_param_num(property_post, "default:ErrorCurrentThreshold", 0.1);
    iot_property_post_add_param_num(property_post, "default:ErrorPowerThreshold", 0);
    // GeoLocation 作为 JSON 字符串添加
    const char *geo_location_json = "{\"Altitude\":0,\"CoordinateSystem\":2,\"Latitude\":-90,\"Longitude\":-180}";
    iot_property_post_add_param_json_str(property_post, "default:GeoLocation", geo_location_json);
    iot_property_post_add_param_num(property_post, "default:LeakageEnable", 1);
    iot_property_post_add_param_num(property_post, "default:LightAdjustLevel", 0);
    iot_property_post_add_param_num(property_post, "default:LightErrorEnable", 1);
    iot_property_post_add_param_num(property_post, "default:LightStatus", 0);
    iot_property_post_add_param_num(property_post, "default:OverCurrentEnable", 0);
    iot_property_post_add_param_num(property_post, "default:OverCurrentThreshold", 0);
    iot_property_post_add_param_num(property_post, "default:OverTiltEnable", 1);
    iot_property_post_add_param_num(property_post, "default:OverVoltEnable", 1);
    iot_property_post_add_param_num(property_post, "default:OverVoltThreshold", 0);
    iot_property_post_add_param_num(property_post, "default:TiltThreshold", 0);
    iot_property_post_add_param_num(property_post, "default:UnderVoltEnable", 1);
    iot_property_post_add_param_num(property_post, "default:UnderVoltThreshold", 0);
    property_post_msg.data.property_post = property_post;
    iot_tm_send(ctx->iot_tm_handler, &property_post_msg);
    iot_property_post_free(property_post);
    
    for (int i = 0; i < 5; i++) {
        // 设备日志上报示例
        DEVICE_LOGI(TEST_ONESDK_IOT, "test log %d", i);
        onesdk_iot_tm_custom_topic_pub(ctx, "send", "{'key': 'value'}");        
        sleep(2);
    }

    onesdk_iot_tm_deinit(ctx);
    onesdk_deinit(ctx);
    return 0;
}