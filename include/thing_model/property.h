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



#ifndef ARENAL_IOT_PROPERTY_H
#define ARENAL_IOT_PROPERTY_H

#include <stdint.h>

typedef struct {
    const char* id;
    const char *version;
    void* params;
    void* payload_root;
} iot_tm_msg_property_post_t;


typedef struct {
    const char *id;
    int32_t code;
    void *payload_root;
} iot_tm_msg_property_set_post_reply_t;

void iot_property_set_post_reply_init(iot_tm_msg_property_set_post_reply_t **pty, const char *id, int32_t code);

void iot_property_set_post_reply_free(iot_tm_msg_property_set_post_reply_t *pty);


void iot_property_post_init(iot_tm_msg_property_post_t **pty);

void iot_property_post_init_with_id(iot_tm_msg_property_post_t **pty, const char *id);

void iot_property_post_add_param_num(iot_tm_msg_property_post_t *pty, const char *key, double value);

void iot_property_post_add_param_json_str(iot_tm_msg_property_post_t *pty,const char *key,const char *json_val);

void iot_property_post_add_param_string(iot_tm_msg_property_post_t *pty,const char *key, const char *value);


void iot_property_post_free(iot_tm_msg_property_post_t *pty);


typedef struct {
    /**
     * @brief 消息标识符, char
     */
    char *msg_id;
    /**
     * @brief 服务器下发的属性数据, 为字符串形式的JSON结构体,  如<i>"{\"LightSwitch\":0}"</i>
     */
    char *params;
    /**
     * @brief 属性数据的字符串长度
     */
    uint32_t params_len;
} iot_tm_recv_property_set_t;


typedef struct {
    // {"ID":"2301669963423725","Code":0,"Data":{}}
    char *msg_id;
    int32_t code;
} iot_tm_recv_property_set_post_reply;




#endif //ARENAL_IOT_PROPERTY_H
