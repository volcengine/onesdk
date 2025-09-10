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
#include "aws/common/string.h"
#include "error_code.h"
#include "thing_model/iot_tm_api.h"
#include "iot_mqtt.h"
#include "thing_model/event.h"
#include "thing_model/service.h"
#include "thing_model/device_delay.h"
#include "thing_model/iot_tm_header.h"
#include "iot/iot_utils.h"
#include "util/util.h"
#include <stdlib.h>

typedef struct {
    iot_tm_recv_type_t type;
    char *topic;
    OnMessageHandler func;
} tm_recv_topic_map_t;

// 客户端发送
static const tm_send_topic_map_t g_dm_send_topic_mapping[IOT_TM_MSG_MAX] = {
        {
                // 属性上报
                "sys/%s/%s/thingmodel/property/post",
                _tm_send_property_post
        },
        {
                // 收到属性下发之后, 给服务端的回复
                "sys/%s/%s/thingmodel/service/preset/propertySet/post_reply",
                _tm_send_property_set_post_reply
        },
        {
                // 上报设备影子
            "sys/%s/%s/shadow/report",
            _tm_send_shadow_post
        },
        {
                // 获取设备影子
                "sys/%s/%s/shadow/desired/get",
                _tm_send_shadow_get
        },{
                // 清除设备影子
                "sys/%s/%s/shadow/desired/clear",
                _tm_send_shadow_clear
        // },{
        //         // gateway add
        //         "sys/%s/%s/gateway/topo/add",
        //         _tm_send_gateway_add_topo
        // },{
        //         // gateway delete
        //         "sys/%s/%s/gateway/topo/delete",
        //         _tm_send_gateway_delete_topo
        // },{
        //         // gateway get
        //         "sys/%s/%s/gateway/topo/get",
        //         _tm_send_gateway_get_topo
        // },{
        //         // gateway get device secret
        //         "sys/%s/%s/gateway/sub/secret/get/%s",
        //         _tm_send_gateway_get_device_secret
        // },{
        //         // gateway sub device login
        //         "sys/%s/%s/gateway/sub/login",
        //         _tm_send_gateway_sub_device_login
        // },{
        //         // gateway sub device logout
        //         "sys/%s/%s/gateway/sub/logout",
        //         _tm_send_gateway_sub_device_logout
        // },{
        //         // gateway sub device discovery
        //         "sys/%s/%s/gateway/sub/discovery",
        //         _tm_send_gateway_sub_device_discovery
        // },{
        //         // gateway add topo notify reply
        //         "sys/%s/%s/gateway/topo/notify_reply/%s",
        //         _tm_send_gateway_add_topo_notify_reply
        // },{
        //         // gateway topo change notify reply
        //         "sys/%s/%s/gateway/topo/change_reply",
        //         _tm_send_gateway_topo_change_notify_reply
        // },{
        //         // gateway sub device status change notify reply
        //         "sys/%s/%s/gateway/sub/change_reply",
        //         _tm_send_gateway_sub_device_change_notify_reply
        },{
                // webshell command reply
                "sys/%s/%s/webshell/cmd/%s/post_reply",
                _tm_send_webshell_command_reply
        },{
                // webshell command pong reply,  topic is same as webshell command，data is not same
                "sys/%s/%s/webshell/cmd/%s/post_reply",
                _tm_send_webshell_command_pong
        },
        {
                // 事件上报
                "sys/%s/%s/thingmodel/event/%s/%s/post",
                _tm_send_event_post
        },
        {
                // 服务端调用回复
                "sys/%s/%s/thingmodel/service/%s/%s/post_reply/%s",
                _tm_send_service_call_reply
        },
        {
                // 自定义topic
                "sys/%s/%s/custom/%s",
                _tm_send_custom_topic_post_data
        },
};


// 服务端下发
static const tm_recv_topic_map_t g_dm_recv_topic_mapping[] = {
        {
                // 服务端下发的属性设置
                IOT_TM_RECV_PROPERTY_SET,
                "sys/%s/%s/thingmodel/service/preset/propertySet/post",
                _tm_recv_property_set_handler,
        },
        {
                // 设置属性后, 服务端给的回复
                IOT_TM_RECV_PROPERTY_SET_POST_REPLY,
                "sys/%s/%s/thingmodel/property/post_reply",
                _tm_recv_property_set_post_reply,
        },
        {
                // 上报设备影子后，服务端给的回复
                IOT_TM_RECV_SHADOW_REPORT_REPLY,
                "sys/%s/%s/shadow/report_reply",
                _tm_recv_shadow_report_reply_handler,
        },
        {
                // 获取设备影子后，服务端给的回复
                IOT_TM_RECV_SHADOW_GET_REPLY,
                "sys/%s/%s/shadow/desired/get/reply",
                _tm_recv_shadow_get_reply_handler,
        },{
                // 服务端下发影子设备设置
                IOT_TM_RECV_SHADOW_SET,
                "sys/%s/%s/shadow/desired/set",
                _tm_recv_shadow_set_handler,
        },{
        //         // 增加子设备网络拓扑关系, 服务端给的回复
        //         IOT_TM_RECV_GATEWAY_ADD_TOPO_REPLY,
        //         "sys/%s/%s/gateway/topo/add_reply",
        //         _tm_recv_gateway_add_topo_reply_handler,
        // },{
        //         // 删除子设备网络拓扑关系, 服务端给的回复
        //         IOT_TM_RECV_GATEWAY_DELETE_TOPO_REPLY,
        //         "sys/%s/%s/gateway/topo/delete_reply",
        //         _tm_recv_gateway_delete_topo_reply_handler,
        // },{
        //         // 获取网络拓扑关系, 服务端给的回复
        //         IOT_TM_RECV_GATEWAY_GET_TOPO_REPLY,
        //         "sys/%s/%s/gateway/topo/get_reply",
        //         _tm_recv_gateway_get_topo_reply_handler,
        // },{
        //         // 获取设备secret, 服务端给的回复
        //         IOT_TM_RECV_GATEWAY_GET_DEVICE_SECRET,
        //         "sys/%s/%s/gateway/sub/secret/get_reply/%s",
        //         _tm_recv_gateway_get_device_secret_reply_handler,
        // },{
        //         // 设备登录，服务端给的回复
        //         IOT_TM_RECV_GATEWAY_SUB_DEVICE_LOGIN,
        //         "sys/%s/%s/gateway/sub/login_reply",
        //         _tm_recv_gateway_sub_device_login_reply_handler,
        // },{
        //         // 设备登出, 服务端给的回复
        //         IOT_TM_RECV_GATEWAY_SUB_DEVICE_LOGOUT,
        //         "sys/%s/%s/gateway/sub/logout_reply",
        //         _tm_recv_gateway_sub_device_logout_reply_handler,
        // },{
        //         // 设备发现上报, 服务端给的回复
        //         IOT_TM_RECV_GATEWAY_SUB_DEVICE_DISCOVERY,
        //         "sys/%s/%s/gateway/sub/discovery_reply",
        //         _tm_recv_gateway_sub_device_discovery_reply_handler,
        // },{
        //         // 收到服务端下发的网络拓扑关系增加通知
        //         IOT_TM_RECV_GATEWAY_ADD_TOPO_NOTIFY,
        //         "sys/%s/%s/gateway/topo/notify/+",
        //         _tm_recv_gateway_add_topo_notify_handler,
        // },{
        //         // 收到服务端下发的网络拓扑变化通知
        //         IOT_TM_RECV_GATEWAY_TOPO_CHANGE_NOTIFY,
        //         "sys/%s/%s/gateway/topo/change",
        //         _tm_recv_gateway_topo_change_notify_handler,
        // },{
        //         // 收到服务端下发的子设备状态变化通知
        //         IOT_TM_RECV_GATEWAY_SUB_DEVICE_CHANGE_NOTIFY,
        //         "sys/%s/%s/gateway/sub/change",
        //         _tm_recv_gateway_sub_device_change_notify_handler,
        // },{
                // 收到服务端下发的webshell 命令
                IOT_TM_RECV_WEBSHELL_COMMAND,
                "sys/%s/%s/webshell/cmd/+/post",
                _tm_recv_webshell_command_handler,
        },
        {
                // event 上报之后, 服务端给的回复
                IOT_TM_RECV_EVENT_POST_REPLY,
                "sys/%s/%s/thingmodel/event/+/+/post_reply",
                _tm_recv_event_post_reply,
        },
        {
                // event  服务端下发 服务调用
                IOT_TM_RECV_SERVICE_CALL,
                "sys/%s/%s/thingmodel/service/+/+/post/+",
                _tm_recv_service_call,
        },
        {
                // event 自定义topic给的回复
                IOT_TM_RECV_CUSTOM_TOPIC,
                "sys/%s/%s/custom/+",
                NULL, // 每个 自定义 topic 都是独立订阅, 这里给一个空的方法
        },
        {
                // event 收到服务端时间的回复
                IOT_TM_RECV_NTP_SERVER_TIME,
                "sys/%s/%s/ntp/response",
                _tm_recv_device_ntp_info, // 每个 自定义 topic 都是独立订阅, 这里给一个空的方法
        },
};


iot_tm_handler_t *iot_tm_init(void) {
    iot_tm_handler_t *dm_handle = (iot_tm_handler_t *)malloc(sizeof(iot_tm_handler_t));
    if (dm_handle == NULL) {
        return NULL;
    }
    memset(dm_handle, 0, sizeof(iot_tm_handler_t));
    dm_handle->allocator = aws_alloc();
    return dm_handle;
}

void iot_tm_deinit(iot_tm_handler_t *handle) {
    if (handle == NULL) {
        return;
    }
    free(handle);
}

int iot_tm_set_mqtt_handler(iot_tm_handler_t *handle, iot_mqtt_ctx_t *mqtt_handle) {
    handle->mqtt_handle = mqtt_handle;
    int ret = _s_tm_set_up_mqtt_topic(handle);
    if (ret != 0) {
        return ret;
    }
    ret = _sub_device_delay_info(handle);
    if (ret!= 0) {
        return ret;
    }
    return VOLC_OK;
}

char* __dm_prepare_rev_topic(struct aws_allocator *allocator, struct aws_string* product_key, struct aws_string* device_name, tm_recv_topic_map_t topic_map_item) {
    switch (topic_map_item.type) {
        case IOT_TM_RECV_PROPERTY_SET:
        case IOT_TM_RECV_PROPERTY_SET_POST_REPLY:
        case IOT_TM_RECV_SHADOW_REPORT_REPLY:
        case IOT_TM_RECV_SHADOW_GET_REPLY:
        case IOT_TM_RECV_SHADOW_SET:
        case IOT_TM_RECV_GATEWAY_ADD_TOPO_REPLY:
        case IOT_TM_RECV_GATEWAY_DELETE_TOPO_REPLY:
        case IOT_TM_RECV_GATEWAY_GET_TOPO_REPLY:
        case IOT_TM_RECV_GATEWAY_SUB_DEVICE_LOGIN:
        case IOT_TM_RECV_GATEWAY_SUB_DEVICE_LOGOUT:
        case IOT_TM_RECV_GATEWAY_SUB_DEVICE_DISCOVERY:
        case IOT_TM_RECV_GATEWAY_ADD_TOPO_NOTIFY:
        case IOT_TM_RECV_GATEWAY_TOPO_CHANGE_NOTIFY:
        case IOT_TM_RECV_GATEWAY_SUB_DEVICE_CHANGE_NOTIFY:
        case IOT_TM_RECV_WEBSHELL_COMMAND:
        case IOT_TM_RECV_EVENT_POST_REPLY:
        case IOT_TM_RECV_SERVICE_CALL:
        case IOT_TM_RECV_CUSTOM_TOPIC:
        case IOT_TM_RECV_NTP_SERVER_TIME:{
            return iot_get_common_topic(allocator, topic_map_item.topic, product_key, device_name);
        }
        case IOT_TM_RECV_GATEWAY_GET_DEVICE_SECRET: {
            return iot_get_topic_with_1_c_str_param(allocator, topic_map_item.topic, product_key, device_name, "+");
        }
        default:
            break;
    }
    return NULL;
}

char *_dm_prepare_rev_topic(iot_tm_handler_t *dm_handle, tm_recv_topic_map_t topic_map_item) {
    if (dm_handle == NULL || dm_handle->mqtt_handle == NULL || dm_handle->mqtt_handle->config == NULL || 
        dm_handle->mqtt_handle->config->basic_config == NULL) {
        return NULL;
    }
    struct aws_string *product_key = aws_string_new_from_c_str(dm_handle->allocator, 
        dm_handle->mqtt_handle->config->basic_config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(dm_handle->allocator, 
        dm_handle->mqtt_handle->config->basic_config->device_name);    
    char* topic = __dm_prepare_rev_topic(dm_handle->allocator, product_key, device_name, topic_map_item);
    aws_string_destroy_secure(product_key);
    aws_string_destroy_secure(device_name);
    return topic;
}

int _s_tm_set_up_mqtt_topic(iot_tm_handler_t *iot_tm_handler) {
    if (iot_tm_handler == NULL || iot_tm_handler->mqtt_handle == NULL || iot_tm_handler->mqtt_handle->config == NULL || 
        iot_tm_handler->mqtt_handle->config->basic_config == NULL) {
        return VOLC_ERR_INVALID_PARAM;
    }
    struct aws_string *product_key = aws_string_new_from_c_str(iot_tm_handler->allocator, 
        iot_tm_handler->mqtt_handle->config->basic_config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(iot_tm_handler->allocator, 
        iot_tm_handler->mqtt_handle->config->basic_config->device_name);
    int result = __s_tm_set_up_mqtt_topic(iot_tm_handler, product_key, device_name);
    aws_string_destroy_secure(product_key);
    aws_string_destroy_secure(device_name);
    return result;
}

int __s_tm_set_up_mqtt_topic(iot_tm_handler_t *iot_tm_handler, struct aws_string* product_key, struct aws_string* device_name) {
    // 订阅处理
    int i = 0;
    for (i = 0; i < sizeof(g_dm_recv_topic_mapping) / sizeof(tm_recv_topic_map_t); i++) {
        iot_mqtt_topic_map_t topic_mapping;
        topic_mapping.topic = __dm_prepare_rev_topic(iot_tm_handler->allocator, product_key, device_name, g_dm_recv_topic_mapping[i]);
        if (topic_mapping.topic == NULL) {
            continue;
        }
        topic_mapping.message_callback = g_dm_recv_topic_mapping[i].func;
        topic_mapping.user_data = iot_tm_handler;
        if (topic_mapping.message_callback == NULL) {
            aws_mem_release(iot_tm_handler->allocator, (void*)topic_mapping.topic);
            continue;
        }
        topic_mapping.qos = IOT_MQTT_QOS1;

        iot_mqtt_subscribe(iot_tm_handler->mqtt_handle, &topic_mapping);

        // 这里可以回收前面申请的内存
        aws_mem_release(iot_tm_handler->allocator, (void*)topic_mapping.topic);
    }
    return VOLC_OK;
}

void iot_tm_set_tm_recv_handler_t(iot_tm_handler_t *handle, iot_tm_recv_handler_t *recv_handler, void *userdata) {
    handle->recv_handler = recv_handler;
    handle->userdata = userdata;
}


//发送数据给服务端的处理流程
static int32_t _dm_prepare_send_topic(iot_tm_handler_t *dm_handle, const iot_tm_msg_t *msg, char **topic) {
    struct aws_string *product_key;
    if (msg->product_key != NULL && secure_strlen(msg->product_key) > 1) {
        product_key = aws_string_new_from_c_str(dm_handle->allocator, msg->product_key);
    } else {
        product_key = aws_string_new_from_c_str(dm_handle->allocator, 
            dm_handle->mqtt_handle->config->basic_config->product_key);
    }

    struct aws_string *device_name;
    if (msg->device_name != NULL && secure_strlen(msg->device_name) > 1) {
        device_name = aws_string_new_from_c_str(dm_handle->allocator, msg->device_name);
    } else {
        device_name = aws_string_new_from_c_str(dm_handle->allocator, 
            dm_handle->mqtt_handle->config->basic_config->device_name);
    }
    int ret = VOLC_OK;

    switch (msg->type) {
        case IOT_TM_MSG_PROPERTY_POST:
        case IOT_TM_MSG_PROPERTY_SET_REPLY: {
            *topic = iot_get_common_topic(dm_handle->allocator, g_dm_send_topic_mapping[msg->type].topic, product_key, device_name);
        }
            break;
        case IOT_TM_MSG_SHADOW_REPORT:
        case IOT_TM_MSG_SHADOW_GET:
        case IOT_TM_MSG_SHADOW_CLEAR:{
            *topic = iot_get_common_topic(dm_handle->allocator, g_dm_send_topic_mapping[msg->type].topic, product_key, device_name);
        }
            break;
        // case IOT_TM_MSG_GATEWAY_ADD_TOPO:
        // case IOT_TM_MSG_GATEWAY_DELETE_TOPO:
        // case IOT_TM_MSG_GATEWAY_GET_TOPO:
        // case IOT_TM_MSG_GATEWAY_SUB_DEVICE_LOGIN:
        // case IOT_TM_MSG_GATEWAY_SUB_DEVICE_LOGOUT:
        // case IOT_TM_MSG_GATEWAY_SUB_DEVICE_DISCOVERY:
        // case IOT_TM_MSG_GATEWAY_TOPO_CHANGE_NOTIFY_REPLY:
        // case IOT_TM_MSG_GATEWAY_SUB_DEVICE_CHANGE_NOTIFY_REPLY: {
        //     *topic = iot_get_common_topic(dm_handle->allocator, 
        //         g_dm_send_topic_mapping[msg->type].topic, product_key, device_name);
        // }
        //     break;
        // case IOT_TM_MSG_GATEWAY_GET_DEVICE_SECRET:{
        //     *topic = iot_get_topic_with_1_c_str_param(dm_handle->allocator, 
        //         g_dm_send_topic_mapping[msg->type].topic, product_key, device_name, 
        //             (char*) msg->data.gateway_get_device_secret->uuid);
        // }
        //     break;
        // case IOT_TM_MSG_GATEWAY_ADD_TOPO_NOTIFY_REPLY: {
        //     *topic = iot_get_topic_with_1_c_str_param(dm_handle->allocator, 
        //         g_dm_send_topic_mapping[msg->type].topic, product_key, device_name,
        //             (char*) msg->data.gateway_add_topo_notify_reply->trace_id);
        // }
            break;
        case IOT_TM_MSG_WEBSHELL_COMMAND_REPLY: {
            *topic = iot_get_topic_with_1_c_str_param(dm_handle->allocator, 
                g_dm_send_topic_mapping[msg->type].topic, product_key, device_name, 
                    (char *)msg->data.webshell_command_reply->uid);
        }
            break;
        case IOT_TM_MSG_WEBSHELL_COMMAND_PONG: {
            *topic = iot_get_topic_with_1_c_str_param(dm_handle->allocator, 
                g_dm_send_topic_mapping[msg->type].topic, product_key, device_name, 
                        (char *)msg->data.webshell_command_pong->uid);
        }
            break;
        case IOT_TM_MSG_EVENT_POST: {
            *topic = iot_get_topic_with_2_c_str_param(dm_handle->allocator, 
                g_dm_send_topic_mapping[msg->type].topic, product_key, device_name,
                        (char*) msg->data.event_post->module_key,
                        (char*) msg->data.event_post->identifier);
        }
            break;
        case IOT_TM_MSG_SERVICE_CALL_REPLY: {
            *topic = iot_get_topic_with_3_c_str_param(dm_handle->allocator, g_dm_send_topic_mapping[msg->type].topic,
                                                      aws_string_c_str(product_key),
                                                      aws_string_c_str(device_name),
                                                msg->data.service_call_reply->module_key,
                                                msg->data.service_call_reply->identifier,
                                                msg->data.service_call_reply->topic_uuid);
            break;
        }
        case IOT_TM_MSG_CUSTOM_TOPIC: {
            *topic = iot_get_topic_with_1_c_str_param(dm_handle->allocator, g_dm_send_topic_mapping[msg->type].topic,
                                                product_key,
                                                device_name,
                                                msg->data.custom_topic_post->custom_topic_suffix);
            break;
        }
        default:
            ret = VOLC_ERR_DM_PUBLISH_TYPE_UNKNOWN;
            break;
    }
    aws_string_destroy_secure(product_key);
    aws_string_destroy_secure(device_name);
    return VOLC_OK;
}

int32_t iot_tm_send(iot_tm_handler_t *handle, const iot_tm_msg_t *msg) {
    // 发送消息
    if (NULL == handle || NULL == msg) {
        return VOLC_ERR_NULL_POINTER;
    }

    if (msg->type >= IOT_TM_MSG_MAX) {
        return VOLC_ERR_TM_USER_INPUT_OUT_RANGE;
    }

    if (NULL == handle->mqtt_handle) {
        return VOLC_ERR_NULL_POINTER;
    }
    int32_t ret = VOLC_OK;
    char *topic = NULL;
    int32_t prepare_topic_ret = _dm_prepare_send_topic(handle, msg, &topic);
    if (prepare_topic_ret == VOLC_OK) {
        ret = g_dm_send_topic_mapping[msg->type].func(handle, topic, msg);
        aws_mem_release(handle->allocator, topic);
        return ret;
    } else {
        return prepare_topic_ret;
    }
}


// 发送回调成功失败处理
/*
void _tm_mqtt_post_on_complete_fn(
        struct aws_mqtt_client_connection *connection,
        uint16_t packet_id,
        int error_code,
        void *userdata) {
    if (error_code != 0) {
        LOGE(TAG_IOT_MQTT, "_dm_mqtt_post_op_complete_fn failed packet_id = %d, error_code = %d ", packet_id, error_code);
    }
}
*/
#endif //ONESDK_ENABLE_IOT