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

#include <inttypes.h>
#include "aws/common/byte_buf.h"
#include "aws/common/encoding.h"
#include "aws/common/json.h"

#include "dynreg.h"
#include "util/hmac_sha256.h"
#include "util/util.h"
#include "util/aws_json.h"
#include "util/aes_decode.h"
#include "error_code.h"

#define SHA256_HMAC_LEN 32

#define DYNAMIC_REGISTER_PATH "/2021-12-14/DynamicRegister"

#define API_VERSION = "2021-12-14"
#define API_VERSION_QUERY_PARAM "Version=2021-12-14"
#define API_ACTION_DYNAMIC_REGISTER  "Action=DynamicRegister"

#define HTTPS_DEFAULT_CONNECTION_TIME_OUT (10 * 1000)

enum iot_http_method {
    GET,
    POST,
    HEADER,
};

int dynamic_register(iot_basic_ctx_t *ctx) {
    if (ctx == NULL ) {
        return -1;
    }
    if (ctx->config->device_secret != NULL && strlen(ctx->config->device_secret) > 0) {
        return 0;
    }

    int32_t rnd = random_num();
    uint64_t ts = unix_timestamp();
    struct aws_allocator *alloc = aws_alloc();
    aws_common_library_init(alloc);

    struct iot_dynamic_register_basic_param registerParam = {
        .instance_id = ctx->config->instance_id,
        .auth_type = ctx->config->auth_type,
        .timestamp = ts,
        .random_num = rnd,
        .product_key = ctx->config->product_key,
        .device_name = ctx->config->device_name
    };

    struct aws_string *sign = iot_hmac_sha256_encrypt(alloc, &registerParam, ctx->config->product_secret);

    // json 数据
    struct aws_json_value *post_data_json = aws_json_value_new_object(alloc);

    struct aws_string *instance_id = aws_string_new_from_c_str(alloc, ctx->config->instance_id);
    struct aws_string *product_key = aws_string_new_from_c_str(alloc, ctx->config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(alloc, ctx->config->device_name);

    aws_json_add_aws_string_val1(alloc, post_data_json, "InstanceID", instance_id);
    aws_json_add_aws_string_val1(alloc, post_data_json, "product_key", product_key);
    aws_json_add_aws_string_val1(alloc, post_data_json, "device_name", device_name);
    aws_json_add_num_val1(alloc, post_data_json, "random_num", registerParam.random_num);
    aws_json_add_num_val1(alloc, post_data_json, "timestamp", (double)registerParam.timestamp);
    aws_json_add_num_val1(alloc, post_data_json, "auth_type", ctx->config->auth_type);
    aws_json_add_aws_string_val1(alloc, post_data_json, "signature", sign);

    aws_string_destroy_secure(instance_id);
    aws_string_destroy_secure(product_key);
    aws_string_destroy_secure(device_name);
    aws_string_destroy_secure(sign);

    // http 请求 url + path + query 参数
    char url_str[1024];
    AWS_ZERO_ARRAY(url_str);
    struct aws_byte_buf url_str_buf = aws_byte_buf_from_empty_array(url_str, sizeof(url_str));
    // aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str("https://"));
    aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str(ctx->config->http_host));
    aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str(DYNAMIC_REGISTER_PATH));
    aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str("?"));
    aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str(API_ACTION_DYNAMIC_REGISTER));
    aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str("&"));
    aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str(API_VERSION_QUERY_PARAM));
    // add query params

    // printf("dynamic_register url_str_buf = %s", url_str);
    struct aws_byte_buf post_data_json_buf = aws_json_obj_to_byte_buf(alloc, post_data_json);
    struct aws_string *body_str = aws_string_new_from_buf(alloc, &post_data_json_buf);

    http_request_context_t *http_ctx = new_http_ctx();
    http_ctx_set_url(http_ctx, url_str);
    http_ctx_set_method(http_ctx, HTTP_POST);

    // optional: set headers
    http_ctx_add_header(http_ctx, HTTPS_HEADER_KEY_CONTENT_TYPE, HTTPS_HEADER_KEY_CONTENT_TYPE_JSON);
    char *bstr = (char *)aws_string_c_str(body_str);
    printf("dynamic_register body_str = %s\n", bstr);
    http_ctx_set_json_body(http_ctx, bstr);

    http_response_t *response = http_request(http_ctx);
    iot_http_response_dynamic_register_t *result;
    int ret = VOLC_OK;
    if (response != NULL) {
        if (response->error_code == 200 ) {
            iot_http_response_dynamic_register_t *result = parse_dynamic_register(alloc, response);
            if (result->result.len > 0) {
                char *decoded = aes_decode(alloc, ctx->config->product_secret, result->result.payload, true);
                if (ctx->config->device_secret != NULL) {
                    free((void*)ctx->config->device_secret);
                }
                ctx->config->device_secret = decoded;
            } else {
                ret = -1;
            }
            if (result->result.payload) {
                aws_mem_release(alloc, result->result.payload);
            }
            if (result->meta_info.action) {
                aws_mem_release(alloc, result->meta_info.action);
            }
            if (result->meta_info.version) {
                aws_mem_release(alloc, result->meta_info.version);
            }
            aws_mem_release(alloc, result);
        } else {

            ret = response->error_code != 0? response->error_code :(response->inner_error_code != 0? response->inner_error_code : VOLC_ERR_HTTP_CONN_FAILED);
            fprintf(stderr, "dynamic_register failed, error_code = %d\n", ret);
            return ret;
        }

    } else {
        ret = VOLC_ERR_HTTP_CONN_FAILED;
    }

// done:
    aws_byte_buf_clean_up_secure(&post_data_json_buf);
    aws_json_value_destroy(post_data_json);
    aws_string_destroy_secure(body_str);
    http_ctx_release(http_ctx);

    return ret;
}

struct aws_string *iot_hmac_sha256_encrypt(struct aws_allocator *allocator,
                                                   const struct iot_dynamic_register_basic_param *registerBasicParam, const char *secret)
{
    char inputStr[256] = {0};
    sprintf(inputStr, "auth_type=%" PRId32 "&device_name=%s&random_num=%" PRId32 "&product_key=%s&timestamp=%" PRIu64,
            (int32_t)registerBasicParam->auth_type, registerBasicParam->device_name, (int32_t)registerBasicParam->random_num,
            registerBasicParam->product_key, (uint64_t)registerBasicParam->timestamp);

    struct aws_byte_cursor secretBuff = aws_byte_cursor_from_c_str(secret);
    struct aws_byte_cursor inputBuff = aws_byte_cursor_from_c_str(inputStr);

    uint8_t output[SHA256_HMAC_LEN] = {0};

    onesdk_hmac_sha256(secretBuff.ptr, secretBuff.len, inputBuff.ptr, inputBuff.len, output, sizeof(output));
    struct aws_byte_buf sha256buf = aws_byte_buf_from_array(output, sizeof(output));

    size_t terminated_length = 0;
    aws_base64_compute_encoded_len(sha256buf.len, &terminated_length);

    struct aws_byte_buf byte_buf;
    aws_byte_buf_init(&byte_buf, allocator, terminated_length + 2);

    struct aws_byte_cursor sha256Cur = aws_byte_cursor_from_buf(&sha256buf);
    aws_base64_encode(&sha256Cur, &byte_buf);

    struct aws_string *encrypt_string = aws_string_new_from_buf(allocator, &byte_buf);
    aws_byte_buf_clean_up(&byte_buf);

    return encrypt_string;
}

static iot_http_response_dynamic_register_t *parse_dynamic_register(struct aws_allocator *allocator,
    const http_response_t *response)
{
    iot_http_response_dynamic_register_t *dynamic_register_res = aws_mem_calloc(allocator, 1, sizeof(iot_http_response_dynamic_register_t));

    if (response->error_code != 0)
    {
        dynamic_register_res->meta_info.responseMetaDataError.CodeN = response->error_code;
        dynamic_register_res->meta_info.responseMetaDataError.Message = "http request failed";
    }

    printf("response->response_body = %s\n", response->response_body);
    struct aws_byte_cursor response_json_cur = aws_byte_cursor_from_c_str(response->response_body);
    struct aws_json_value *response_json = aws_json_value_new_from_string(allocator, response_json_cur);
    struct aws_json_value *result_json = aws_json_value_get_from_object(response_json, aws_byte_cursor_from_c_str("Result"));
    struct aws_json_value *metadata_json = aws_json_value_get_from_object(response_json, aws_byte_cursor_from_c_str("ResponseMetadata"));

    dynamic_register_res->meta_info.action = aws_json_get_str(allocator, metadata_json, "Action");
    dynamic_register_res->meta_info.version = aws_json_get_str(allocator, metadata_json, "Version");

    if (aws_json_value_has_key(metadata_json, aws_byte_cursor_from_c_str("Error")))
    {
        struct aws_json_value *error_json = aws_json_value_get_from_object(metadata_json, aws_byte_cursor_from_c_str("Error"));
        if (error_json != NULL)
        {
            dynamic_register_res->meta_info.responseMetaDataError.Code = aws_json_get_str(allocator, error_json, "Code");
            dynamic_register_res->meta_info.responseMetaDataError.CodeN = (int32_t)aws_json_get_num_val(error_json, "CodeN");
            dynamic_register_res->meta_info.responseMetaDataError.Message = aws_json_get_str(allocator, error_json, "Message");
        }
    }
    dynamic_register_res->result.len = (int32_t)aws_json_get_num_val(result_json, "len");
    dynamic_register_res->result.payload = aws_json_get_str(allocator, result_json, "payload");

    aws_json_value_destroy(response_json);

    return dynamic_register_res;
}
