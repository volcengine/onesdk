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


#include "esp_log.h"
#include "audio_utils.h"
#include "mbedtls/base64.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define TAG "AUDIO_UTILS"
// ... existing code ...
int onesdk_base64_encode(const char *input, size_t input_length, char **output, size_t *output_length) {

    
    // printf("sending audio data len1: %zu\n" , input_length); // 使用 %zu 打印 size_t
    size_t required_len = ((input_length + 2) / 3) * 4 + 1; // Base64 编码后长度的估算 (包含 '\0')
    // int ret = mbedtls_base64_encode(NULL, 0, &required_len,
    //                      (const unsigned char *)input,
    //                      input_length);

    // if (ret != 0) {
    //     ESP_LOGE(TAG, "failed to get encode base64 output length, error code: %d", ret);
    //     return ret;
    // }
    // 分配输出缓冲区
    *output = malloc(required_len);
    if (*output == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for decoded data");
        return -1;
    }

    // size_t encoded_len = 0; // encoded_len 未被使用，可以移除或视情况保留
        // 执行编码
    int ret = mbedtls_base64_encode((unsigned char *)*output, required_len, output_length, // required_len 作为缓冲区大小
                               (const unsigned char *)input, input_length);

    // printf("encoded audio data len: %d, %d\n" , strlen(*output), *output_length);
    if (ret != 0) {
        ESP_LOGE(TAG, "Base64 encoding failed with error code: -0x%04X", -ret); // mbedtls 错误码通常为负
        free(*output);
        *output = NULL;
        return ret;
    }
    (*output)[*output_length] = '\0'; // 确保字符串以 null 结尾
    // printf("encoded audio data len1: %zu, actual encoded_len: %zu\n" , strlen(*output), *output_length); // 使用 %zu
    return ret;
}

int onesdk_base64_decode(const char *input, size_t input_length, char **output, size_t *output_length) {
    // printf("onesdk_base64_decode received audio data len: %zu\n" , input_length); // 使用 %zu 打印 size_t
    // printf("onesdk_base64_decode input data: <%s>\n", input); // 打印输入数据
    size_t actual_decoded_len = 0;
    // 第一次调用：获取解码后所需的实际长度
    int ret = mbedtls_base64_decode(NULL, 0, &actual_decoded_len,
                                   (const unsigned char *)input, input_length);
    // 如果输入长度为0，mbedtls_base64_decode 返回 0，actual_decoded_len 为 0
    // 如果输入有效且非空，它返回 MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL，并且 actual_decoded_len 被设置为所需大小
    // 其他返回值表示错误（例如，无效字符）
    if (ret != 0 && ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
        ESP_LOGE(TAG, "mbedtls_base64_decode (get length) failed: -0x%04X", -ret);
        return ret;
    }
    // printf("response audio len %d, actual_decoded_len: %zu\n", input_length, actual_decoded_len);
    // 分配足够的内存，包括末尾的 '\0'
    *output = (char *)malloc(actual_decoded_len + 1);
    if (*output == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for decoded data (%zu bytes)", actual_decoded_len + 1);
        return -1; // 或者更具体的错误码
    }

    // 第二次调用：执行实际的解码操作
    // output_length 将接收实际写入的字节数
    ret = mbedtls_base64_decode((unsigned char*)*output, actual_decoded_len, output_length,
                               (const unsigned char*)input, input_length);
    if (ret != 0) {
        ESP_LOGE(TAG, "onesdk_base64_decode Base64 decoding failed with error code: -0x%04X", -ret);
        free(*output); // 修正：释放 *output 指向的内存
        *output = NULL;
        return ret;
    }

    // 确保解码后的数据以 null 结尾，以便作为字符串使用
    (*output)[*output_length] = '\0';

    // 检查实际解码长度是否与预期一致 (通常它们应该相等)
    if (*output_length != actual_decoded_len) {
        ESP_LOGW(TAG, "Base64 decoding length mismatch: expected %zu, got %zu", actual_decoded_len, *output_length);
    }

    // printf("decoded audio data len (strlen): %zu, actual decoded_len: %zu\n" , strlen(*output), *output_length); // 使用 %zu
    return ret;
}