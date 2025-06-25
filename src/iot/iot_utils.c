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
#ifdef ONESDK_ENABLE_IOT

#include "iot/iot_utils.h"
#include "aws/common/date_time.h"
#include "aws/common/string.h"
#include "aws/common/math.h"
#include "util/util.h"
#include "util/md5.h"
#include <stdint.h>
#include <inttypes.h>

char *iot_get_common_topic(struct aws_allocator *allocator, char *fmt, struct aws_string *product_key, struct aws_string *device_name) {
    int32_t out_size = strlen(fmt) + product_key->len + device_name->len;
    char *topic = (char*) aws_mem_calloc(allocator, 1, out_size + 1);
    sprintf(topic, fmt, aws_string_c_str(product_key), aws_string_c_str(device_name));
    return topic;
}

struct aws_string *md5File(FILE *file) {
    char input_buffer[1024] = {0};
    size_t input_size = 0;


    iot_md5_context ctx;
    utils_md5_init(&ctx);
    utils_md5_starts(&ctx);
    while ((input_size = fread(input_buffer, 1,1024,file)) >0 ) {
        utils_md5_update(&ctx, (uint8_t *) input_buffer, input_size);
    }
    uint8_t result[16];
    utils_md5_finish(&ctx, result);

    char mdt_str[32];
    AWS_ZERO_ARRAY(mdt_str);
    struct aws_byte_buf mdt_str_buf = aws_byte_buf_from_empty_array(mdt_str, sizeof(mdt_str));
    for (unsigned int i = 0; i < 16; ++i) {
        char str[3];
        sprintf(str, "%02x", result[i]);
        aws_byte_buf_write_from_whole_cursor(&mdt_str_buf, aws_byte_cursor_from_c_str(str));
    }
    struct aws_string *md5_string = aws_string_new_from_buf(aws_alloc(), &mdt_str_buf);
    return md5_string;
}

uint64_t get_current_time_mil() {
    struct aws_date_time current_time;
    aws_date_time_init_now(&current_time);
    return aws_date_time_as_millis(&current_time);
}

int32_t secure_strlen(const char *str) {
    size_t len = 0;
    aws_secure_strlen(str, 1024, &len);
    return (int32_t) len;
}

char *iot_get_topic_with_1_c_str_param(struct aws_allocator *allocator, char *fmt, struct aws_string *product_key, struct aws_string *device_name, char *p1) {
    int32_t out_size = secure_strlen(fmt) + product_key->len + device_name->len + secure_strlen(p1);
    char *topic = aws_mem_calloc(allocator, 1, out_size + 1);
    sprintf(topic, fmt, aws_string_c_str(product_key), aws_string_c_str(device_name), p1);
    return topic;
}


char *iot_get_topic_with_1_param(struct aws_allocator *allocator, char *fmt, struct aws_string *product_key, struct aws_string *device_name, struct aws_string *p1) {
    int32_t out_size = secure_strlen(fmt) + product_key->len + device_name->len + p1->len;
    char *topic = aws_mem_calloc(allocator, 1, out_size + 1);
    sprintf(topic, fmt, aws_string_c_str(product_key), aws_string_c_str(device_name), aws_string_c_str(p1));
    return topic;
}

char *iot_get_topic_with_2_c_str_param(struct aws_allocator *allocator, char *fmt, struct aws_string *product_key, struct aws_string *device_name, char *p1, char* p2) {
    int32_t out_size = secure_strlen(fmt) + product_key->len + device_name->len + secure_strlen(p1) + secure_strlen(p2);
    char *topic = aws_mem_calloc(allocator, 1, out_size + 1);
    sprintf(topic, fmt, aws_string_c_str(product_key), aws_string_c_str(device_name), p1, p2);
    return topic;
}

char *iot_get_topic_with_3_c_str_param(struct aws_allocator *allocator, const char *fmt, const char* product_key, const char* device_name, const char* p1,
    const char* p2, const char* p3) {
    int32_t out_size = secure_strlen(fmt) + secure_strlen(product_key) + secure_strlen(device_name) + secure_strlen(p1) + secure_strlen(p2) + secure_strlen(p3);
    char *topic = aws_mem_calloc(allocator, 1, out_size + 1);
    sprintf(topic, fmt, (product_key), (device_name), (p1), (p2), (p3));
    return topic;
}

const char* get_random_string_id_c_str(struct aws_allocator* allocator) {
    uint8_t randomVal = 0;
    // aws_device_random_u8(&randomVal);
    randomVal = (uint8_t)random_num();
    struct aws_date_time current_time;
    aws_date_time_init_now(&current_time);
    char* random_c_str = aws_mem_calloc(allocator, 1, 64);
    sprintf(random_c_str, "%d%llu", randomVal, aws_date_time_as_millis(&current_time));
    return random_c_str;
}

char *aws_buf_to_char_str(
    struct aws_allocator *allocator,
    const struct aws_byte_buf *cur) {
    AWS_PRECONDITION(allocator);
    size_t malloc_size;
    if (aws_add_size_checked(sizeof(char), cur->len + 1, &malloc_size)) {
        return NULL;
    }
    char *str = aws_mem_acquire(allocator, malloc_size);
    memset(str, 0, malloc_size);
    if (!str) {
        return NULL;
    }
    if (cur->len > 0) {
        memcpy((void *) str, cur->buffer, cur->len);
    }
    return str;
}
#endif  // ONESDK_ENABLE_IOT