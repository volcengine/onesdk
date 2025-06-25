/*
 * Copyright 2022-2024 Beijing Volcano Engine Technology Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "onesdk_config.h"
#ifdef ONESDK_ENABLE_IOT
#include "thing_model/iot_tm_api.h"
#include "thing_model/iot_ntp.h"
#include "thing_model/iot_tm_header.h"
#include "error_code.h"

#include "iot/iot_utils.h"

#define GetServerTime_PATH "/2021-12-14/GetServerTime"
#define SecretTypeProduct "Product"
#define SecretTypeDevice "Device"
#define SecretType "SecretType"


int tm_ntp_get_server_time(iot_mqtt_ctx_t *mqtt_ctx, uint64_t *server_time_mil) {
    int ret = 0;
    // int32_t randomNum = arenal_get_random_num();
    // struct aws_date_time test_time;
    // aws_date_time_init_now(&test_time);
    // uint64_t timeMils = aws_date_time_as_millis(&test_time);

    // struct iot_mqtt_dynamic_register_basic_param registerParam = {
    //         .instance_id = mqtt_ctx->instance_id,
    //         .auth_type = mqtt_ctx->auth_type,
    //         .timestamp = timeMils,
    //         .random_num = randomNum,
    //         .product_key = aws_string_c_str(mqtt_ctx->product_key),
    //         .device_name = aws_string_c_str(mqtt_ctx->device_name)
    // };

    // // json 数据
    // struct aws_json_value *post_data_json = aws_json_value_new_object(mqtt_ctx->alloc);
    // aws_json_add_aws_string_val1(mqtt_ctx->alloc, post_data_json, "InstanceID", mqtt_ctx->instance_id);
    // aws_json_add_aws_string_val1(mqtt_ctx->alloc, post_data_json, "product_key", mqtt_ctx->product_key);
    // aws_json_add_aws_string_val1(mqtt_ctx->alloc, post_data_json, "device_name", mqtt_ctx->device_name);
    // aws_json_add_num_val1(mqtt_ctx->alloc, post_data_json, "random_num", registerParam.random_num);
    // aws_json_add_num_val1(mqtt_ctx->alloc, post_data_json, "timestamp", registerParam.timestamp);
    // aws_json_add_num_val1(mqtt_ctx->alloc, post_data_json, "auth_type", mqtt_ctx->auth_type);
    // aws_json_add_num_val1(mqtt_ctx->alloc, post_data_json, "DeviceSendTime", get_current_time_mil());

    // if (mqtt_ctx->device_secret != NULL && aws_string_is_valid(mqtt_ctx->device_secret)) {
    //     struct aws_string *sign = _iot_mqtt_hmac_sha256_encrypt(mqtt_ctx->alloc, &registerParam, aws_string_c_str(mqtt_ctx->device_secret));
    //     aws_json_add_aws_string_val1(mqtt_ctx->alloc, post_data_json, "signature", sign);
    //     aws_string_destroy_secure(sign);
    //     aws_json_add_str_val(post_data_json, SecretType, SecretTypeDevice);

    // } else {
    //     struct aws_string *sign = _iot_mqtt_hmac_sha256_encrypt(mqtt_ctx->alloc, &registerParam, aws_string_c_str(mqtt_ctx->product_secret));
    //     aws_json_add_aws_string_val1(mqtt_ctx->alloc, post_data_json, "signature", sign);
    //     aws_string_destroy_secure(sign);
    //     aws_json_add_str_val(post_data_json, SecretType, SecretTypeProduct);
    // }

    // char url_str[1024];
    // AWS_ZERO_ARRAY(url_str);
    // struct aws_byte_buf url_str_buf = aws_byte_buf_from_empty_array(url_str, sizeof(url_str));
    // aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str("https://"));
    // aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_string(mqtt_ctx->http_host));
    // aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str(GetServerTime_PATH));
    // aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str("?"));
    // aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str(API_ACTION_DYNAMIC_REGISTER));
    // aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str("&"));
    // aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str(API_VERSION_QUERY_PARAM));


    // LOGD(TAG_IOT_MQTT, "tm_ntp_get_server_time url_str_buf = %s", url_str);
    // struct iot_http_request_context *http_ctx = iot_new_http_ctx(mqtt_ctx->alloc);
    // iot_http_ctx_set_url(http_ctx, url_str);
    // iot_http_ctx_set_method(http_ctx, POST);
    // struct aws_byte_buf post_data_json_buf = aws_json_obj_to_byte_buf(mqtt_ctx->alloc, post_data_json);
    // char *post_data_str = aws_buf_to_char_str(mqtt_ctx->alloc, &post_data_json_buf);
    // iot_http_ctx_set_json_body(http_ctx, post_data_str);
    // struct iot_http_response *response = iot_http_request(http_ctx);

    // // {"ResponseMetadata":{"Action":"DynamicRegister","Version":"2021-12-14","Error":{"CodeN":10000030,"Message":"The parameter %s is invalid."}},"Result":{"DeviceSendTime":0,"ServerRecvTime":0,"ServerSendTime":0}}
    // // {"ResponseMetadata":{"Action":"DynamicRegister","Version":"2021-12-14"},"Result":{"DeviceSendTime":1682064791116,"ServerRecvTime":1682064791799,"ServerSendTime":1682064791799}}
    // LOGD(TAG_IOT_MQTT, "tm_ntp_get_server_time response = %s", (response->response_body));

    // if (response->error_code != 0) {
    //     ret = response->error_code;
    //     goto done;
    // }

    // // 判断成功失败
    // struct aws_byte_cursor response_json_cur = aws_byte_cursor_from_c_str(response->response_body);
    // struct aws_json_value *response_json = aws_json_value_new_from_string(mqtt_ctx->alloc, response_json_cur);
    // struct aws_json_value *error_json = aws_json_value_get_from_object(response_json, aws_byte_cursor_from_c_str("Error"));
    // int error_code = aws_json_get_num_val(error_json, "CodeN");
    // LOGD(TAG_IOT_MQTT, "tm_ntp_get_server_time error_code = %d", error_code);
    // if (error_code != 0) {
    //     ret = error_code;
    //     goto done;
    // } else {
    //     struct aws_json_value *result_json = aws_json_value_get_from_object(response_json, aws_byte_cursor_from_c_str("Result"));
    //     uint64_t device_send_time = aws_json_get_num_val(result_json, "DeviceSendTime");
    //     uint64_t server_recv_time = aws_json_get_num_val(result_json, "ServerRecvTime");
    //     uint64_t server_send_time = aws_json_get_num_val(result_json, "ServerSendTime");
    //     uint64_t device_recv_time = get_current_time_mil();
    //     *server_time_mil = (server_recv_time + server_send_time + device_recv_time - device_send_time) / 2;
    //     LOGD(TAG_IOT_MQTT, "tm_ntp_get_server_time server_time_mil = %ld", *server_time_mil);
    //     goto done;
    // }


    // done:
    // aws_byte_buf_clean_up(&url_str_buf);
    // aws_json_value_destroy(post_data_json);
    // aws_byte_buf_clean_up(&post_data_json_buf);
    // aws_mem_release(mqtt_ctx->alloc, post_data_str);
    // iot_http_response_release(response);
    return ret;

}

void _tm_recv_device_ntp_info(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
    // struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
    // struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);


    // LOGD(TAG_IOT_MQTT, "_tm_recv_device_ntp_info call topic = %.*s,  payload = %.*s", 
    //     AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
    //     AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
    // iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) pUserData;

    // // payload ={"ID":"1811682067929782","Code":0,"Data":{"DeviceSendTime":1682067929781,"ServerRecvTime":1682067930151,"ServerSendTime":1682067930151}}

    // struct aws_json_value *payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload);

    // int code = aws_json_get_num_val(payload_json, "Code");

    // if (code == 0) {
    //     struct aws_json_value *result_json = aws_json_value_get_from_object(payload_json, aws_byte_cursor_from_c_str("Data"));
    //     uint64_t device_send_time = aws_json_get_num_val(result_json, "DeviceSendTime");
    //     uint64_t server_recv_time = aws_json_get_num_val(result_json, "ServerRecvTime");
    //     uint64_t server_send_time = aws_json_get_num_val(result_json, "ServerSendTime");
    //     uint64_t device_recv_time = get_current_time_mil();
    //     uint64_t server_time_mil = (server_recv_time + server_send_time + device_recv_time - device_send_time) / 2;
    //     LOGD(TAG_IOT_MQTT, "_tm_recv_device_ntp_info server_time_mil = %ld", server_time_mil);

    //     // 数据封装
    //     iot_tm_recv_t recv = {0};
    //     recv.type = IOT_TM_RECV_NTP_SERVER_TIME;
    //     iot_tm_recv_npt_server_time_t server_time = {0};
    //     server_time.server_time_mil = server_time_mil;
    //     recv.data.npt_server_time = server_time;

    //     // 回调给业务
    //     dm_handle->recv_handler(dm_handle, &recv, dm_handle->userdata);

    // }

    // aws_json_value_destroy(payload_json);
}


int32_t tm_send_device_npt_request(void *handler) {
    // iot_tm_handler_t *dm_handle = (iot_tm_handler_t *) handler;
    int ret = VOLC_OK;

    // char *reply_topic = iot_get_common_topic(dm_handle->allocator, "sys/%s/%s/ntp/request",
    //                                           dm_handle->mqtt_handle->config->basic_config->product_key, dm_handle->mqtt_handle->config->basic_config->device_name);

    // struct aws_json_value *post_data_json = aws_json_value_new_object(dm_handle->allocator);
    // aws_json_add_num_val1(dm_handle->allocator, post_data_json, "Params", get_current_time_mil());
    // aws_json_add_str_val_1(dm_handle->allocator, post_data_json, "Version", "1.0");
    // aws_json_add_str_val_1(dm_handle->allocator, post_data_json, "ID", get_random_string_with_time_suffix(dm_handle->allocator));
    // struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(aws_alloc(), post_data_json);
    // struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);

    // // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(reply_topic);
    // // uint16_t packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
    // //                                                         AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
    // //                                                         _tm_mqtt_post_on_complete_fn,
    // //                                                         NULL);

    // PublishParams pubParams = DEFAULT_PUB_PARAMS;
    // pubParams.qos           = QOS1;
    // pubParams.payload_len   = payload_cur.len;
    // pubParams.payload       = (void *)payload_cur.ptr;
    // int packet_id = IOT_MQTT_Publish(dm_handle->mqtt_client, reply_topic, &pubParams);

    // LOGD(TAG_IOT_MQTT, "tm_send_device_npt_request call packet_id = %d, topic = %.*s, payload = %.*s", packet_id, reply_topic,
    //      AWS_BYTE_CURSOR_PRI(payload_cur));

    // // 回收内存数据
    // aws_mem_release(dm_handle->allocator, reply_topic);
    // aws_json_value_destroy(post_data_json);
    // aws_byte_buf_clean_up(&payload_buf);

    // if (packet_id == 0) {
    //     ret = STATE_DM_MQTT_PUBLISH_ERROR;
    // }
    return ret;
}

#endif // ONESDK_ENABLE_IOT