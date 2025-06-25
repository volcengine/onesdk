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

#include <libwebsockets.h>

#include "protocols/http.h"
#include "protocols/private_lws_http_download.h"
#include "protocols/private_lws_http_client.h"
#include "error_code.h"

/*
http相关header
curl -v  -H "Range: bytes=0-5119" <ddownload_url>
< Accept-Ranges: bytes
< Content-Disposition: attachment
< Content-Length: 5120
< Content-Range: bytes 0-5119/112429
*/




/*
符合定义：http_on_get_body_cb
*/
void _internal_download_handle(const char *in, size_t in_len, bool is_last_chunk, void *user_data) {
    http_download_context_t *dl_ctx = (http_download_context_t *)user_data;
    if (!dl_ctx) return;
    if (in && in_len > 0) {
        // write file to data
        if (dl_ctx->file_fd == NULL) {
            // open file
            lwsl_info("open file %s for download\n", dl_ctx->file_path);
            dl_ctx->file_fd = (void *)fopen(dl_ctx->file_path, "wb+");
            if (dl_ctx->file_fd == NULL) {
                lwsl_err("open file %s failed\n", dl_ctx->file_path);
                return;
            }
        }
        // write file
        fwrite(in, 1, in_len, dl_ctx->file_fd);
        dl_ctx->downloaded_size += in_len;
    }
}

void _internal_download_complete(void *user_data) {
    http_download_context_t *dl_ctx = (http_download_context_t *)user_data;
    if (!dl_ctx) return;
    // close the file 
    if (dl_ctx->file_fd) {
        fclose(dl_ctx->file_fd);
        dl_ctx->file_fd = NULL;
    }
    lwsl_info("download complete, file %s closed.\n", dl_ctx->file_path);
}

void _internal_download_error(int error_code, void *user_data) {
    http_download_context_t *dl_ctx = (http_download_context_t *)user_data;
    if (!dl_ctx) return;
    // close the file
    lwsl_info("download error: %d, closing file %s\n", error_code, dl_ctx->file_path);
    if (dl_ctx->file_fd) {
        fclose(dl_ctx->file_fd);
        dl_ctx->file_fd = NULL;
    }
}


// 发送Range请求头
static int add_range_header(struct lws *wsi, size_t start, size_t end)
{
    char range_header[64];
    snprintf(range_header, sizeof(range_header), "bytes=%d-%d", start, end);
    
    unsigned char *p;
    if (lws_add_http_header_by_name(wsi, (unsigned char*)"Range",
                                   (unsigned char*)range_header, strlen(range_header),
                                   &p, NULL))
        return -1;
                                   
    return 0;
}

// 初始化下载上下文
http_download_context_t *lws_http_download_init(void *parent_http_ctx, char *file_path, int file_size) {
    if (!parent_http_ctx) return NULL;
    // 初始化基础上下文
    http_download_context_t *dl_ctx = (http_download_context_t *)malloc(sizeof(http_download_context_t));
    if (!dl_ctx) return NULL;
    memset(dl_ctx, 0, sizeof(http_download_context_t));
    dl_ctx->parent_http_ctx = parent_http_ctx;
    dl_ctx->total_size = file_size;
    dl_ctx->downloaded_size = 0;
    dl_ctx->file_path = file_path;
    dl_ctx->file_fd = NULL;
    dl_ctx->on_download_data = _internal_download_handle;
    dl_ctx->on_download_complete = _internal_download_complete;
    dl_ctx->on_download_error = _internal_download_error;
    dl_ctx->user_data = dl_ctx;
    // 
    return dl_ctx;
}

// 设置下载路径
int lws_http_download_set_url(http_download_context_t *dl_ctx, const char* url) {
    if (!dl_ctx || !url) return -1;
    http_ctx_set_url(dl_ctx->parent_http_ctx, (char *)url);
    return 0;
}

// 设置下载回调
// int lws_http_download_set_callback(http_download_context_t *dl_ctx, http_download_data_cb* on_download_data, http_download_complete_cb* on_download_complete, http_download_error_cb* on_download_error, void* user_data) {
//     if (!dl_ctx) return -1;
//     dl_ctx->on_download_data = on_download_data;
//     dl_ctx->on_download_complete = on_download_complete;
//     dl_ctx->on_download_error = on_download_error;
//     dl_ctx->user_data = user_data;
//     return 0;
// }


// 设置下载文件大小
int lws_http_download_set_file_size(http_download_context_t *dl_ctx, size_t file_size) {
    if (!dl_ctx) return -1;
    dl_ctx->total_size = file_size;
    return 0;
}

// 设置下载超时时间
int lws_http_download_set_timeout(http_download_context_t *dl_ctx, int timeout_ms) {
    if (!dl_ctx) return -1;
    http_ctx_set_timeout_mil(dl_ctx->parent_http_ctx, timeout_ms);
    return 0;
}

// 设置下载CA证书
int lws_http_download_set_ca_crt(http_download_context_t *dl_ctx, const char* ca_crt) {
    if (!dl_ctx || !ca_crt) return -1;
    http_ctx_set_ca_crt(dl_ctx->parent_http_ctx, (char *)ca_crt);
    return 0;
}

// 发送HEAD请求获取文件信息
int lws_http_download_get_size(http_download_context_t *dl_ctx) {
    if (!dl_ctx) return -1;
    if (!dl_ctx->parent_http_ctx) return -1;
    http_request_context_t *http_ctx = (http_request_context_t *)dl_ctx->parent_http_ctx;
    http_ctx->method = HTTP_HEAD;
    http_response_t *resp = http_request(dl_ctx->parent_http_ctx);
    if (resp != NULL) {
        if (resp->error_code == 200) {
            
        }
    }
    return 0;
}

// 启动分段下载
int lws_http_download_start(http_download_context_t *dl_ctx) {
    if (!dl_ctx) return -1;
    if (dl_ctx->total_size == 0) {
        int r = lws_http_download_get_size(dl_ctx);
        if (r != 0) {
            return r;
        }
    }
    return 0;
}

// 取消下载
int lws_http_download_cancel(http_download_context_t *dl_ctx) {
    if (!dl_ctx) return -1;
    lws_http_client_disconnect(dl_ctx->parent_http_ctx);
    return 0;
}

// 释放下载上下文
int lws_http_download_deinit(http_download_context_t *dl_ctx) {
    if (!dl_ctx) return VOLC_OK;
    if (dl_ctx->file_fd) {
        fclose(dl_ctx->file_fd);
        dl_ctx->file_fd = NULL;
    }
    free(dl_ctx);
    return VOLC_OK;
}