/*
 * Copyright 2022-2024 Beijing Volcano Engine Technology Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "onesdk_config.h"
#ifdef ONESDK_ENABLE_IOT

#include "iot/iot_kv.h"
#include "cJSON.h"
#include "iot_log.h"
#include "aws/common/byte_buf.h"
#include "aws/common/json.h"
#include "util/aws_json.h"
#include "aws/common/common.h"
#include "aws/common/array_list.h"
#include "aws/common/string.h"
#include "util/util.h"
#include <libwebsockets.h>

#define IOT_TAG_KV "iot_kv"

struct iot_kv_ctx *iot_kv_init(char *save_file_dir_path, char *filename_name) {
    struct iot_kv_ctx *kv_ctx = aws_mem_acquire(aws_alloc(), sizeof(struct iot_kv_ctx));
    AWS_ZERO_STRUCT(*kv_ctx);
    kv_ctx->alloc = aws_alloc();
    lws_pthread_mutex_init(&kv_ctx->lock);

    char filename_array[1024];
    AWS_ZERO_ARRAY(filename_array);
    struct aws_byte_buf filename_buf = aws_byte_buf_from_empty_array(filename_array, sizeof(filename_array));
    aws_byte_buf_write_from_whole_cursor(&filename_buf, aws_byte_cursor_from_c_str(save_file_dir_path));

    // 通用存储目录
    struct aws_string *common_file_dir_string = aws_string_new_from_c_str(kv_ctx->alloc, save_file_dir_path);
    // 创建目录
    // if (!aws_directory_exists(common_file_dir_string)) {
    //     aws_directory_create(common_file_dir_string);
    // }
    aws_string_destroy_secure(common_file_dir_string);

    aws_byte_buf_write_from_whole_cursor(&filename_buf, aws_byte_cursor_from_c_str(filename_name));

    struct aws_string *kv_file_path = aws_string_new_from_buf(kv_ctx->alloc, (const struct aws_byte_buf *)(&filename_buf));
    kv_ctx->file_path = kv_file_path;

    LOGD(IOT_TAG_KV, "kv_file_path = %s", aws_string_c_str(kv_file_path));

    if (access(aws_string_c_str(kv_file_path), F_OK)) {
        LOGD(IOT_TAG_KV, "kv_file_path = %s not exists", aws_string_c_str(kv_file_path));
        struct aws_json_value *kv_content_json = aws_json_value_new_object(kv_ctx->alloc);
        kv_ctx->kv_json_data = kv_content_json;
    } else {
        LOGI(IOT_TAG_KV, "kv_file_path = %s HAL_FileExist", aws_string_c_str(kv_file_path));
        struct aws_byte_buf file_buf = {0};
        aws_byte_buf_init_from_file(&file_buf, kv_ctx->alloc, aws_string_c_str(kv_file_path));
        struct aws_json_value *kv_content_json = aws_json_value_new_from_string(kv_ctx->alloc,
                                                                                aws_byte_cursor_from_buf(&file_buf));
        aws_byte_buf_clean_up(&file_buf);
        if (kv_content_json == NULL) {
            kv_ctx->kv_json_data = aws_json_value_new_object(kv_ctx->alloc);
        } else {
            kv_ctx->kv_json_data = kv_content_json;
        }
    }
    return kv_ctx;
}


void iot_kv_deinit(struct iot_kv_ctx *kv_ctx) {
    aws_string_destroy_secure(kv_ctx->file_path);
    aws_json_value_destroy(kv_ctx->kv_json_data);
    lws_pthread_mutex_destroy(&kv_ctx->lock);
}

void _iot_write_file(struct iot_kv_ctx *kv_ctx) {
    // 把整个 json 写入到文件
    // FILE *kv_vile = aws_fopen(aws_string_c_str(kv_ctx->file_path), "wb");
    FILE *kv_vile = fopen(aws_string_c_str(kv_ctx->file_path), "wb");
    struct aws_byte_buf json_data_buf;
    aws_byte_buf_init(&json_data_buf, kv_ctx->alloc, 0);

    aws_byte_buf_append_json_string(kv_ctx->kv_json_data, &json_data_buf);
    fwrite(json_data_buf.buffer, 1, json_data_buf.len, kv_vile);
    fflush(kv_vile);
    fclose(kv_vile);
    aws_byte_buf_clean_up(&json_data_buf);
}

void iot_add_kv_str(struct iot_kv_ctx *kv_ctx, char *key_cur, char *value_cur) {
    iot_add_kv_string(kv_ctx, aws_byte_cursor_from_c_str(key_cur), aws_byte_cursor_from_c_str(value_cur));
}

void iot_add_kv_string(struct iot_kv_ctx *kv_ctx, struct aws_byte_cursor key_cur, struct aws_byte_cursor value_cur) {
    // 写加锁处理
    lws_pthread_mutex_lock(&kv_ctx->lock);

    aws_json_value_remove_from_object(kv_ctx->kv_json_data, key_cur);
    aws_json_value_add_to_object(kv_ctx->kv_json_data, key_cur,
                                 aws_json_value_new_string(kv_ctx->alloc, value_cur));
    _iot_write_file(kv_ctx);
    lws_pthread_mutex_unlock(&kv_ctx->lock);
}

void
iot_get_kv_string(struct iot_kv_ctx *kv_ctx, struct aws_byte_cursor key_cur, struct aws_byte_cursor *out_value_cur) {
    struct aws_json_value *value_json = aws_json_value_get_from_object(kv_ctx->kv_json_data, key_cur);
    if (!aws_json_value_is_null(value_json)) {
        aws_json_value_get_string(value_json, out_value_cur);
    } else {
        out_value_cur->len = 0;
    }
}

void iot_get_kv_str(struct iot_kv_ctx *kv_ctx, char *key_cur, char **out_value_char_str) {
    struct aws_byte_cursor out_value_cur = {0};
    iot_get_kv_string(kv_ctx, aws_byte_cursor_from_c_str(key_cur), &out_value_cur);
    *out_value_char_str = aws_cur_to_char_str(aws_alloc(), &out_value_cur);
}


void iot_kv_set_num(struct iot_kv_ctx *kv_ctx, char *key_cur, double num) {
    iot_kv_set_num_use_cur(kv_ctx, aws_byte_cursor_from_c_str(key_cur), num);
}


void iot_kv_set_num_use_cur(struct iot_kv_ctx *kv_ctx, struct aws_byte_cursor key_cur, double num) {
    lws_pthread_mutex_lock(&kv_ctx->lock);

    aws_json_value_remove_from_object(kv_ctx->kv_json_data, key_cur);
    aws_json_value_add_to_object(kv_ctx->kv_json_data, key_cur,
                                 aws_json_value_new_number(kv_ctx->alloc, num));
    _iot_write_file(kv_ctx);
    lws_pthread_mutex_unlock(&kv_ctx->lock);
}

double iot_kv_get_num(struct iot_kv_ctx *kv_ctx, char *key_cur) {
    return iot_kv_get_num_use_cur(kv_ctx, aws_byte_cursor_from_c_str(key_cur));
}

double iot_kv_get_num_use_cur(struct iot_kv_ctx *kv_ctx, struct aws_byte_cursor key_cur) {
    struct aws_json_value *value_json = aws_json_value_get_from_object(kv_ctx->kv_json_data, key_cur);
    if (!aws_json_value_is_null(value_json)) {
        double value;
        aws_json_value_get_number(value_json, &value);
        return value;
    } else {
        return 0;
    }
}

void iot_remove_key(struct iot_kv_ctx *kv_ctx, struct aws_byte_cursor key_cur) {
    lws_pthread_mutex_lock(&kv_ctx->lock);
    aws_json_value_remove_from_object(kv_ctx->kv_json_data, key_cur);
    _iot_write_file(kv_ctx);
    lws_pthread_mutex_unlock(&kv_ctx->lock);
}

void iot_remove_key_str(struct iot_kv_ctx *kv_ctx, char *key_cur) {
    iot_remove_key(kv_ctx, aws_byte_cursor_from_c_str(key_cur));
}

void iot_get_kv_keys(struct iot_kv_ctx *kv_ctx, struct aws_array_list *key_list) {
    if (kv_ctx->kv_json_data == NULL) {
        return;
    }
    struct cJSON *cjson = (struct cJSON *) kv_ctx->kv_json_data;
    cjson = cjson->child;
    while (cjson != NULL) {
        printf("getKvKeys key =  %s\n", cjson->string);
        struct aws_string *key_string = aws_string_new_from_c_str(kv_ctx->alloc, cjson->string);
        aws_array_list_push_back(key_list, &key_string);
        cjson = cjson->next;
    }
}

void iot_kv_destroy(struct iot_kv_ctx *ctx) {
    aws_string_destroy_secure(ctx->file_path);
    aws_json_value_destroy(ctx->kv_json_data);
    lws_pthread_mutex_destroy(&ctx->lock);
}
#endif