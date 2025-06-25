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
#include "aws/common/string.h"
#include "iot/dynreg.h"
#include "util/util.h"
#include "aigw/auth.h"
#include "protocols/private_http_libs.h"
#include "plat/platform.h"


#define MAX_TIMESTAMP_LEN 20

http_request_context_t *device_auth_client(http_request_context_t *http_ctx, iot_basic_ctx_t *iot_basic_ctx) {
    if (http_ctx == NULL) {
        http_ctx = new_http_ctx();
    }
    int32_t rnd = random_num();
    uint64_t ts = unix_timestamp();
    struct aws_allocator *alloc = aws_alloc();
    aws_common_library_init(alloc);

    struct iot_dynamic_register_basic_param registerParam = {
        .instance_id = iot_basic_ctx->config->instance_id,
        .auth_type = iot_basic_ctx->config->auth_type,
        .timestamp = ts,
        .random_num = rnd,
        .product_key = iot_basic_ctx->config->product_key,
        .device_name = iot_basic_ctx->config->device_name
    };


    if (iot_basic_ctx->config->device_secret == NULL) {
        int ret = dynamic_register(iot_basic_ctx);
        if (ret != 0) {
            return NULL;
        }
    }
    struct aws_string *sign = iot_hmac_sha256_encrypt(alloc, &registerParam, iot_basic_ctx->config->device_secret);
    const char *signature = aws_string_c_str(sign);


    http_ctx_add_header(http_ctx, (char *)HEADER_SIGNATURE, (char *)signature);
    http_ctx_add_header(http_ctx, (char *)HEADER_AUTH_TYPE, (char *)iot_auth_type_c_str(iot_basic_ctx->config->auth_type));
    http_ctx_add_header(http_ctx, (char *)HEADER_DEVICE_NAME, (char *)iot_basic_ctx->config->device_name);
    http_ctx_add_header(http_ctx, (char *)HEADER_PRODUCT_KEY, (char *)iot_basic_ctx->config->product_key);
    char str[MAX_TIMESTAMP_LEN];
    sprintf(str, "%ld", rnd);
    http_ctx_add_header(http_ctx, HEADER_RANDOM_NUM, str);
    sprintf(str, "%llu", ts);
    http_ctx_add_header(http_ctx, HEADER_TIMESTAMP, str);
    char *hw_id = plat_hardware_id();
    http_ctx_add_header(http_ctx, HEADER_AIGW_HARDWARE_ID, hw_id);

    free(hw_id);
    aws_string_destroy(sign);
    return http_ctx;
}

aigw_auth_header_t *aigw_auth_header_new(iot_dynamic_register_basic_param_t *param, const char *device_secret) {
    struct aws_string *sign = iot_hmac_sha256_encrypt(aws_alloc(), param, device_secret);
    const char *signature = aws_string_c_str(sign);
    aigw_auth_header_t *header = malloc(sizeof(aigw_auth_header_t));
    header->signature = strdup(signature);
    header->auth_type = strdup(iot_auth_type_c_str(param->auth_type));
    header->device_name = strdup(param->device_name);
    header->product_key = strdup(param->product_key);
    char str[MAX_TIMESTAMP_LEN];
    sprintf(str, "%ld", param->random_num);
    header->random_num = strdup(str);
    sprintf(str, "%llu", param->timestamp);
    header->timestamp = strdup(str);
    aws_string_destroy(sign);
    return header;
}

void aigw_auth_header_free(aigw_auth_header_t *header) {
    if (header == NULL) {
        return;
    }
    free(header->signature);
    free(header->auth_type);
    free(header->device_name);
    free(header->product_key);
    free(header->random_num);
    free(header->timestamp);
    free(header);
}