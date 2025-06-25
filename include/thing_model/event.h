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



#ifndef ARENAL_IOT_EVENT_H
#define ARENAL_IOT_EVENT_H

#include <stdint.h>
/**
 * 事件上报 结构体
 */
typedef struct {
    const char* id;
    const char* version;
    const char* module_key;
    const char* identifier;
    void* params;
    void* payloadRoot;
} iot_tm_msg_event_post_t;


/**
 * 初始化 iot_tm_msg_event_post_t 结构体
 */
void iot_tm_msg_event_post_init(iot_tm_msg_event_post_t **event_post, const char* moduleKey, const char* identifier);

/**
 *  向 iot_tm_msg_event_post_t.params json 中 添加 value 为 int  的数据
 */
void iot_tm_msg_event_post_param_add_num(iot_tm_msg_event_post_t *event_post, char *key, double value);

/**
 *  向 iot_tm_msg_event_post_t.params json 中 添加 value 为 string  的数据
 */
void iot_tm_msg_event_post_param_add_string(iot_tm_msg_event_post_t *event_post, char *key, char *value);

/**
 * 设置  iot_tm_msg_event_post_t.params 中的 json 数据
 */
void iot_tm_msg_event_post_set_prams_json_str(iot_tm_msg_event_post_t *event_post, char *param_json_str);

/**
 * 释放 arenal_event_param 占用的内存
 */
void iot_tm_msg_event_post_free(iot_tm_msg_event_post_t *event_post);

typedef struct {
    // {"ID":"2301669963423725","Code":0,"Data":{}}
    char *msg_id;
    char *module_key;
    char *identifier;
    int32_t code;
} iot_tm_recv_event_post_reply_t;




// 接受 event

#endif //ARENAL_IOT_EVENT_H
