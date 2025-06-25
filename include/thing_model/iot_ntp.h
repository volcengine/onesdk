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



#ifndef IOT_SDK_IOT_NTP_H
#define IOT_SDK_IOT_NTP_H

#include <stdint.h>
#include "iot_mqtt.h"

typedef struct {
    uint64_t server_time_mil;
} iot_tm_recv_npt_server_time_t;



int tm_ntp_get_server_time(iot_mqtt_ctx_t *mqtt_ctx, uint64_t* server_time_mil);


int32_t tm_send_device_npt_request(void *handler);


#endif //IOT_SDK_IOT_NTP_H
