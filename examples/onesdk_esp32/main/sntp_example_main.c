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

#include "esp_sntp.h"
#include <time.h>
#include <sys/time.h>

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
// #include "protocol_examples_common.h"
#include "esp_sntp.h"
void current_time() {
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    // Set timezone to China Standard Time
    // setenv("TZ", "CST-8", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
//    ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
    printf("[before ntp]The current date/time in Shanghai is: %s\n", strftime_buf);
    //
//    struct timeval tv;
//    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
//    sntp_sync_time(&tv);
////    strftime(strftime_buf, sizeof(strftime_buf), "%c", &tv);
////    printf("The new date/time in Shanghai is: %lld\n", tv.tv_sec);

}


// SNTP 同步成功的回调函数
void time_sync_notification_cb_2(struct timeval *tv) {
    printf("SNTP time synchronized\n");
}

int setup_ntp_time() {
    // 初始化 SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "cn.pool.ntp.org");  // 使用默认的 NTP 服务器
    sntp_set_time_sync_notification_cb(time_sync_notification_cb_2);
    esp_sntp_init();

    // 等待系统时间同步
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        printf("Waiting for system time to be set... (%d/%d)\n", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);  // 等待 2 秒
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year < (2016 - 1900)) {
        printf("Failed to obtain time.\n");
        return -1;
    }
    printf("time is syncd");
    // 设置时区为中国标准时间
    setenv("TZ", "CST-8", 1);
    tzset();

    // 打印当前时间
    char strftime_buf[64];
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    printf("The current date/time in Shanghai is: %s\n", strftime_buf);
    return 0;
}