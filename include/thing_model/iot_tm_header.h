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



// tm header
#ifndef ARENAL_IOT_IOT_TM_HEADER_H
#define ARENAL_IOT_IOT_TM_HEADER_H


#include "thing_model/custom_topic.h"
#include "thing_model/gateway.h"
#include "thing_model/property.h"
#include "thing_model/service.h"
#include "thing_model/shadow.h"
#include "thing_model/webshell.h"
#include "thing_model/iot_tm_api.h"
#include "iot_mqtt.h"

// custom_topic.c

char* iot_tm_msg_aiot_tm_msg_custom_topic_post_payload(iot_tm_msg_custom_topic_post_t *custom_topic_post);

int32_t _tm_send_custom_topic_post_data(void *handler, const char *topic, const void *msg_p);

/**
 * 接收 服务端下发的 自定义 topic
 */
void _tm_recv_custom_topic(const char* topic, const uint8_t *payload, size_t len, void *pUserData);

// device_delay.c


// event.c
/**
 * 发送 event
 */
int32_t _tm_send_event_post(void *handler, const char *topic, const void *msg_p);

/**
 * 接收 事件上报之后 服务端给的回复
 */
void _tm_recv_event_post_reply(const char* topic, const uint8_t *payload, size_t len, void *pUserData);


// gateway.c
// int mapForeach(void *context, struct aws_hash_element *p_element);

bool _check_device_name_legality(const char* device_name);

void to_lowercase(const char* cstr, char* output);

// void* iot_gateway_add_topo_payload(iot_tm_msg_gateway_add_topo_t* pty);


// int32_t _tm_send_gateway_add_topo(void* handler, const char* topic, const void* msg_p);

// void _tm_recv_gateway_add_topo_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);


// void* iot_gateway_delete_topo_payload(iot_tm_msg_gateway_delete_topo_t* pty);


// int32_t _tm_send_gateway_delete_topo(void* handler, const char* topic, const void* msg_p);

// /*
// static void _tm_recv_gateway_unsubscribe_topic_handler(struct aws_mqtt_client_connection *connection,
//                                                        uint16_t packet_id,
//                                                        int error_code,
//                                                        void *userdata);
//                                                       */

// void _tm_recv_gateway_delete_topo_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);


// void* iot_gateway_get_topo_payload(iot_tm_msg_gateway_get_topo_t* pty);

// int32_t _tm_send_gateway_get_topo(void* handler, const char* topic, const void* msg_p);

// void _tm_recv_gateway_get_topo_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);


// void* iot_gateway_get_device_secret_payload(iot_tm_msg_gateway_get_device_secret_t* pty);

// int32_t _tm_send_gateway_get_device_secret(void* handler, const char* topic, const void* msg_p);

// void _tm_recv_gateway_get_device_secret_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);


// void* iot_gateway_sub_device_login_payload(iot_tm_msg_gateway_sub_device_login_t* pty);

// int32_t _tm_send_gateway_sub_device_login(void* handler, const char* topic, const void* msg_p);

// void _tm_recv_gateway_sub_device_login_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);


// void* iot_gateway_sub_device_logout_payload(iot_tm_msg_gateway_sub_device_logout_t* pty);

// int32_t _tm_send_gateway_sub_device_logout(void* handler, const char* topic, const void* msg_p);

// void _tm_recv_gateway_sub_device_logout_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);


// void* iot_gateway_sub_device_discovery_payload(iot_tm_msg_gateway_sub_device_discovery_t* pty);

// int32_t _tm_send_gateway_sub_device_discovery(void* handler, const char* topic, const void* msg_p);

// void _tm_recv_gateway_sub_device_discovery_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);


// // recv server add topo notify 's msg
// void _tm_recv_gateway_add_topo_notify_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);

// // reply server add topo notify
// void __send_gateway_add_topo_notify_reply(void* handle, const char* id, int64_t time_stamp, const char* trace_id);

// int32_t _tm_send_gateway_add_topo_notify_reply(void* handler, const char* topic, const void* msg_p);


// void iot_gateway_add_topo_notify_reply_init(iot_tm_msg_gateway_add_topo_notify_reply_t** pty, const char* id, int64_t time_stamp, const char* trace_id);

// void iot_gateway_add_topo_notify_reply_free(iot_tm_msg_gateway_add_topo_notify_reply_t* pty);

// void* iot_gateway_add_topo_notify_reply_payload(iot_tm_msg_gateway_add_topo_notify_reply_t* pty);


// // reply server topo change notify
// void __send_gateway_topo_change_notify_reply(void* handle, const char* id);

// // recv server topo change notify 's msg
// void _tm_recv_gateway_topo_change_notify_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);

// void iot_gateway_topo_change_notify_reply_init(iot_tm_msg_gateway_topo_change_notify_reply_t** pty, const char* id);

// void* iot_gateway_topo_change_notify_reply_payload(iot_tm_msg_gateway_topo_change_notify_reply_t* pty);

// void iot_gateway_topo_change_notify_reply_free(iot_tm_msg_gateway_topo_change_notify_reply_t* pty);

// int32_t _tm_send_gateway_topo_change_notify_reply(void* handler, const char* topic, const void* msg_p);


// // reply server sub device status change notify
// void __send_gateway_sub_device_change_notify_reply(void* handle, const char* id);

// // recv server sub device status change notify 's msg
// void _tm_recv_gateway_sub_device_change_notify_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);

// void iot_gateway_sub_device_change_notify_reply_init(iot_tm_msg_gateway_sub_device_change_notify_reply_t** pty, const char* id);

// void* iot_gateway_sub_device_change_notify_reply_payload(iot_tm_msg_gateway_sub_device_change_notify_reply_t* pty);

// void iot_gateway_sub_device_change_notify_reply_free(iot_tm_msg_gateway_sub_device_change_notify_reply_t* pty);

// int32_t _tm_send_gateway_sub_device_change_notify_reply(void* handler, const char* topic, const void* msg_p);



// iot_ntp.c
void _tm_recv_device_ntp_info(const char* topic, const uint8_t *payload, size_t len, void *pUserData);


// property.c
void* iot_property_post_payload(iot_tm_msg_property_post_t *pty);

int32_t _tm_send_property_post(void *handle, const char *topic, const void *msg);

int32_t _tm_send_property_set_post_reply(void *handle, const char *topic, const void *msg);

void _tm_recv_property_set_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);

void* iot_property_set_post_reply_payload(iot_tm_msg_property_set_post_reply_t *pty);

void __tm_send_server_property_set_reply(void *iot_tm_handler, const char* msg_id);


void _tm_recv_property_set_post_reply(const char* topic, const uint8_t *payload, size_t len, void *pUserData);


// service.c
/**
 * 接收 服务端下发的 service call
 */
void _tm_recv_service_call(const char* topic, const uint8_t *payload, size_t len, void *pUserData);

void* iot_tm_msg_service_call_reply_payload(iot_tm_msg_service_call_reply_t *reply);

int32_t _tm_send_service_call_reply(void *handler, const char *topic, const void *msg_p);

void __tm_send_server_service_call_reply(void* handler, const char* module_key, const char* identifier,
                                         const char* topic_uuid,const char* msg_id, int32_t code);


// shadows.c

void iot_shadow_post_add_param_object(iot_tm_msg_shadow_post_t* pty, const char* key, struct aws_json_value *value);

void* iot_shadow_post_payload(iot_tm_msg_shadow_post_t* pty);

int32_t _tm_send_shadow_post(void* handler, const char* topic, const void* msg_p);

void _tm_recv_shadow_report_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);


void* iot_shadow_get_payload(iot_tm_msg_shadow_get_t* pty);

int32_t _tm_send_shadow_get(void* handler, const char* topic, const void* msg_p);

void _tm_recv_shadow_get_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);

void* iot_shadow_clear_payload(iot_tm_msg_shadow_clear_post_t* pty);

int32_t _tm_send_shadow_clear(void* handler, const char* topic, const void* msg_p);

void _tm_recv_shadow_set_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);

void __send_shadow_clear(void* handle);


// webshell.c
// recv server webshell cmd  's msg
void _tm_recv_webshell_command_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData);

void* iot_webshell_command_reply_payload(iot_tm_recv_webshell_command_reply_t* pty);

void __send_webshell_command_reply(void* handle, const char* id);

int32_t _tm_send_webshell_command_reply(void* handler, const char* topic, const void* msg_p);


const char* iot_webshell_command_pong_payload(iot_tm_msg_webshell_command_pong_t* pty);

void __send_webshell_command_pong(void* handle, const char* uid, const char* pong);

int32_t _tm_send_webshell_command_pong(void* handler, const char* topic, const void* msg_p);


// iot_tm_api.c
typedef struct iot_tm_handler{
    iot_mqtt_ctx_t *mqtt_handle;
    void *mqtt_client;
    struct aws_allocator *allocator;
    iot_tm_recv_handler_t *recv_handler;
    void *userdata;
} iot_tm_handler_t;

int _s_tm_set_up_mqtt_topic(iot_tm_handler_t *iot_tm_handler);

int __s_tm_set_up_mqtt_topic(iot_tm_handler_t *iot_tm_handler, struct aws_string* product_key, struct aws_string* device_name);

/*
typedef struct {
    iot_tm_handler_t *handle;
    struct aws_byte_cursor payload_cur;
    struct aws_byte_cursor public_topic;
    iot_tm_msg_t msg_copy;
} mqtt_post_on_complete_data_t;

void _tm_mqtt_post_on_complete_fn(
        struct aws_mqtt_client_connection *connection,
        uint16_t packet_id,
        int error_code,
        void *userdata);

*/

void _tm_recv_empty(const char* topic, const uint8_t *payload, size_t len, void *pUserData);



#endif //ARENAL_IOT_IOT_TM_HEADER_H
