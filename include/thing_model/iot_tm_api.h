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



#ifndef ARENAL_IOT_IOT_TM_API_H
#define ARENAL_IOT_IOT_TM_API_H

#include "property.h"
#include "shadow.h"
#include "gateway.h"
#include "webshell.h"
#include "event.h"
#include "service.h"
#include "iot_ntp.h"
#include "custom_topic.h"


/**
 * @brief things-model模块发送消息类型
 *
 * @details
 *
 * 这个枚举类型包括了tm模块支持发送的所有数据类型, 不同的消息类型将对于不同的消息结构体
 *
 */
typedef enum {
    /**
     * @brief 属性上报, 消息结构体参考 @ref aiot_tm_msg_property_post_t \n
     */
    IOT_TM_MSG_PROPERTY_POST,

    /**
     * @brief 属性设置应答, 消息结构体参考 @ref aiot_tm_msg_property_set_post_reply_t
     */
    IOT_TM_MSG_PROPERTY_SET_REPLY,

    /**
     * @brief 设备影子上报
     */
    IOT_TM_MSG_SHADOW_REPORT,

    /**
     * @brief 获取设备影子
     */
    IOT_TM_MSG_SHADOW_GET,

    /**
     * @brief 清楚设备影子
     */
    IOT_TM_MSG_SHADOW_CLEAR,

    // /**
    //  * @brief 子设备添加网络拓扑关系
    //  */
    // IOT_TM_MSG_GATEWAY_ADD_TOPO,

    // /**
    //  * @brief 删除子设备网络拓扑关系
    //  */
    // IOT_TM_MSG_GATEWAY_DELETE_TOPO,

    // /**
    //  * @brief 获取网络拓扑关系
    //  */
    // IOT_TM_MSG_GATEWAY_GET_TOPO,

    // /**
    //  * @brief 获取设备secret
    //  */
    // IOT_TM_MSG_GATEWAY_GET_DEVICE_SECRET,

    // /**
    //  * @brief 子设备登录
    //  */
    // IOT_TM_MSG_GATEWAY_SUB_DEVICE_LOGIN,

    // /**
    //  * @brief 子设备登出
    //  */
    // IOT_TM_MSG_GATEWAY_SUB_DEVICE_LOGOUT,

    // /**
    // * @brief 子设备发现上报
    // */
    // IOT_TM_MSG_GATEWAY_SUB_DEVICE_DISCOVERY,


    // /**
    // * @brief 回复服务端下发网络拓扑关系增加的通知后
    // */
    // IOT_TM_MSG_GATEWAY_ADD_TOPO_NOTIFY_REPLY,

    // /**
    // * @brief 回复服务端下发网络拓扑关系变化的通知后
    // */
    // IOT_TM_MSG_GATEWAY_TOPO_CHANGE_NOTIFY_REPLY,

    // /**
    // * @brief 回复服务端下发网络拓扑关系变化的通知后
    // */
    // IOT_TM_MSG_GATEWAY_SUB_DEVICE_CHANGE_NOTIFY_REPLY,

    /**
    * @brief 回复服务端下发的webshell命令
    */
    IOT_TM_MSG_WEBSHELL_COMMAND_REPLY,

    /**
   * @brief 回复服务端下发ping的webshell命令
   */
    IOT_TM_MSG_WEBSHELL_COMMAND_PONG,

    /**
     * @brief 事件上报, , 消息结构体参考 @ref aiot_tm_msg_event_post_t
     */
    IOT_TM_MSG_EVENT_POST,

    /**
     * 服务端调用的回复
     */
    IOT_TM_MSG_SERVICE_CALL_REPLY,

    /**
     * 自定义 topic
     */
    IOT_TM_MSG_CUSTOM_TOPIC,

    IOT_TM_MSG_MAX,
} iot_tm_msg_type_t;


/**
 * @brief things-model模块发送消息的消息结构体
 */
typedef struct {
    /**
     * @brief 消息所属设备的 product_key, 若为NULL则使用通过 mqtt_handle 设置的  product_key
     * 在网关子设备场景下, 可通过指定为子设备的product_key来发送子设备的消息到云端
     */
    char *product_key;
    /**
     * @brief 消息所属设备的 device_name, 若为NULL则使用通过  mqtt_handle 设置的  device_name\n
     * 在网关子设备场景下, 可通过指定为子设备的product_key来发送子设备的消息到云端
     */
    char *device_name;
    /**
     * @brief 消息类型, 可参考@ref aiot_tm_msg_type_t
     */
    iot_tm_msg_type_t type;
    /**
     * @brief 消息数据联合体, 不同的消息类型将使用不同的消息结构体
     */
    union {
        iot_tm_msg_webshell_command_pong_t* webshell_command_pong;
        iot_tm_recv_webshell_command_reply_t* webshell_command_reply;
        // iot_tm_msg_gateway_sub_device_change_notify_reply_t* gateway_sub_device_notify_reply;
        // iot_tm_msg_gateway_topo_change_notify_reply_t* gateway_topo_change_notify_reply;
        // iot_tm_msg_gateway_add_topo_notify_reply_t* gateway_add_topo_notify_reply;
        // iot_tm_msg_gateway_sub_device_discovery_t* gateway_sub_device_discovery;
        // iot_tm_msg_gateway_sub_device_logout_t* gateway_sub_device_logout;
        // iot_tm_msg_gateway_sub_device_login_t* gateway_sub_device_login;
        // iot_tm_msg_gateway_get_device_secret_t* gateway_get_device_secret;
        // iot_tm_msg_gateway_add_topo_reply_t* gateway_get_topo;
        // iot_tm_msg_gateway_delete_topo_t* gateway_delete_topo;
        // iot_tm_msg_gateway_add_topo_t* gateway_add_topo;
        iot_tm_msg_shadow_post_t* shadow_post;
        iot_tm_msg_shadow_get_t* shadow_get;
        iot_tm_msg_shadow_clear_post_t* shadow_clear;
        iot_tm_msg_property_post_t *property_post;
        iot_tm_msg_property_set_post_reply_t *property_set_post_reply;
        iot_tm_msg_event_post_t *event_post;
        iot_tm_msg_service_call_reply_t *service_call_reply;
        iot_tm_msg_custom_topic_post_t *custom_topic_post;
    } data;
} iot_tm_msg_t;


/**
 * @brief things-model模块 服务端下发的消息类型
 *
 * @details
 *
 * 这个枚举类型包括了tm  服务端下发的 所有数据类型, 不同的消息类型将对于不同的消息结构体
 *
 */
typedef enum {
    /**
     * @brief 服务器下发的属性设置消息, 消息数据结构体参考 @ref aiot_tm_recv_property_set_t
     */
    IOT_TM_RECV_PROPERTY_SET,

    /**
     * PROPERTY 设置时候, 服务端给的回复 消息数据结构体参考 @ref aiot_tm_recv_property_set_post_reply
     */
    IOT_TM_RECV_PROPERTY_SET_POST_REPLY,

    /**
     *  上报设备影子服务端的回执
     */
    IOT_TM_RECV_SHADOW_REPORT_REPLY,

    /**
     *  获取设备影子服务端回执
     */
    IOT_TM_RECV_SHADOW_GET_REPLY,

    /**
     *  服务端下发，设备影子
     */
    IOT_TM_RECV_SHADOW_SET,
    /**
     * 添加子设备网络拓扑关系服务端回复
     */
    IOT_TM_RECV_GATEWAY_ADD_TOPO_REPLY,

    /**
     * 删除子设备网络拓扑关系服务端回复
     */
    IOT_TM_RECV_GATEWAY_DELETE_TOPO_REPLY,

    /**
     * 获取网络拓扑关系服务端回复
     */
    IOT_TM_RECV_GATEWAY_GET_TOPO_REPLY,

    /**
     * 获取设备secret
     */
    IOT_TM_RECV_GATEWAY_GET_DEVICE_SECRET,

    /**
     * 子设备登录
     */
    IOT_TM_RECV_GATEWAY_SUB_DEVICE_LOGIN,

    /**
     * 子设备登出
     */
    IOT_TM_RECV_GATEWAY_SUB_DEVICE_LOGOUT,

    /**
    * 子设备发现上报
    */
    IOT_TM_RECV_GATEWAY_SUB_DEVICE_DISCOVERY,

    /**
    * 服务端下发添加拓扑关系
    */
    IOT_TM_RECV_GATEWAY_ADD_TOPO_NOTIFY,

    /**
     * 服务端下发网络拓扑关系变化
     */
    IOT_TM_RECV_GATEWAY_TOPO_CHANGE_NOTIFY,

    /**
    * 服务端下发子设备网络拓扑关系变化
    */
    IOT_TM_RECV_GATEWAY_SUB_DEVICE_CHANGE_NOTIFY,

    /**
   * 服务端下发webshell命令
   */
    IOT_TM_RECV_WEBSHELL_COMMAND,

    /**
     * event 上报之后, 服务端给的回复  消息数据结构体参考 @ref aiot_tm_msg_event_post_reply_t
     */
    IOT_TM_RECV_EVENT_POST_REPLY,

    /**
     * 服务端下发 服务调用
     */
    IOT_TM_RECV_SERVICE_CALL,

    /**
     * 自定义topic给的回复
     */
    IOT_TM_RECV_CUSTOM_TOPIC,

    /**
     * 收到服务端时间的回复
     */
    IOT_TM_RECV_NTP_SERVER_TIME,

    /**
     * @brief 消息数量最大值, 不可用作消息类型
     */
    IOT_TM_RECV_MAX,
} iot_tm_recv_type_t;


/**
 * @brief things-model模块 接收服务端数据的接口体
 */
typedef struct {
    /**
     * @brief 消息所属设备的product_key, 不配置则默认使用MQTT模块配置的product_key
     */
    char *product_key;
    /**
     * @brief 消息所属设备的device_name, 不配置则默认使用MQTT模块配置的device_name
     */
    char *device_name;
    /**
     * @brief 接收消息的类型, 可参考@ref aiot_tm_recv_type_t
     */
    iot_tm_recv_type_t type;
    /**
     * @brief 消息数据联合体, 不同的消息类型将使用不同的消息结构体
     */
    union {
        iot_tm_recv_property_set_t property_set;
        iot_tm_recv_property_set_post_reply property_set_post_reply;
        iot_tm_recv_shadow_post_reply_t shadow_post_reply;
        iot_tm_recv_shadow_get_reply_t shadow_get_reply;
        iot_tm_recv_shadow_set_t shadow_set;
        // iot_tm_msg_gateway_get_topo_reply_t gateway_get;
        // iot_tm_recv_gateway_add_topo_notify_t gateway_add_topo_notify;
        // iot_tm_recv_gateway_topo_change_notify_t gateway_topo_change_notify;
        // iot_tm_recv_gateway_sub_device_change_notify_t gateway_sub_device_change_notify;
        iot_tm_recv_event_post_reply_t event_post_reply;
        iot_tm_recv_service_call_t service_call;
        iot_tm_recv_custom_topic_t custom_topic;
        iot_tm_recv_npt_server_time_t npt_server_time;
    } data;
} iot_tm_recv_t;


/* things-model内部发送函数原型定义 */
typedef int32_t (*tm_msg_send_func_t)(void *handle, const char *topic, const void *msg);

/* 包含上行topic和对应处理函数的结构体定义 */
typedef struct {
    char *topic;
    tm_msg_send_func_t func;
} tm_send_topic_map_t;

typedef void (iot_tm_recv_handler_t)(void *handle, const iot_tm_recv_t *recv, void *userdata);

typedef struct iot_tm_handler iot_tm_handler_t;

/**
 * TM 模块初始化
 * @return
 */
iot_tm_handler_t *iot_tm_init(void);

/**
 * 设置 MQTT handler
 * @param handle
 * @param mqtt_handle
 */
int iot_tm_set_mqtt_handler(iot_tm_handler_t *handle, iot_mqtt_ctx_t *mqtt_handle);

/**
 * 数组数据接收的 handler
 * @param handle
 * @param recv_handler
 * @param userdata 上下文数据, 在iot_tm_recv_handler_t  回调的时候时会带上
 */
void iot_tm_set_tm_recv_handler_t(iot_tm_handler_t *handle, iot_tm_recv_handler_t *recv_handler, void *userdata);


/**
 * 发送 TM 数据
 * @param handle
 * @param msg
 * @return
 */
int32_t iot_tm_send(iot_tm_handler_t *handle, const iot_tm_msg_t *msg);

/**
 * 释放 TM 模块
 * @param handle
 */
void iot_tm_deinit(iot_tm_handler_t *handle);

typedef void (*OnMessageHandler)(const char* topic, const uint8_t *payload, size_t len, void *pUserData);

#endif //ARENAL_IOT_IOT_TM_API_H
