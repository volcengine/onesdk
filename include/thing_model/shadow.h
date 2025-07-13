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



#ifndef ARENAL_IOT_SHADOW_H
#define ARENAL_IOT_SHADOW_H

#include <stdint.h>

typedef struct {
    const char* id;
    const char* version;
    void* params;
    void* report;
    void* payload_root;
} iot_tm_msg_shadow_post_t;

typedef struct {
    char* msg_id;
    int64_t version;
    char* reply_json_str;
} iot_tm_recv_shadow_post_reply_t;


typedef struct {
    const char* id;
    const char* version;
    void* payload_root;
} iot_tm_msg_shadow_get_t;

typedef struct {
    char* msg_id;
    int64_t version;
    char* desired_json_str;

} iot_tm_recv_shadow_get_reply_t;

typedef struct {
    const char* id;
    const char* version;
    int64_t shadow_version;
    void* payload_root;
} iot_tm_msg_shadow_clear_post_t;

typedef struct {
    int64_t shadow_version;
    char* desired_json_str;
} iot_tm_recv_shadow_set_t;


void iot_shadow_post_init(iot_tm_msg_shadow_post_t** pty);

void iot_shadow_post_init_with_id(iot_tm_msg_shadow_post_t** pty, const char* id);

void iot_shadow_post_add_param_num(iot_tm_msg_shadow_post_t* pty, const char* key, double value);

void iot_shadow_post_add_param_json_str(iot_tm_msg_shadow_post_t* pty, const char* key, const char* json_val);

void iot_shadow_post_add_param_string(iot_tm_msg_shadow_post_t* pty, const char* key, char const* value);

void iot_shadow_post_free(iot_tm_msg_shadow_post_t* pty);


void iot_shadow_get_init(iot_tm_msg_shadow_get_t** pty);

void iot_shadow_get_init_with_id(iot_tm_msg_shadow_get_t** pty, const char* id);

void iot_shadow_get_free(iot_tm_msg_shadow_get_t* pty);



// send clear msg when recv shadow get reply
void iot_shadow_clear_init(iot_tm_msg_shadow_clear_post_t** pty) ;

void iot_shadow_clear_init_with_id(iot_tm_msg_shadow_clear_post_t** pty,const char* id);

void iot_shadow_clear_free(iot_tm_msg_shadow_clear_post_t* pty);





#endif // AGENAL_IOT_SHADOW

