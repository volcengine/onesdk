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

#include "onesdk_config.h"

#include "util.h"
#include "aws_json.h"
#include "aws/common/json.h"
#include "aws/common/math.h"
// #include "iot/iot_utils.h"

struct aws_json_value *
new_reply_payload(struct aws_allocator *alloc, struct aws_byte_cursor idCur, int code, struct aws_json_value *data) {
    struct aws_json_value *replyPayload = aws_json_value_new_object(alloc);
    aws_json_value_add_to_object(replyPayload, aws_byte_cursor_from_c_str("ID"),
                                 aws_json_value_new_string(alloc, idCur));
    aws_json_value_add_to_object(replyPayload, aws_byte_cursor_from_c_str("Code"),
                                 aws_json_value_new_number(alloc, (double) code));
    if (data != NULL) {
        aws_json_value_add_to_object(replyPayload, aws_byte_cursor_from_c_str("Data"), data);
    }
    return replyPayload;
}

struct aws_json_value *
new_request_payload(struct aws_allocator *alloc, const char *version,
                    struct aws_json_value *data) {
    struct aws_json_value *replyPayload = aws_json_value_new_object(alloc);
    char *id = get_random_string_with_time_suffix(alloc);
    aws_json_value_add_to_object(replyPayload, aws_byte_cursor_from_c_str("id"),
                                 aws_json_value_new_string(alloc, aws_byte_cursor_from_c_str(id)));

    aws_mem_release(alloc, id);
    const char *real_version_str = version;
    // if (real_version_str == NULL) {
    //     real_version_str = SDK_VERSION;
    // }
    aws_json_value_add_to_object(replyPayload, aws_byte_cursor_from_c_str("version"),
                                 aws_json_value_new_string(alloc, aws_byte_cursor_from_c_str(real_version_str)));
    if (data != NULL) {
        aws_json_value_add_to_object(replyPayload, aws_byte_cursor_from_c_str("params"), data);
    }
    return replyPayload;
}

char *aws_cur_to_char_str(
    struct aws_allocator *allocator,
    const struct aws_byte_cursor *cur) {
    AWS_PRECONDITION(allocator);
    size_t malloc_size;
    if (aws_add_size_checked(sizeof(char), cur->len + 1, &malloc_size))
    {
        return NULL;
    }
    char *str = aws_mem_acquire(allocator, malloc_size);
    memset(str, 0, malloc_size);
    if (!str)
    {
        return NULL;
    }
    if (cur->len > 0)
    {
        memcpy((void *)str, cur->ptr, cur->len);
    }
    return str;
}


struct aws_string *aws_json_get_string_val(struct aws_json_value *data_json, const char *key) {
    struct aws_json_value *json_value = aws_json_value_get_from_object(data_json, aws_byte_cursor_from_c_str(key));
    struct aws_byte_cursor value_cur = {0};
    aws_json_value_get_string(json_value, &value_cur);
    return aws_string_new_from_cursor(aws_alloc(), &value_cur);

}


struct aws_string *aws_json_get_string1_val(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key) {
    struct aws_json_value *json_value = aws_json_value_get_from_object(data_json, aws_byte_cursor_from_c_str(key));
    struct aws_byte_cursor value_cur = {0};
    aws_json_value_get_string(json_value, &value_cur);
    return aws_string_new_from_cursor(allocator, &value_cur);
}


struct aws_byte_cursor aws_json_get_str_byte_cur_val(struct aws_json_value *data_json, const char *key) {
    struct aws_json_value *json_val = aws_json_value_get_from_object(data_json,
                                                                     aws_byte_cursor_from_c_str(
                                                                             key));
    struct aws_byte_cursor cur_val = {0};
    aws_json_value_get_string(json_val, &cur_val);
    return cur_val;
}


struct aws_byte_buf aws_json_get_json_obj_to_bye_buf(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key) {
    struct aws_json_value *json_val = aws_json_value_get_from_object(data_json, aws_byte_cursor_from_c_str(key));
    struct aws_byte_buf result_string_buf;
    aws_byte_buf_init(&result_string_buf, allocator, 0);
    aws_byte_buf_append_json_string(json_val, &result_string_buf);
    return result_string_buf;
}

struct aws_json_value* aws_json_get_json_obj(struct  aws_allocator* allocator, struct aws_json_value* data_json, const char* key) {
    struct aws_json_value* json_val = aws_json_value_get_from_object(data_json, aws_byte_cursor_from_c_str(key));
    return json_val;
}


struct aws_byte_buf aws_json_obj_to_byte_buf(struct aws_allocator *allocator, struct aws_json_value *data_json) {
    struct aws_byte_buf result_string_buf;
    aws_byte_buf_init(&result_string_buf, allocator, 0);
    aws_byte_buf_append_json_string(data_json, &result_string_buf);
    return result_string_buf;
}


char *aws_json_get_str(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key) {
    struct aws_byte_cursor cur_val = aws_json_get_str_byte_cur_val(data_json, key);
    return aws_cur_to_char_str(allocator, &cur_val);
}

void aws_json_add_str_val(struct aws_json_value *data_json, const char *key, const char *value) {
    if (value == NULL) {
        aws_json_value_remove_from_object(data_json, aws_byte_cursor_from_c_str(key));
        return;
    }
    aws_json_value_add_to_object(data_json, aws_byte_cursor_from_c_str(key),
                                 aws_json_value_new_string(aws_alloc(), aws_byte_cursor_from_c_str(value)));
}

void aws_json_add_str_val_1(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key, const char *value) {
    if (value == NULL) {
        aws_json_value_remove_from_object(data_json, aws_byte_cursor_from_c_str(key));
        return;
    }
    aws_json_value_add_to_object(data_json, aws_byte_cursor_from_c_str(key), aws_json_value_new_string(allocator, aws_byte_cursor_from_c_str(value)));
}


void aws_json_add_json_str_obj(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key, const char *value) {
    if (value == NULL) {
        aws_json_value_remove_from_object(data_json, aws_byte_cursor_from_c_str(key));
        return;
    }
    aws_json_value_add_to_object(data_json, aws_byte_cursor_from_c_str(key), aws_json_value_new_from_string(allocator, aws_byte_cursor_from_c_str(value)));
}

void aws_json_add_json_obj(struct aws_json_value *data_json, const char *key, struct aws_json_value *value) {
    if (value == NULL) {
        aws_json_value_remove_from_object(data_json, aws_byte_cursor_from_c_str(key));
        return;
    }
    aws_json_value_add_to_object(data_json, aws_byte_cursor_from_c_str(key), value);
}

void aws_json_add_aws_string_val1(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key, struct aws_string *value) {
    if (value == NULL || !aws_string_is_valid(value)) {
        aws_json_value_remove_from_object(data_json, aws_byte_cursor_from_c_str(key));
        return;
    }
    aws_json_value_add_to_object(data_json, aws_byte_cursor_from_c_str(key), aws_json_value_new_string(allocator, aws_byte_cursor_from_string(value)));
}

void aws_json_add_num_val(struct aws_json_value *data_json, const char *key, double value) {
    aws_json_value_add_to_object(data_json, aws_byte_cursor_from_c_str(key),
                                 aws_json_value_new_number(aws_alloc(), value));
}

void aws_json_add_num_val1(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key, double value) {
    aws_json_value_add_to_object(data_json, aws_byte_cursor_from_c_str(key),
                                 aws_json_value_new_number(allocator, value));
}

void aws_json_add_bool_val(struct aws_json_value *data_json, const char *key, bool value) {
    aws_json_value_add_to_object(data_json, aws_byte_cursor_from_c_str(key),
                                 aws_json_value_new_boolean(aws_alloc(), value));
}

void aws_json_add_bool_val1(struct aws_allocator *allocator, struct aws_json_value *data_json, const char *key, bool value) {
    aws_json_value_add_to_object(data_json, aws_byte_cursor_from_c_str(key),
                                 aws_json_value_new_boolean(allocator, value));
}

void aws_json_add_array_element(struct aws_json_value* array, struct aws_json_value* value) {
    aws_json_value_add_array_element(array, value);
}

double aws_json_get_num_val(struct aws_json_value *data_json, const char *key) {
    struct aws_json_value *json_val = aws_json_value_get_from_object(data_json, aws_byte_cursor_from_c_str(key));
    double value;
    aws_json_value_get_number(json_val, &value);
    return value;

}

bool aws_json_get_bool_val(struct aws_json_value *data_json, const char *key) {
    struct aws_json_value *json_val = aws_json_value_get_from_object(data_json, aws_byte_cursor_from_c_str(key));
    bool value;
    aws_json_value_get_boolean(json_val, &value);
    return value;

}