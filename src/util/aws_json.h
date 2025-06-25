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

#ifndef _UTIL_AWS_JSON_H
#define _UTIL_AWS_JSON_H

#include "aws/common/byte_buf.h"
#include "aws/common/string.h"
#include "aws/common/json.h"

struct aws_json_value *
new_reply_payload(struct aws_allocator *alloc, struct aws_byte_cursor idCur, int code, struct aws_json_value *data);

struct aws_json_value *
new_request_payload(struct aws_allocator *alloc, const char *version,
                    struct aws_json_value *data);

char *aws_cur_to_char_str(
        struct aws_allocator *allocator,
        const struct aws_byte_cursor *cur);

struct aws_string *aws_json_get_string_val(struct aws_json_value *data_json, const char *key);

struct aws_string *aws_json_get_string1_val(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key);

struct aws_byte_cursor aws_json_get_str_byte_cur_val(struct aws_json_value *data_json, const char *key);

struct aws_byte_buf aws_json_get_json_obj_to_bye_buf(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key);

struct aws_json_value* aws_json_get_json_obj(struct  aws_allocator* allocator, struct aws_json_value* data_json, const char* key);

struct aws_byte_buf aws_json_obj_to_byte_buf(struct aws_allocator *allocator, struct aws_json_value *data_json);


char *aws_json_get_str(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key);

void aws_json_add_str_val_1(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key, const char *value);

void aws_json_add_json_str_obj(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key, const char *value);

void aws_json_add_json_obj(struct aws_json_value *data_json, const char *key, struct aws_json_value *value);

void aws_json_add_str_val(struct aws_json_value *data_json, const char *key, const char *value);

void aws_json_add_aws_string_val(struct aws_json_value *data_json, const char *key, struct aws_string *value);

void aws_json_add_aws_string_val1(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key, struct aws_string *value);

void aws_json_add_num_val(struct aws_json_value *data_json, const char *key, double value);

void aws_json_add_num_val1(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key, double value);

void aws_json_add_bool_val(struct aws_json_value *data_json, const char *key, bool value);

void aws_json_add_bool_val1(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key, bool value);

void aws_json_add_array_element(struct aws_json_value* data_json, struct aws_json_value* array);

double aws_json_get_num_val(struct aws_json_value *data_json, const char *key);

bool aws_json_get_bool_val(struct aws_json_value *data_json, const char *key);


#endif  // _UTIL_AWS_JSON_H