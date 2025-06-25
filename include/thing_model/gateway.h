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



// #ifndef ARENAL_IOT_GATEWAY_H
// #define ARENAL_IOT_GATEWAY_H

// #include <stdint.h>

// typedef struct {
//     const char* product_key;
//     const char* device_name;
//     const char* product_secret;
// } iot_tm_msg_gateway_add_topo_item_t ;

// typedef struct {
//     const char* id;
//     const char* version;
//     const void* gateway_topo_list;
//     const void* payload_root;
// } iot_tm_msg_gateway_add_topo_t;

// typedef struct {

// } iot_tm_msg_gateway_add_topo_reply_t;

// typedef struct {
//     const char* id;
//     const char* version;
//     void* gateway_topo_list;
//     void* payload_root;
// } iot_tm_msg_gateway_delete_topo_t;

// typedef struct {
//     const char* product_key;
//     const char* device_name;
// } iot_tm_msg_gateway_delete_topo_item_t;

// typedef struct {

// } iot_tm_msg_gateway_delete_topo_reply_t;

// typedef struct {
//     const char* id ;
//     const char* version;
//     void* payload_root;
// } iot_tm_msg_gateway_get_topo_t;

// typedef struct {
//     const char* product_key;
//     const char* device_name;
// } iot_tm_msg_gateway_get_topo_item_t;

// typedef struct {
//     iot_tm_msg_gateway_get_topo_item_t** gateway_topo_list;
// } iot_tm_msg_gateway_get_topo_reply_t;

// typedef struct {
//     const char* product_key;
//     const char* device_name;

// } iot_tm_msg_gateway_get_device_secret_item_t;

// typedef struct {
//     const char* id ;
//     const char* version;
//     const char* uuid; // topic fmt need
//     void* gateway_topo_list;
//     void* payload_root;

// } iot_tm_msg_gateway_get_device_secret_t;

// typedef struct {

// } iot_tm_msg_gateway_get_device_secret_reply_t;

// typedef struct {
//     const char* product_key;
//     const char* device_name;
// } iot_tm_msg_gateway_sub_device_login_item_t;

// typedef struct {
//     const char* id;
//     const char* version;
//     void* gateway_topo_list;
//     void* payload_root;

// } iot_tm_msg_gateway_sub_device_login_t;

// typedef struct {

// } iot_tm_msg_gateway_sub_device_login_reply_t;

// typedef struct {
//     const char* product_key;
//     const char* device_name;

// } iot_tm_msg_gateway_sub_device_logout_item_t;

// typedef struct {
//     const char* id;
//     const char* version;
//     void* gateway_topo_list;
//     void* payload_root;

// } iot_tm_msg_gateway_sub_device_logout_t;


// typedef struct {

// } iot_tm_msg_gateway_sub_device_logout_reply_t;

// typedef struct {
//     struct aws_string* product_key;
//     struct aws_string* device_name;

// } iot_tm_msg_gateway_sub_device_discovery_item_t;

// typedef struct {
//     const char* id;
//     const char* version;
//     void* gateway_topo_list;
//     void* payload_root;

// } iot_tm_msg_gateway_sub_device_discovery_t;


// typedef struct {

// } iot_tm_msg_gateway_sub_device_discovery_reply_t;

// typedef struct {
//     const char* product_key;
//     const char* device_name;
// } iot_tm_recv_gateway_add_topo_notify_item_t;

// typedef struct {
//     iot_tm_recv_gateway_add_topo_notify_item_t** gateway_topo_list;
// } iot_tm_recv_gateway_add_topo_notify_t;


// typedef enum {
//     IOT_GATEWAY_TOPO_CHANGE_TYPE_CREATE ,
//     IOT_GATEWAY_TOPO_CHANGE_TYPE_DELETE,
//     IOT_GATEWAY_TOPO_CHANGE_TYPE_ENABLE,
//     IOT_GATEWAY_TOPO_CHANGE_TYPE_DISABLE,
// } iot_gateway_topo_change_type_t;

// typedef enum {
//     IOT_GATEWAY_SUB_DEVICE_CHANGE_TYPE_CREATE ,
//     IOT_GATEWAY_SUB_DEVICE_CHANGE_TYPE_DELETE,
//     IOT_GATEWAY_SUB_DEVICE_CHANGE_TYPE_ENABLE,
//     IOT_GATEWAY_SUB_DEVICE_CHANGE_TYPE_DISABLE,
// } iot_gateway_sub_device_change_type_t;

// typedef struct {
//     const char* product_key;
//     const char* device_name;
//     const char* device_secrt;
// } iot_tm_recv_gateway_topo_change_notify_item_t;

// typedef struct {
//     iot_gateway_topo_change_type_t change_type;
//     iot_tm_recv_gateway_topo_change_notify_item_t** gateway_topo_list;
// } iot_tm_recv_gateway_topo_change_notify_t;


// typedef struct {
//     const char* product_key;
//     const char* device_name;
// } iot_tm_recv_gateway_sub_device_change_notify_item_t;

// typedef struct {
//     iot_gateway_sub_device_change_type_t change_type;
//     iot_tm_recv_gateway_sub_device_change_notify_item_t** gateway_topo_list;
// } iot_tm_recv_gateway_sub_device_change_notify_t;

// typedef struct {
//     const char* id;
//     int32_t code;
//     int64_t time_stamp;
//     const char* trace_id;
//     void *payload_root;
// } iot_tm_msg_gateway_add_topo_notify_reply_t;

// typedef struct {
//     const char* id;
//     int32_t code;
//     void* payload_root;
// } iot_tm_msg_gateway_topo_change_notify_reply_t;

// typedef struct {
//     const char* id;
//     int32_t code;
//     void* payload_root;
// } iot_tm_msg_gateway_sub_device_change_notify_reply_t;


// void iot_gateway_add_topo_init(iot_tm_msg_gateway_add_topo_t** pty);

// int32_t iot_gateway_add_topo_item(iot_tm_msg_gateway_add_topo_t* pty,const char* device_name, const char* product_key, const char* product_secret);

// void iot_gateway_add_topo_free(iot_tm_msg_gateway_add_topo_t* pty);


// void iot_gateway_delete_topo_init(iot_tm_msg_gateway_delete_topo_t** pty);

// void iot_gateway_delete_topo_item(iot_tm_msg_gateway_delete_topo_t* pty, const char* device_name, const char* product_key);

// void iot_gateway_delete_topo_free(iot_tm_msg_gateway_delete_topo_t* pty);


// void iot_gateway_get_topo_init(iot_tm_msg_gateway_get_topo_t** pty);

// void iot_gateway_get_topo_init_with_id(iot_tm_msg_gateway_get_topo_t** pty, const char* id);

// void iot_gateway_get_topo_free(iot_tm_msg_gateway_get_topo_t* pty);


// void iot_gateway_get_device_secret_init(iot_tm_msg_gateway_get_device_secret_t** pty);

// int32_t iot_gateway_get_device_secret_item(iot_tm_msg_gateway_get_device_secret_t* pty, const char* device_name, const char* product_key);

// void iot_gateway_get_device_secret_free(iot_tm_msg_gateway_get_device_secret_t* pty);


// void iot_gateway_sub_device_login_init(iot_tm_msg_gateway_sub_device_login_t** pty);

// int32_t iot_gateway_sub_device_login_item(iot_tm_msg_gateway_sub_device_login_t* pty, const char* device_name, const char* product_key);

// void iot_gateway_sub_device_login_free(iot_tm_msg_gateway_sub_device_login_t* pty);


// void iot_gateway_sub_device_logout_init(iot_tm_msg_gateway_sub_device_logout_t** pty);

// int32_t iot_gateway_sub_device_logout_item(iot_tm_msg_gateway_sub_device_logout_t* pty, const char* device_name, const char* product_key);

// void iot_gateway_sub_device_logout_free(iot_tm_msg_gateway_sub_device_logout_t* pty);


// void iot_gateway_sub_device_discovery_init(iot_tm_msg_gateway_sub_device_discovery_t** pty);

// int32_t iot_gateway_sub_device_discovery_item(iot_tm_msg_gateway_sub_device_discovery_t* pty, const char* device_name, const char* product_key);

// void iot_gateway_sub_device_discovery_free(iot_tm_msg_gateway_sub_device_discovery_t* pty);


// #endif // ARENAL_IOT_GATEWAY_H