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

#include <time.h>
#include <stdlib.h>
#ifdef _WIN32
#include "platform_compat.h"
#else
#include <sys/time.h>
#endif
#include "util.h"


uint64_t plat_unix_timestamp_ms() { // 函数名修改为 _ms 以表示毫秒
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000);
}

// 保留原来的秒级时间戳函数，如果其他地方还在使用的话
uint64_t plat_unix_timestamp() {
    return (uint64_t) time(NULL);
}

int32_t plat_random_num() {
    srand((unsigned) plat_unix_timestamp());
    return rand();
}

