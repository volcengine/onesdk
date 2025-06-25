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



#ifndef ARENAL_IOT_WEBSHELL_H
#define ARENAL_IOT_WEBSHELL_H

#include <stdint.h>

typedef struct {
    const char* uid;
} iot_tm_recv_webshell_command_reply_t;

typedef struct {
    const char* pong;
    const char* uid;
} iot_tm_msg_webshell_command_pong_t;

void iot_webshell_command_reply_init(iot_tm_recv_webshell_command_reply_t** pty, const char* uid);

void iot_webshell_command_reply_free(iot_tm_recv_webshell_command_reply_t* pty);


void iot_webshell_command_pong_init(iot_tm_msg_webshell_command_pong_t** pty, const char* uid, const char* pong_byte);

void iot_webshell_command_pong_free(iot_tm_msg_webshell_command_pong_t* pty);



#endif // ARENAL_IOT_WEBSHELL_H
