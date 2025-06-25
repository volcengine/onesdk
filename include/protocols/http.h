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


#ifndef ONESDK_CORE_HTTP_H
#define ONESDK_CORE_HTTP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "private_http_libs.h"
#include "private_lws_http_download.h"
#include "private_lws_http_upload.h"

#define ONESDK_HTTP_MAX_HEADER_SIZE 1024
#define ONESDK_HTTP_MAX_BODY_SIZE 10 * 1024
#define ONESDK_HTTP_MAX_HOST_SIZE 128
#define ONESDK_HTTP_MAX_PATH_SIZE 1024

#define HTTPS_HEADER_KEY_CONTENT_LENGTH "Content-Length"
#define HTTPS_HEADER_KEY_CONTENT_TYPE "Content-Type"
#define HTTPS_HEADER_KEY_CONTENT_TYPE_JSON "application/json"
#define HTTPS_HEADER_KEY_HOST "Host"

#define HTTP_PREFIX  ("http://")
#define HTTPS_PREFIX ("https://")
#define HTTP_PORT    80
#define HTTPS_PORT   443

#define HTTP_DEFAULT_CONNECT_TIMEOUT_SECOND 30  // 30s 
#define HTTP_DEFAULT_TIMEOUT_SECOND 3 * 60 // 3min

typedef enum { HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_DELETE, HTTP_HEAD } HttpMethod;

typedef struct {
    int     response_code; // response code
    char *  headers;  // response header
    char *  auth_user; // basic auth user
    char *  auth_password; // basic auth password
    char *  bearer_token; // bearer token
} http_client_t;

typedef struct {
    bool  has_more;               // flag for more data
    bool  is_chunked;            // flag for chunked data
    bool  is_sse;            // flag for text stream data; ex: text/event-stream
    sse_context_t *sse_ctx;     // sse context for current sse data
    int   retrieve_len;          // length of retrieve
    char *post_buf;              // post send data buffer(only for post body, not include headers)
    int   post_buf_len;          // post send data length
    char *post_content_type;     // type of post content; ex: application/json
    char *chunked_buf;          // response data buffer (may be a chunked data if Transfer-Encoding: chunked)
    int   chunked_buf_len;      // length of response data buffer(may be chunked if Transfer-Encoding: chunked)
    int   response_content_len;  // length of resposne content (no chunked)

} http_client_data_t;

/**
 * http 请求 response
 */
typedef struct http_response {
    size_t body_size;    // 对于同步请求，为response body的全部长度
    char* response_body; // 对于is_chunked为false情况，为完整的response body
    // 对于is_chunked为true情况，为当前chunked数据，需要调用者自己拼接
    int32_t error_code;
    int32_t inner_error_code; // http lib 返回的错误吗
    char* headers; // 当前主要用来获取待下载文件的大小
} http_response_t;



/**
 * http 异步请求时，获取body方法, download或者body过大时使用
 * @param body
 * @param body_len
 * @param is_last_chunk
 * @param user_data
 */
typedef void (*http_on_get_body_cb)(const char *body,
                                      size_t body_len,
                                      bool is_last_chunk,
                                      void *cb_user_data);

/**
 * http 异步请求时，返回sse(server sent event)时的回调，http流式接口
 *
 * @param sse_ctx
 * @param is_last_chunk
 * @param user_data
 */
typedef void (*http_on_get_sse_cb)(sse_context_t *sse_ctx,
                                      bool is_last_chunk,
                                      void *cb_user_data);

/**
 * http 异步请求时，请求结束时的回调
 *
 * @param user_data
 */
typedef void (*http_on_complete_cb)(void *cb_user_data);


/**
 * http 异步请求时，请求错误时的回调
 *
 * @param user_data
 */

typedef void (*http_on_error_cb)(int error_code, char *msg, void *cb_user_data);

// http context hold handler for http request
typedef struct http_request_context {
    http_client_t* client;
    http_client_data_t* client_data; // 存储http的下载数据
    char* url; // url地址，全路径：http://www.bytedance.com/path/user/1
    int port; // HTTP_PORT or HTTPS_PORT, set automatically by set_url
    // interal data
    char *_host; // ex: www.bytedance.com or 192.168.1.1
    char *_path; // ex: /path/user/1
    char *ca_crt;  // ca certificate
    char *headers; // http request headers
    bool is_ssl; // 是否使用ssl, 用户不用手动配置
    bool verify_ssl; // 是否验证服务器ssl证书，为false时，会跳过自签证书，证书过期，SAN等校验，仅作为测试使用
    HttpMethod method;
    int32_t connect_timeout_ms;
    int32_t timeout_ms;
    bool is_async_request;
    bool closed; // 判断是否需要是否lws相关内存

    http_on_get_body_cb on_get_body_cb;
    void *on_get_body_cb_user_data;
    http_on_get_sse_cb on_get_sse_cb;
    void *on_get_sse_cb_user_data;
    http_on_complete_cb on_complete_cb;
    void *on_complete_cb_user_data;
    http_on_error_cb on_error_cb;
    void *on_error_cb_user_data;
    //

    http_response_t *response;

    bool stream_complete;
    bool client_connection_is_shutdown;
    bool is_connection_completed; // 1 if user closed/connection error/timeout/complete 0 in-progress

    int wait_connection_result;
    bool has_request;
    // download上下文
    http_download_context_t *download_ctx;
    void *network_ctx; // 网络库的上下文,lws库时，为lws_context


} http_request_context_t;




/**
 * 请求 handler 一个请求一个 通过 onesdk_new_http_ctx 创建
 */


http_request_context_t *new_http_ctx();

void http_ctx_release(http_request_context_t *ctx);

void http_ctx_set_url(http_request_context_t *ctx, char *url);

void http_ctx_add_header(http_request_context_t *ctx, char *key, char *value);

void http_ctx_set_method(http_request_context_t *ctx, HttpMethod method);

void http_ctx_set_json_body(http_request_context_t *ctx, char *json_body);


int http_ctx_download_init(http_request_context_t *ctx, char *file_path, int file_size);

void http_ctx_set_connect_timeout_mil(http_request_context_t *ctx, int32_t time_mil);

void http_ctx_set_timeout_mil(http_request_context_t *ctx, int32_t time_mil);

void http_ctx_set_bearer_token(http_request_context_t *ctx, char *bearer_token);

void http_ctx_set_auth(http_request_context_t *ctx, char *auth_user, char *auth_password);

void http_ctx_set_ca_crt(http_request_context_t *ctx, char *ca_crt);

void http_ctx_set_verify_ssl(http_request_context_t *ctx, bool verify_ssl);
// 设置相关回调接口

void http_ctx_set_on_get_body_cb(http_request_context_t *ctx, http_on_get_body_cb callback, void *cb_user_data);

void http_ctx_set_on_get_sse_cb(http_request_context_t *ctx, http_on_get_sse_cb callback, void *cb_user_data);

void http_ctx_set_on_complete_cb(http_request_context_t *ctx, http_on_complete_cb callback, void *cb_user_data);

void http_ctx_set_on_error_cb(http_request_context_t *ctx, http_on_error_cb callback, void *cb_user_data);
// 设置相关回调结束
/**
 * 同步请求
 * 注意，如果返回结构体过大，请使用异步请求：http_request_asyn
 * @param http_context
 * @return
 */
http_response_t *http_request(http_request_context_t *http_context);



/**
 * 异步http请求
 * @param http_context
 * @param callback
 * @param userdata
 * @return
 */
int http_request_async(http_request_context_t *http_context);

void http_wait_complete(http_request_context_t *ctx);

void http_ctx_release(http_request_context_t *http_context);
void http_close(http_request_context_t *http_ctx);
// helper methods
int http_ctx_assert_request(http_request_context_t *http_ctx);

void http_ctx_set_host(http_request_context_t *http_ctx, char *host);

char* http_ctx_get_host(http_request_context_t *ctx);

int http_ctx_get_port(http_request_context_t *ctx);

char* http_ctx_get_path(http_request_context_t *ctx);

char* http_ctx_get_method_s(http_request_context_t *ctx);

void _http_response_release(http_response_t *resp);

#endif //ONESDK_CORE_HTTP_H
