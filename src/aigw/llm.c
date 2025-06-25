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
#include <stdint.h>

#include "aigw/llm.h"
#include "iot_basic.h"
#include "iot/dynreg.h"
#include "protocols/http.h"
#include "util/util.h"
#include "util/aws_json.h"
#include "util/aes_decode.h"
#include "aws/common/byte_buf.h"
#include "aws/common/encoding.h"
#include "util/hmac_sha256.h"

#define GET_LLM_CONFIG_PATH "/2021-12-14/GetLLMConfig"

#define API_VERSION_QUERY_PARAM "Version=2021-12-14"
#define API_ACTION_GET_LLM_CONFIG  "Action=GetLLMConfig"

static int parse_llm_config(struct aws_allocator *allocator, const char *device_secret,
    const http_response_t *response, aigw_llm_config_t *out_config);

int get_llm_config(iot_basic_ctx_t *iot_basic_ctx,
     aigw_llm_config_t *out_config) {
    int32_t rnd = random_num();
    uint64_t ts = unix_timestamp();
    struct aws_allocator *alloc = aws_alloc();
    aws_common_library_init(alloc);

    struct iot_dynamic_register_basic_param registerParam = {
        .instance_id = iot_basic_ctx->config->instance_id,
        .timestamp = ts,
        .random_num = rnd,
        .product_key = iot_basic_ctx->config->product_key,
        .device_name = iot_basic_ctx->config->device_name,
        .auth_type = iot_basic_ctx->config->auth_type,
    };

    if (iot_basic_ctx->config->device_secret == NULL) {
        int ret = dynamic_register(iot_basic_ctx);
        if (ret != 0) {
            return LLM_ERR_DEV_REG_FAILED;
        }
    }
    // llm api auth_type must be 0.
    registerParam.auth_type = 0;

    struct aws_string *sign = iot_hmac_sha256_encrypt(alloc, &registerParam, iot_basic_ctx->config->device_secret);

    struct aws_json_value *post_data_json = aws_json_value_new_object(alloc);

    struct aws_string *instance_id = aws_string_new_from_c_str(alloc, iot_basic_ctx->config->instance_id);
    struct aws_string *product_key = aws_string_new_from_c_str(alloc, iot_basic_ctx->config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(alloc, iot_basic_ctx->config->device_name);

    aws_json_add_aws_string_val1(alloc, post_data_json, "InstanceID", instance_id);
    aws_json_add_aws_string_val1(alloc, post_data_json, "product_key", product_key);
    aws_json_add_aws_string_val1(alloc, post_data_json, "device_name", device_name);
    aws_json_add_num_val1(alloc, post_data_json, "random_num", registerParam.random_num);
    aws_json_add_num_val1(alloc, post_data_json, "timestamp", (double)registerParam.timestamp);
    aws_json_add_aws_string_val1(alloc, post_data_json, "signature", sign);

    aws_string_destroy_secure(instance_id);
    aws_string_destroy_secure(product_key);
    aws_string_destroy_secure(device_name);
    aws_string_destroy_secure(sign);

        // http 请求 url + path + query 参数
    char url_str[1024];
    AWS_ZERO_ARRAY(url_str);
    struct aws_byte_buf url_str_buf = aws_byte_buf_from_empty_array(url_str, sizeof(url_str));
    // if (iot_basic_ctx->config->verify_ssl) {
    //     aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str("https://"));
    // } else {
    //     aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str("http://"));
    // }
    aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str(iot_basic_ctx->config->http_host));
    aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str(GET_LLM_CONFIG_PATH));
    aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str("?"));
    aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str(API_ACTION_GET_LLM_CONFIG));
    aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str("&"));
    aws_byte_buf_write_from_whole_cursor(&url_str_buf, aws_byte_cursor_from_c_str(API_VERSION_QUERY_PARAM));

    // printf("get llm config url_str_buf = %s\n", url_str);
    struct aws_byte_buf post_data_json_buf = aws_json_obj_to_byte_buf(alloc, post_data_json);
    struct aws_string *body_str = aws_string_new_from_buf(alloc, &post_data_json_buf);

    http_request_context_t *http_ctx = new_http_ctx();
    http_ctx_set_url(http_ctx, url_str);
    http_ctx_set_method(http_ctx, HTTP_POST);

    // optional: set headers
    http_ctx_add_header(http_ctx, HTTPS_HEADER_KEY_CONTENT_TYPE, HTTPS_HEADER_KEY_CONTENT_TYPE_JSON);
    const char *content = (char *)aws_string_c_str(body_str);
    printf("llm request body: %s\n", content);
    http_ctx_set_json_body(http_ctx, (char *)content);

    http_response_t *response = http_request(http_ctx);
    int ret = 0;
    if (response != NULL) {
        printf("llm response body: %s\n", response->response_body);

        // if (result->meta_info.responseMetaDataError.Code == 0 && result->result.len > 0) {
        ret = parse_llm_config(alloc, iot_basic_ctx->config->device_secret, response, out_config);
        if (ret != LLM_OK) {
            goto done;
        }
    } else {
        ret = LLM_ERR_NETWORK_FAIL;
    }

    done:
    aws_byte_buf_clean_up_secure(&post_data_json_buf);
    aws_json_value_destroy(post_data_json);
    aws_string_destroy_secure(body_str);
    http_ctx_release(http_ctx);
    return ret;
}

static int parse_llm_config(struct aws_allocator *allocator, const char *device_secret,
    const http_response_t *response, aigw_llm_config_t *out_config) {
    if (out_config == NULL) {
        return LLM_ERR_ALLOC_FAILED;
    }

    struct aws_json_value *response_json = aws_json_value_new_from_string(allocator,
        aws_byte_cursor_from_c_str(response->response_body));
    if (!response_json) {
        goto error_cleanup;
    }

    struct aws_json_value *result_json = aws_json_value_get_from_object(response_json,
        aws_byte_cursor_from_c_str("Result"));
    if (!result_json) {
        goto error_cleanup;
    }

    char *key = aws_json_get_str(allocator, result_json, "APIKey");
    if (!key) {
        goto error_cleanup;
    }

    char *api_key = aes_decode(allocator, device_secret, key, false);
    free(key);  // 修正内存释放方式
    if (!api_key) {
        goto error_cleanup;
    }

    out_config->api_key = api_key;
    char * url = aws_json_get_str(allocator, result_json, "URL");
    if (!url) {
        goto error_cleanup;
    }
    out_config->url = strdup(url);
    free(url);

    // 修正JSON对象释放方式
    aws_json_value_destroy(response_json);
    return LLM_OK;

error_cleanup:
    if (response_json)
        aws_json_value_destroy(response_json);
    return LLM_ERR_PARSE_FAILED;
}

void aigw_llm_config_destroy(aigw_llm_config_t *config) {
    if (!config) return;
    if (config->api_key) {
        free(config->api_key);
        config->api_key = NULL;
    }
    if (config->url) {
        free(config->url);
        config->url = NULL;
    }
    free(config);
    config = NULL;
}