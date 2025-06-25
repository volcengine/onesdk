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

#ifndef __AIGW_LLM_H
#define __AIGW_LLM_H

#include "iot_basic.h"

// 在头文件llm.h中添加错误码定义
typedef enum {
    LLM_OK = 0,          // 成功
    LLM_ERR_BASE = -100, // 错误码基准值

    // 具体错误类型
    LLM_ERR_ALLOC_FAILED,    // 内存分配失败
    LLM_ERR_DEV_REG_FAILED,  // 设备注册失败
    LLM_ERR_SIGN_FAILED,     // 签名生成失败
    LLM_ERR_INVALID_CTX,     // 上下文无效
    LLM_ERR_NETWORK_FAIL,     // 网络通信失败
    LLM_ERR_PARSE_FAILED
} llm_error_t;

typedef struct {
    llm_error_t err_code;

    char *url;
    char *api_key;
} aigw_llm_config_t;


int get_llm_config(iot_basic_ctx_t *iot_basic_ctx,
     aigw_llm_config_t *out_config);

void aigw_llm_config_destroy(aigw_llm_config_t *config);

#endif // __AIGW_LLM_H
