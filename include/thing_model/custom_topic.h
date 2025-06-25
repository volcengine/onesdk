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


#ifndef ARENAL_IOT_CUSTOM_TOPIC_H
#define ARENAL_IOT_CUSTOM_TOPIC_H
#include <stdio.h>
#include <stdint.h>

#define TAG_TM_CUSTOM_TOPIC "custom_topic"

typedef struct {
    char *custom_topic_suffix;
    char *params;
} iot_tm_msg_custom_topic_post_t;

typedef struct {
    char *custom_topic_suffix;
    char *params_json_str;
} iot_tm_recv_custom_topic_t;

void iot_tm_msg_aiot_tm_msg_custom_topic_post_init(iot_tm_msg_custom_topic_post_t **custom_topic_post, char *custom_topic_suffix, char *payload_json);

void iot_tm_msg_aiot_tm_msg_custom_topic_post_free(iot_tm_msg_custom_topic_post_t *custom_topic_post);

int32_t tm_sub_custom_topic(void *handler, const char *topic_suffix);



#endif //ARENAL_IOT_CUSTOM_TOPIC_H
