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

#include "aes.h"
#include "aes_decode.h"

bool url_trim(uint8_t ch) {
    if (ch < 32) {
        return true;
    }
    switch (ch) {
        case 0x20: /* ' ' - space */
        case 0x09: /* '\t' - horizontal tab */
        case 0x0A: /* '\n' - line feed */
        case 0x0B: /* '\v' - vertical tab */
        case 0x0C: /* '\f' - form feed */
        case 0x0D: /* '\r' - carriage return */
        case '\a': /* '\r' - carriage return */
            return true;
        default:
            return false;
    }
}

// url 解密
#define DECODE_BUFF_LEN (512)
char *aes_decode(struct aws_allocator *allocator, const char *device_secret,
    const char *encrypt_data, bool partial_secret) {
    struct aws_byte_cursor encoded_cur = aws_byte_cursor_from_c_str(encrypt_data);
    size_t decoded_length = 0;
    aws_base64_compute_decoded_len(&encoded_cur, &decoded_length);
    struct aws_byte_buf output_buf;
    aws_byte_buf_init(&output_buf, allocator, decoded_length + 1);
    aws_byte_buf_secure_zero(&output_buf);
    aws_base64_decode(&encoded_cur, &output_buf);

    uint8_t aes_iv[16];
    uint16_t key_bits = AES_KEY_BITS_192;
    char *key = strdup(device_secret);
    if (partial_secret) {
        free(key);
        key = aws_mem_acquire(allocator, 16);
        memset(key, 0, 16);
        memcpy(key, device_secret, 16);
        key_bits = AES_KEY_BITS_128;
    }
    memcpy(aes_iv, device_secret, UTILS_AES_BLOCK_LEN);
    char decodeBuff[DECODE_BUFF_LEN] = {0};

    int ret = utils_aes_cbc((uint8_t *)(output_buf.buffer), output_buf.len,
        (uint8_t *)decodeBuff, DECODE_BUFF_LEN, UTILS_AES_DECRYPT,
        (uint8_t*)key, key_bits, aes_iv);
    if (ret != 0) {
        free(key);
        return NULL;
    }
    // Log_d("decoded buffer: %s", decodeBuff);
    struct aws_byte_cursor decode_cur = aws_byte_cursor_from_c_str(decodeBuff);
    struct aws_byte_cursor trim_cur = aws_byte_cursor_right_trim_pred(&decode_cur, url_trim);
    struct aws_string *decode = aws_string_new_from_cursor(allocator, &trim_cur);
    aws_byte_buf_clean_up(&output_buf);
    const char *c_str = aws_string_c_str(decode);
    char *result = strdup(c_str);
    aws_string_destroy(decode);
    free(key);

    return result;
}