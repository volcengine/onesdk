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

#ifndef ARENAL_IOT_IOT_KV_H
#define ARENAL_IOT_IOT_KV_H
#ifdef ONESDK_ENABLE_IOT

#include <pthread.h>
#include "aws/common/byte_buf.h"
#include "aws/common/json.h"
#include "aws/common/array_list.h"
#include "aws/common/string.h"

struct iot_kv_ctx {
    struct aws_allocator *alloc;
    struct aws_string *file_path;
    struct aws_json_value *kv_json_data;
    // 读写锁
    pthread_mutex_t lock;
};

/**
 * 内部维护一个 json 数据
 * @return
 */
struct iot_kv_ctx *iot_kv_init(char *save_file_dir_path, char *filename_name);

void iot_kv_deinit(struct iot_kv_ctx *kv_ctx);

void iot_add_kv_string(struct iot_kv_ctx *kv_ctx, struct aws_byte_cursor key_cur, struct aws_byte_cursor value_cur);

void iot_add_kv_str(struct iot_kv_ctx *kv_ctx, char *key_cur, char *value_cur);

void iot_get_kv_string(struct iot_kv_ctx *kv_ctx, struct aws_byte_cursor key_cur, struct aws_byte_cursor *out_value_cur);

void iot_get_kv_str(struct iot_kv_ctx *kv_ctx, char *key_cur, char **out_value_char_str);

void iot_remove_key(struct iot_kv_ctx *kv_ctx, struct aws_byte_cursor key_cur);

void iot_remove_key_str(struct iot_kv_ctx *kv_ctx, char *key_cur);

void iot_kv_destroy(struct iot_kv_ctx *ctx);

void iot_get_kv_keys(struct iot_kv_ctx *kv_ctx, struct aws_array_list *key_list);

void iot_kv_set_num_use_cur(struct iot_kv_ctx *kv_ctx, struct aws_byte_cursor key_cur, double num);

void iot_kv_set_num(struct iot_kv_ctx *kv_ctx, char *key_cur, double num);

double iot_kv_get_num_use_cur(struct iot_kv_ctx *kv_ctx, struct aws_byte_cursor key_cur);

double iot_kv_get_num(struct iot_kv_ctx *kv_ctx, char *key_cur);


/**
 * 内有私有方法
 * @param kv_ctx
 */
void _iot_write_file(struct iot_kv_ctx *kv_ctx);

#endif //ARENAL_IOT_IOT_KV_H
#endif //ONESDK_ENABLE_IOT