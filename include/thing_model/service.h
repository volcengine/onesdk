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



#ifndef ARENAL_IOT_SERVICE_H
#define ARENAL_IOT_SERVICE_H

#include <stdint.h>

typedef struct {
    const char* id;
    const char* version;
    const char* module_key;
    const char* identifier;
    const char* topic_uuid;
    int32_t code;
    void *params;
    void *payload_root;
} iot_tm_msg_service_call_reply_t;

typedef struct {
    char *msg_id;
    char *module_key;
    char *identifier;
    char *version;
    char *params_json_str;
    char *topic_uuid;
} iot_tm_recv_service_call_t;

void iot_tm_msg_service_call_reply_init(iot_tm_msg_service_call_reply_t **reply,
                                         const char* module_key, const char* identifier,
                                         const char* topic_uuid,const char* msgId, int32_t code
);

void iot_tm_msg_service_call_reply_set_prams_json_str(iot_tm_msg_service_call_reply_t *reply, const char* param_json_str);

void iot_tm_msg_service_call_reply_free(iot_tm_msg_service_call_reply_t *reply);



#endif //ARENAL_IOT_SERVICE_H
