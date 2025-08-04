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


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "platform.h"

static char persistent_mac[18] = {0};  // 新增静态存储

static char router_mac[18] = {0};  // 新增静态存储

// 新增帮助函数声明
static char* get_default_interface();
static bool get_mac_address(const char* ifname, char* buf, size_t buf_size);

char *plat_hardware_id() {
#ifdef _WIN32
    // Windows implementation
    if (router_mac[0] != '\0') {
        return strdup(router_mac);
    }
    
    // Try to get MAC address from Windows registry or network adapter
    // For now, use fallback method
    return fallback_mac_addr();
#elif defined(__linux__)
    if (router_mac[0] != '\0') {
        return strdup(router_mac);
    }
    // 获取默认路由网卡名称
    char *ifname = get_default_interface();
    if (!ifname) return fallback_mac_addr();

    // 获取MAC地址
    if (get_mac_address(ifname, router_mac, sizeof(router_mac))) {
        free(ifname);
        return strdup(router_mac);
    }

    free(ifname);
    return fallback_mac_addr();
#else
    return fallback_mac_addr();
#endif
}

// 新增帮助函数实现
static char* get_default_interface() {
#ifdef _WIN32
    // Windows implementation - for now return NULL to use fallback
    return NULL;
#elif defined(__linux__)
    FILE *fp = fopen("/proc/net/route", "r");
    if (!fp) return NULL;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char ifname[16];
        unsigned long dest;

        if (sscanf(line, "%15s %lx", ifname, &dest) == 2 && dest == 0) {
            fclose(fp);
            return strdup(ifname);
        }
    }

    fclose(fp);
    return NULL;
#else
    return NULL;
#endif
}

static bool get_mac_address(const char* ifname, char* buf, size_t buf_size) {
#ifdef _WIN32
    // Windows implementation - for now return false to use fallback
    return false;
#elif defined(__linux__)
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/net/%s/address", ifname);

    FILE *fp = fopen(path, "r");
    if (!fp) return false;

    if (fgets(buf, buf_size, fp)) {
        // 移除末尾换行符
        buf[strcspn(buf, "\n")] = '\0';
        fclose(fp);
        return true;
    }

    fclose(fp);
    return false;
#else
    return false;
#endif
}

char *fallback_mac_addr() {
    // 仅首次调用时生成
    if (persistent_mac[0] != '\0') {
        return strdup(persistent_mac);
    }
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    int hour_seed = tm_info->tm_hour * 3600;
    srand((unsigned int)hour_seed);
    uint8_t bytes[6];

    // 生成符合规范的MAC地址（本地管理地址 + 单播）
    bytes[0] = (uint8_t)((rand() % 0xFE) | 0x02); // 保证是本地管理地址
    for (int i = 1; i < 6; i++) {
        bytes[i] = (uint8_t)(rand() % 0xFF);
    }

    // 格式化为MAC地址字符串
    snprintf(persistent_mac, sizeof(persistent_mac),
             "%02x:%02x:%02x:%02x:%02x:%02x",
             bytes[0], bytes[1], bytes[2],
             bytes[3], bytes[4], bytes[5]);

    return strdup(persistent_mac);
}