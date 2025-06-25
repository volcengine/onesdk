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

// #include "aws/common/allocator.h"
#include <string.h>
#include "util/util.h"

#include "error_code.h"
#include "protocols/http.h"
#include "protocols/private_lws_http_client.h"
#include "protocols/private_lws_http_download.h"
#include "protocols/private_lws_http_upload.h"

http_request_context_t *new_http_ctx() {
    // iot 相关实现
    http_request_context_t *http_context = malloc(sizeof(http_request_context_t));
    if (!http_context) {
        return NULL;
    }
    memset(http_context, 0, sizeof(http_request_context_t));
    http_context->url = NULL;
    http_context->ca_crt = NULL;
    http_context->is_ssl = false;
    http_context->verify_ssl = false;
    // 初始化client
    http_context->client = (http_client_t *)malloc(sizeof(http_client_t));
    memset(http_context->client, 0, sizeof(http_client_t));
    http_context->client->response_code = 0;
    // 初始化 response headers
    http_context->client->headers = NULL;
    // 初始化client_data

    http_context->client_data = (http_client_data_t *)malloc(sizeof(http_client_data_t));
    memset(http_context->client_data, 0, sizeof(http_client_data_t));
    http_context->client_data->is_chunked = false;
    http_context->client_data->post_content_type = NULL;
    http_context->client_data->post_buf = NULL;
    http_context->client_data->post_buf_len = 0;
    http_context->client_data->chunked_buf = NULL;
    http_context->client_data->chunked_buf_len = 0;

    // 初始化 sse
    http_context->client_data->sse_ctx = (sse_context_t *)malloc(sizeof(sse_context_t));
    memset(http_context->client_data->sse_ctx, 0, sizeof(sse_context_t));
    memset(http_context->client_data->sse_ctx->buffer, 0, SSE_BUFFER_SIZE);
    http_context->client_data->sse_ctx->data = NULL;
    http_context->client_data->sse_ctx->data_len = 0;
    http_context->client_data->sse_ctx->event = NULL;
    http_context->client_data->sse_ctx->event_len = 0;
    http_context->client_data->sse_ctx->id = NULL;
    http_context->client_data->sse_ctx->id_len = 0;
    http_context->client_data->sse_ctx->used_len = 0;
    memset(http_context->client_data->sse_ctx->buffer, 0, SSE_BUFFER_SIZE);
    // 初始化http_response
    http_context->response = (http_response_t *)malloc(sizeof(http_response_t));
    memset(http_context->response, 0, sizeof(http_response_t));
    http_context->response->error_code = 0;
    http_context->response->inner_error_code = 0;
    http_context->response->body_size = 0;
    http_context->response->headers = NULL;
    http_context->response->response_body = NULL;
    

    http_context->timeout_ms = HTTP_DEFAULT_TIMEOUT_SECOND * 1000;
    http_context->connect_timeout_ms = HTTP_DEFAULT_CONNECT_TIMEOUT_SECOND * 1000;
    return http_context;
}


void http_ctx_set_url(http_request_context_t *ctx, char *purl) {
    // printf("http_ctx_set_url called with %s\n", purl);
    if(purl == NULL || strlen(purl) == 0) {
        printf("purl is NULL\n");
        return;
    }
    ctx->url = purl;
    const char *protocol;
    char url_copy[1024];
    const char *host_copy;
    const char *path_copy;
    strcpy(url_copy, purl);
    // printf("before parse: %p, len = %d, proto = %p\n", url_copy, strlen(url_copy), protocol);
    int ret = lws_http_client_parse_url(url_copy , &protocol, &host_copy, &ctx->port, &path_copy);
    if (ret != 0) {
        printf("Failed to parse URL, please check %s\n", url_copy);
        return;
    }
    // printf("after parse: protocol = %s, host = %s, path = %s\n", protocol, host_copy, path_copy);
    int host_len = strlen(host_copy);
    int path_len = strlen(path_copy);

    ctx->_host = malloc(host_len + 1);
    ctx->_path = malloc(path_len + 1);
    memset(ctx->_host, 0, host_len + 1);
    memset(ctx->_path, 0, path_len + 1);
    strncpy(ctx->_host, host_copy, strlen(host_copy));
    strncpy(ctx->_path, path_copy, strlen(path_copy));
    // printf("1Host: %p|%s\n", ctx->_host, ctx->_host);
    // printf("1Port: %d\n", ctx->port);
    // printf("1Path: %p|%s\n", ctx->_path, ctx->_path);
    if (strcmp(protocol, HTTPS_PREFIX) == 0) {
        ctx->is_ssl = true;
        printf("1is_ssl: %d\n", ctx->is_ssl);
        if (ctx->port == 0) {
            // default to HTTPS_PORT
            ctx->port = HTTPS_PORT;
        }
    }
    if (strcmp(protocol, HTTP_PREFIX) == 0) {
        ctx->is_ssl = false;
        printf("2is_ssl: %d\n", ctx->is_ssl);
        if (ctx->port == 0) {
            // default to HTTP_PORT
            ctx->port = HTTP_PORT;
        }
    }
    if (protocol == NULL || strlen(protocol) == 0) {
        printf("protocol is NULL\n");
        protocol = "http";
        ctx->is_ssl = false;
    }

}

void http_ctx_add_header(http_request_context_t *ctx, char *key, char *value) {

    // Assuming ctx->headers is a char* buffer to store the concatenated headers
    if (ctx->headers == NULL) {
        // TODO: HAL_Malloc
        ctx->headers = (char*)malloc(1); // Allocate initial buffer
        ctx->headers[0] = '\0'; // Initialize as empty string
    }

    // Calculate the new length needed
    size_t current_length = strlen(ctx->headers);
    size_t key_length = strlen(key)+1;
    size_t value_length = strlen(value)+1;
    size_t new_length = current_length + key_length + value_length + 4; // 4 for ": ", "\r\n", and null terminator

        // 分配新内存并复制旧数据
    char *new_header = (char *)malloc(new_length);
    if (new_header == NULL) {
        printf("Failed to allocate memory for new header\n");
        return;
    }
    memset(new_header, 0, new_length);
    if (ctx->headers) {
        strcpy(new_header, ctx->headers); // 复制旧数据
        free(ctx->headers);               // 释放旧内存
    }
    ctx->headers = new_header;
    // Append the new header
    strcat(ctx->headers, key);
    strcat(ctx->headers, ": ");
    strcat(ctx->headers, value);
    strcat(ctx->headers, "\r\n");
}

void http_ctx_set_method(http_request_context_t *ctx, const HttpMethod method) {
    ctx->method = method;
}

void http_ctx_set_json_body(http_request_context_t *ctx, char *json_body) {
    if (json_body == NULL) {
        return;
    }
    if (ctx->client_data == NULL) {
        return;
    }

    // 先释放旧的post_buf
    if (ctx->client_data->post_buf != NULL) {
        free(ctx->client_data->post_buf);
        ctx->client_data->post_buf = NULL;
    }

    size_t current_length = strlen(json_body);
    char *buf = malloc(current_length + 1);
    if (buf == NULL) {
        fprintf(stderr, "Failed to allocate memory for json_body: code %d\n", VOLC_ERR_HTTP_MALLOC_FAILED);
        return;
    }
    memset(buf, 0, current_length + 1);
    memcpy(buf, json_body, current_length);
    buf[current_length] = '\0';
    ctx->client_data->post_buf = buf;
    ctx->client_data->post_buf_len = current_length;
    ctx->client_data->post_content_type = "application/json";
}

void http_ctx_set_connect_timeout_mil(http_request_context_t *ctx, int32_t time_mil) {
    ctx->connect_timeout_ms = time_mil;
}

void http_ctx_set_timeout_mil(http_request_context_t *ctx, int32_t time_mil) {
    ctx->timeout_ms = time_mil;
}

void http_ctx_set_bearer_token(http_request_context_t *ctx, char *bearer_token) {
    if (bearer_token == NULL) {
        return;
    }
    // 拷贝bearer_token,并\0结尾
    size_t current_length = strlen(bearer_token);
    char *buf = malloc(current_length + 1);
    memset(buf, 0, current_length + 1);
    memcpy(buf, bearer_token, current_length);
    buf[current_length] = '\0';
    ctx->client->bearer_token = buf;
}

void http_ctx_set_auth(http_request_context_t *ctx, char *auth_user, char *auth_password) {
    ctx->client->auth_user = auth_user;
    ctx->client->auth_password = auth_password;
}


void http_ctx_set_ca_crt(http_request_context_t *ctx, char *ca_crt) {
    if (ca_crt == NULL) {
        return;
    }
    ctx->is_ssl = true;
    // 拷贝ca_crt,并\0结尾
    size_t current_length = strlen(ca_crt);
    char *buf = malloc(current_length + 1);
    memset(buf, 0, current_length + 1);
    memcpy(buf, ca_crt, current_length);
    buf[current_length] = '\0';
    // fprintf(stderr, "http_ctx_set_ca_crt: %s\n", buf);
    ctx->ca_crt = buf;
}
void http_ctx_set_verify_ssl(http_request_context_t *ctx, bool verify_ssl) {
    ctx->verify_ssl = verify_ssl;
}

int http_ctx_download_init(http_request_context_t *ctx, char *file_path, int file_size) {
    // 初始化下载上下文
    http_download_context_t *dl_ctx = lws_http_download_init(ctx, file_path, file_size);
    if (dl_ctx == NULL) {
        return VOLC_ERR_HTTP_MALLOC_FAILED;
    }
    ctx->download_ctx = dl_ctx;
    return VOLC_OK;
}

void http_ctx_set_on_get_body_cb(http_request_context_t *ctx, http_on_get_body_cb callback, void *cb_user_data) {
    if (callback == NULL) {
        return;
    }
    ctx->on_get_body_cb = callback;
    ctx->on_get_body_cb_user_data = cb_user_data;
}

void http_ctx_set_on_get_sse_cb(http_request_context_t *ctx, http_on_get_sse_cb callback, void *cb_user_data) {
    if (callback == NULL) {
        return;
    }
    ctx->on_get_sse_cb = callback;
    ctx->on_get_sse_cb_user_data = cb_user_data;
}

void http_ctx_set_on_complete_cb(http_request_context_t *ctx, http_on_complete_cb callback, void *cb_user_data) {
    if (callback == NULL) {
        return;
    }
    ctx->on_complete_cb = callback;
    ctx->on_complete_cb_user_data = cb_user_data;
}

void http_ctx_set_on_error_cb(http_request_context_t *ctx, http_on_error_cb callback, void *cb_user_data) {
    if (callback == NULL) {
        return;
    }
    ctx->on_error_cb = callback;
    ctx->on_error_cb_user_data = cb_user_data;
}

/**
 * 同步请求
 * 注意，如果返回结构体过大，请使用异步请求：http_request_async
 * @param http_context
 * @return
 */
http_response_t *http_request(http_request_context_t *http_context) {

    http_context->is_async_request = false;
    // TODO 待实现 同步请求
    int ret = 0;
    ret = lws_http_client_init(http_context);
    if (ret != 0) {
        // error out
        printf("Failed to initialize HTTP client\n");
        return NULL;
    }
    ret = lws_http_client_connect(http_context);
    if (ret != 0) {
        http_context->response->error_code = ret;
        return NULL;
    }
    http_response_t *response = lws_http_client_send_request(http_context);

    return response;
}



/**
 * 异步http请求
 * @param http_context
 * @param callback
 * @param userdata
 * @return
 */
int http_request_async(http_request_context_t *http_context) {
    http_context->is_async_request = true;

    int ret = 0;
    ret = lws_http_client_init(http_context);
    if (ret != 0) {
        // error out
        fprintf(stderr, "Failed to initialize HTTP client\n");
        http_context->response->inner_error_code = ret;
        return VOLC_ERR_HTTP_CONN_INIT_FAILED;
    }
    ret = lws_http_client_connect(http_context);
    if (ret != 0) {
        http_context->response->error_code = ret;
        return ret;
    }
    http_response_t *rest = lws_http_client_send_request(http_context);
    if (rest != NULL) {
        lwsl_debug("http_request_async rest %ld, %ld\n", rest->error_code, rest->inner_error_code);
        if (rest->error_code != 0) {
            return rest->error_code;
        }
        if (rest->inner_error_code != 0) {
            return rest->inner_error_code;
        }
    }
    return VOLC_OK;
}

void http_close(http_request_context_t *http_ctx) {
    // 清理 http相关信息 from utils_url_download.c->qcloud_url_download_deinit
    if (http_ctx == NULL) {
        return;
    }
    if (http_ctx->closed) {
        return;
    }
    http_ctx->closed = true;
    if (http_ctx == NULL) {
        return;
    }
    if (http_ctx->client != NULL) {
        lws_http_client_disconnect(http_ctx);
    }
    // if (http_ctx->client_data != NULL) {
    //     free(http_ctx->client_data);
    //     http_ctx->client_data = NULL;
    // }
    // if (http_ctx->headers != NULL) {
    //     free(http_ctx->headers);
    //     http_ctx->headers = NULL;
    // }
}

void http_wait_complete(http_request_context_t *http_ctx) {
    // 等待异步请求完成
    if (http_ctx == NULL) {
        return;
    }
    int n = 0;
    lwsl_debug("http_wait_complete %p\n", http_ctx);
    struct lws_context *ctx = (struct lws_context *)http_ctx->network_ctx;
    while (n >= 0 && !http_ctx->is_connection_completed) {
        lwsl_debug("http_wait_complete n = %d\n", n);
        n = lws_service(ctx, 0);
    }


    // if (close_after_start)
    // 	lws_wsi_close(client_wsi, LWS_TO_KILL_SYNC);
}

void _http_response_release(http_response_t *response);
void http_ctx_release(http_request_context_t *http_context) {
    // LOGD(onesdk_TAG_HTTP, "http_ctx_release called");
    // Log_i("enter http_ctx_release");
    
    if (http_context != NULL) {
        http_close(http_context);

        lws_http_client_destroy_network_context(http_context);
        if (http_context->headers != NULL) {
            free(http_context->headers);
            http_context->headers = NULL;
        }
        if (http_context->response != NULL) {
            _http_response_release(http_context->response);
            http_context->response = NULL;
        }
        if (http_context->ca_crt != NULL) {
            free(http_context->ca_crt);
            http_context->ca_crt = NULL;
        }
        if (http_context->headers != NULL) {
            free(http_context->headers);
            http_context->headers = NULL;
        }
        if (http_context->client != NULL) {
            if (http_context->client->headers != NULL) {
                free(http_context->client->headers);
                http_context->client->headers = NULL;
            }
            if (http_context->client->bearer_token != NULL) {
                free(http_context->client->bearer_token);
                http_context->client->bearer_token = NULL;
            }
            free(http_context->client);
            http_context->client = NULL;
        }
        if (http_context->client_data != NULL) {
            _http_client_data_free(http_context->client_data);
            http_context->client_data = NULL;
        }
        if (http_context->_host != NULL) {
            free(http_context->_host);
            http_context->_host = NULL;
        }
        if (http_context->_path != NULL) {
            free(http_context->_path);
            http_context->_path = NULL;
        }
        if (http_context->download_ctx != NULL) {
            lws_http_download_deinit(http_context->download_ctx);
            http_context->download_ctx = NULL;
        }
        free(http_context);
        http_context = NULL;
    }
}

void _http_response_release(http_response_t *response) {
    if (response != NULL) {
        // free header buffer
        if (response->headers != NULL) {
            free(response->headers);
            response->headers = NULL;
        }
        if (response->response_body != NULL) {
            free(response->response_body);
            response->response_body = NULL;
        }
        // free response body buffer
        free(response);
        response = NULL;
    }
}

// helper function for http client
int http_ctx_assert_request(http_request_context_t *http_ctx) {
    if (http_ctx == NULL) {
      return 1;
    }
    if (http_ctx->url == NULL) {
      return 2;
    }
    if (http_ctx->client_data == NULL) {
      return 3;
    }

    if (http_ctx->client == NULL) {
      return 4;
    }
    if (http_ctx->network_ctx == NULL) {
      return 5;
    }
    return 0;
}

void http_ctx_set_host(http_request_context_t *http_ctx, char *host) {
    if (http_ctx == NULL || host == NULL) {
        return;
    }
    http_ctx->_host = host;
}
void http_ctx_set_port(http_request_context_t *http_ctx, int32_t port) {

}

char* http_ctx_get_host(http_request_context_t *http_ctx) {
    return http_ctx->_host;
}

int http_ctx_get_port(http_request_context_t *http_ctx) {
    return http_ctx->port;
}

char* http_ctx_get_path(http_request_context_t *http_ctx) {
    return http_ctx->_path;
}

char* http_ctx_get_method_s(http_request_context_t *http_ctx) {
    HttpMethod method = http_ctx->method;
    char *        meth                                = (method == HTTP_GET)      ? "GET"
    : (method == HTTP_POST)   ? "POST"
    : (method == HTTP_PUT)    ? "PUT"
    : (method == HTTP_DELETE) ? "DELETE"
    : (method == HTTP_HEAD)   ? "HEAD"
                              : "";
    return meth;
}

void _http_client_data_sse_free(sse_context_t *sse) {
    if (sse == NULL) {
        return;
    }
    if (sse->data != NULL) {
        free(sse->data);
        sse->data = NULL;
    }
    if (sse->event != NULL) {
        free(sse->event);
        sse->event = NULL;
    }
    if (sse->id != NULL) {
        free(sse->id);
        sse->id = NULL;
    }
    free(sse);
    sse = NULL;
}


void _http_client_data_free(http_client_data_t *data) {
    if (!data) return;

    // 释放SSE上下文
    if (data->sse_ctx) {
        _http_client_data_sse_free(data->sse_ctx);
        data->sse_ctx = NULL;
    }

    // 释放POST数据缓冲区
    if (data->post_buf) {
        free(data->post_buf);
        data->post_buf = NULL;
    }

    // 释放分块数据缓冲区
    if (data->chunked_buf) {
        free(data->chunked_buf);
        data->chunked_buf = NULL;
    }

    // // 释放POST内容类型字符串（假设是动态分配的）
    // if (data->post_content_type) {
    //     free(data->post_content_type);
    //     data->post_content_type = NULL;
    // }
    free(data);
    data = NULL;
}